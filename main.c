#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#ifdef __APPLE__
#else
#include <sys/inotify.h>
#endif

#define EVENT_SIZE (sizeof(struct inotify_event))
#define INOTIFY_BUF_LEN (1024 * (EVENT_SIZE + 16))

typedef struct {
  char *path;
  uint32_t path_str_len;
  int wd;
} Folder;

typedef struct {
  Folder *arr;
  uint64_t len;
  uint64_t cap;
} Folders;

struct ProgramOptions {
  char *HELP;
  char *IGNORE_FILES;
} PROGRAM_OPTIONS = { "--help\0", "--ignore\0" };

const long SLEEP_FOR_NSECONDS = 500 * 1000000;
struct timespec req_sleep_time = { .tv_sec = 0, .tv_nsec = SLEEP_FOR_NSECONDS };
struct timespec rem_sleep_time;

pid_t PID_OF_CMD; 
int PROCESS_STATUS;
char CURRENT_DIRECTORY[] = ".";
const int DEFAULT_IGNORES_LEN = 2;
char *DEFAULT_IGNORES[2] = { "..", "." };
char **IGNORE_DIRS;
int IGNORE_DIRS_LEN = 0;
int fd_inotify;


void reportErrAndExitProgram(
    char *err_message,
    int exit_status
) {
  if (PID_OF_CMD > 0) {
    printf("\nKilling PID %d and exiting program \n", PID_OF_CMD);
    kill(-PID_OF_CMD, SIGTERM);
    waitpid(PID_OF_CMD, &PROCESS_STATUS, 0);
  }

  close(fd_inotify);
  exit(exit_status);
}


Folders* folderArrMake(uint32_t len) {
  errno = 0;

  uint32_t cap = 0; 
  if(len > 0) {
    cap = len * 2;
  }
  assert(cap > 0);

  Folder *folder_arr = malloc(cap * sizeof(Folder));
  if (folder_arr == NULL) {
    perror("Error allocating mem for FileMeta\n");
    return NULL;
  }

  for(int i = 0; i < cap; i++) {
    char *default_path_size = malloc(5 * sizeof(char));
    if(default_path_size == NULL) {
      perror("Error allocating mem of default string for FileMeta.path\n");
      return NULL;
    }

    folder_arr[i].path = default_path_size;
    folder_arr[i].path_str_len = 5;
  }

  Folders *folders= malloc(sizeof(Folders));
  if (folders== NULL) {
    perror("Error allocating mem for Files\n");
    return NULL;
  }

  folders->arr = folder_arr;
  folders->len = 0;
  folders->cap = cap;

  return folders;
}

void folderArrAdd(Folders *folders, char *path_to_add, uint64_t *idx) {
  uint64_t new_idx = folders->len;
  if(idx == NULL) {
    idx = &new_idx;
    assert(*idx == folders->len);
  }
  assert(idx != NULL);
  

  int is_valid_position = *idx <= folders->len; 
  if (!is_valid_position) {
   return; 
  }

  int has_to_increase_cap = *idx > folders->cap - 1;
  int has_to_increase_len = *idx == folders->len;

  if(has_to_increase_cap) {
    int new_cap = folders->cap * 2;
    Folder *reassigned_folder_arr = malloc(new_cap * sizeof(Folder));
    if(reassigned_folder_arr == NULL) {
      perror("Error allocating mem for Folder array\n");
      return;
    }

    Folder *previous_folder_arr = folders->arr;
    for(int i = 0; i < new_cap; i++) {
      if(i < folders->len) {
        reassigned_folder_arr[i].path = previous_folder_arr[i].path;
        reassigned_folder_arr[i].path_str_len = previous_folder_arr[i].path_str_len;
      } else {
        reassigned_folder_arr[i].path = NULL;
        reassigned_folder_arr[i].path_str_len = 0;
      }
    }

    folders->arr = reassigned_folder_arr;
    folders->cap = new_cap; 
    free(previous_folder_arr);
  }
  if(has_to_increase_len) {
    folders->len = folders->len + 1;
  }
  
  int path_to_add_len = strlen(path_to_add) + 1; //NULL CHAR
  int has_to_increase_char_size = folders->arr[*idx].path_str_len < path_to_add_len;
  if(has_to_increase_char_size) {
    int new_path_len = path_to_add_len * 2;
    char *new_path = malloc(new_path_len);
    if(new_path == NULL) {
      perror("Error allocating mem for new FileMeta.path size\n");
      return;
    }

    for(int i = 0; i < path_to_add_len - 1; i++) {
      new_path[i] = path_to_add[i];
    }
    new_path[path_to_add_len - 1] = '\0';

    char *prev_allocd_str = folders->arr[*idx].path;

    folders->arr[*idx].path = new_path;
    folders->arr[*idx].path_str_len = new_path_len;

    free(prev_allocd_str);
    return;
  }

  for(int i = 0; i < path_to_add_len; i++) {
    if(i == path_to_add_len - 1) {
      folders->arr[*idx].path[i] = '\0';
      break;
    }
    folders->arr[*idx].path[i] = path_to_add[i];
  }
}

void folderArrReset(Folders *folders) {
  for(int i = 0; i < folders->len; i++) {
    for(int j = 0; j < folders->arr[i].path_str_len; j++) {
      if(j == folders->arr[i].path_str_len - 1) {
        folders->arr[i].path[j] = '\0';
        break;
      }
      folders->arr[i].path[j] = ' ';
    }
  }

  folders->len = 0;
}


int isInvalidPath(char *path) {
  int path_len = strlen(path); 

  for(int i = 0; i < path_len; i++) {
    if(path_len + 1 < path_len && path[i] == '/' && path[i + 1] == '/') {
      reportErrAndExitProgram("Invalid path. Check root path\n", EXIT_FAILURE);
    }
  }

  return 1;
}

int isIgnoredPath(char *path) {
  for(int i = 0; i < IGNORE_DIRS_LEN; i++) {
    if(strcmp(path, IGNORE_DIRS[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

void execCmd(char *cmd[]) {
  errno = 0;
  PID_OF_CMD = fork(); 

  if(PID_OF_CMD == -1) {
    perror("Fork failed");
    reportErrAndExitProgram(NULL, EXIT_FAILURE);
  }
  if(PID_OF_CMD == 0) {
    setpgid(0, 0);
    execvp(cmd[0], cmd);
  }
}

void killPidAndRestart(char *cmd[]) {
  int status;
  if(PID_OF_CMD > 0) {
    //KILL GROUP WIHT -pid
    kill(-PID_OF_CMD, SIGTERM);
    waitpid(PID_OF_CMD, &status, 0);
    execCmd(cmd);
    return;
  }

  reportErrAndExitProgram("\nInvalid pid while trying to restart the program\n", EXIT_FAILURE);
}

#ifdef __APPLE__
void addFoldersToWatcher(Folders *folders) {

}
#else
void addFoldersToWatcher(Folders *folders) {
  int wd_inotify;
  for(int i = 0; i < folders->len; i++) {
    errno = 0;
    wd_inotify = inotify_add_watch(
      fd_inotify, 
      folders->arr[i].path,
      IN_MODIFY | IN_CREATE | IN_DELETE
    );
    if (wd_inotify < 0) {
      perror("add watch failed");
      reportErrAndExitProgram(NULL, EXIT_FAILURE);
      break;
    }
    
    folders->arr[i].wd = wd_inotify;
  }
}
#endif

void removeWatchers(Folders *folders) {
  int result = 0;
  for(int i = 0; i < folders->len; i++) {
    errno = 0;
    result = 0;
    result = inotify_rm_watch(
      fd_inotify, 
      folders->arr[i].wd
    );
    // printf("\nRemoving wd: %d. Result: %d\n", folders->arr[i].wd, result);
    if (result < 0) {
      perror("remove watch failed");
      reportErrAndExitProgram(NULL, EXIT_FAILURE);
      break;
    }
    
  }
}

Folders *getFoldersFromPath(Folders *folders, char *root) {
  DIR *dir_to_get_folders;
  struct dirent *found_dir;
  dir_to_get_folders = opendir(root);

  if (strcmp(root, CURRENT_DIRECTORY) == 0) {
    folderArrAdd(folders, CURRENT_DIRECTORY, NULL);
  }
  if (dir_to_get_folders) {
    while ((found_dir = readdir(dir_to_get_folders)) != NULL) {
      if(found_dir->d_type == DT_DIR){
        if(isIgnoredPath(found_dir->d_name)){
          continue;
        } else {
          char full_path[strlen(root) + strlen(found_dir->d_name) + (1 * sizeof(char))];
          sprintf(full_path, "%s/%s", root, found_dir->d_name);
          puts(full_path);
          isInvalidPath(full_path);
          folderArrAdd(folders, full_path, NULL);
          getFoldersFromPath(folders, full_path);
        }
      }
    }
    closedir(dir_to_get_folders);
  }


  return folders;
}



void execCmdAndWatch(char *cmd[], Folders *folders) {
  errno = 0;
  fd_inotify = inotify_init();
  if (fd_inotify < 0) {
    perror("inotify_init");
    reportErrAndExitProgram(NULL, EXIT_FAILURE);
  }

  execCmd(cmd);

  if(PID_OF_CMD > 0) {
    addFoldersToWatcher(folders);

    int length, i = 0;
    char buffer[INOTIFY_BUF_LEN];
    struct inotify_event *event;
    while (1) {
      i = 0;
      length = read(fd_inotify, buffer, INOTIFY_BUF_LEN);
      if (length < 0) {
        perror("read");
      }


      while (i < length) {
        event = (struct inotify_event *) &buffer[i];
        if (event->len) {
            if (event->mask & IN_CREATE || event->mask & IN_DELETE || event->mask & IN_MODIFY) {
              printf("The file %s was created/modified/deleted.\n", event->name);
              removeWatchers(folders);
              folderArrReset(folders);
              getFoldersFromPath(folders, CURRENT_DIRECTORY);
              addFoldersToWatcher(folders);
              killPidAndRestart(cmd);
              break;
            }
        }
        i += EVENT_SIZE + event->len;
      }

    }
  }
}

void signal_catcher(int signum) {
  switch (signum) {
    case SIGINT:
      reportErrAndExitProgram(NULL, EXIT_SUCCESS);
  }
}

int isSignalCatcherOn(int signum) {
  errno = 0;
  struct sigaction action;

  memset(&action, 0, sizeof action);
  sigemptyset(&action.sa_mask);

  action.sa_handler = &signal_catcher;
  action.sa_flags = SA_RESTART;
  if (sigaction(signum, &action, NULL) == -1) {
    return 0;
  }

  return 1;
}

void parseIgnoredFiles(int length, char *options[]) {
  int ignore_paths_start_idx = 0;
  for(int i = 0; i < length; i++) {
    if(strcmp(options[i], PROGRAM_OPTIONS.IGNORE_FILES) == 0 && i + 1 < length) {
      ignore_paths_start_idx = i + 1;
      break;
    }
  }

  int has_ignore_options = ignore_paths_start_idx > 0;
  int arr_length;
  if(!has_ignore_options) {
    arr_length = DEFAULT_IGNORES_LEN;
  } else {
    arr_length = (length - ignore_paths_start_idx) + DEFAULT_IGNORES_LEN;
  }

  IGNORE_DIRS = malloc((arr_length) * sizeof(char*));
  IGNORE_DIRS_LEN = arr_length;
  int ignore_default_dirs_start_idx = 0;
  for(int i = 0; i < arr_length; i++) {
    if(has_ignore_options && ignore_paths_start_idx < length) {
      IGNORE_DIRS[i] = options[ignore_paths_start_idx];
      ignore_paths_start_idx++;
    } else {
      IGNORE_DIRS[i] = DEFAULT_IGNORES[ignore_default_dirs_start_idx];
      ignore_default_dirs_start_idx++;
    }
  }
}

int main(int argc, char *argv[]) {
  if(argc < 2) {
    printf("Not enough arguments provided to run the program. Use --help to understand why");
    exit(EXIT_FAILURE);
  }

  if(strcmp(argv[1], PROGRAM_OPTIONS.HELP) == 0) {
    printf("THE HALPPPPP!!!!\n");
    return 0;
  }

  if(!isSignalCatcherOn(SIGINT)) {
    perror("An error occurred while adding signal handler");
    reportErrAndExitProgram(NULL, EXIT_FAILURE);
  }

  parseIgnoredFiles(argc, argv);
  
  Folders *folders = folderArrMake(5);
  getFoldersFromPath(folders, CURRENT_DIRECTORY);
  execCmdAndWatch(&argv[1], folders);
  
  return 0;
}

