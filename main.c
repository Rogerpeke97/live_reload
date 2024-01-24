#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <sys/dirent.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

typedef struct {
  char *path;
  uint64_t path_str_len;
  ino_t inode;
  time_t last_modified;
} FileMeta;

typedef struct {
  FileMeta *arr;
  uint64_t len;
  uint64_t cap;
} Files;

struct ProgramOptions {
  char *HELP;
  char *IGNORE_FILES;
} PROGRAM_OPTIONS = { "--help\0", "--ignore\0" };

pid_t PID_OF_CMD; 
int PROCESS_STATUS;
char CURRENT_DIRECTORY[] = ".";
const int DEFAULT_IGNORES_LEN = 2;
char *DEFAULT_IGNORES[DEFAULT_IGNORES_LEN] = { "..", "." };
char **IGNORE_DIRS;
int IGNORE_DIRS_LEN = 0;

Files* file_arr_make(uint64_t *len) {
  errno = 0;

  uint64_t cap = 2;
  if(len != NULL) {
    cap = *len * 2;
  }
  assert(cap > 0);

  FileMeta *file_arr= malloc(cap * sizeof(FileMeta));
  if (file_arr == NULL) {
    perror("Error allocating mem for FileMeta\n");
    return NULL;
  }

  for(int i = 0; i < cap; i++) {
    char *default_path_size = malloc(5 * sizeof(char));
    if(default_path_size == NULL) {
      perror("Error allocating mem of default string for FileMeta.path\n");
      return NULL;
    }

    file_arr[i].path = default_path_size;
    file_arr[i].path_str_len = 5;
  }

  Files *files = malloc(sizeof(Files));
  if (files == NULL) {
    perror("Error allocating mem for Files\n");
    return NULL;
  }

  files->arr = file_arr;
  files->len = 0;
  files->cap = cap;

  return files;
}

void file_arr_add(Files *file_arr, char *path_to_add, ino_t inode, time_t last_modified, uint64_t *idx) {
  uint64_t new_idx = file_arr->len;
  if(idx == NULL) {
    idx = &new_idx;
    assert(*idx == file_arr->len);
  }
  assert(idx != NULL);
  

  int is_valid_position = *idx <= file_arr->len; 
  if (!is_valid_position) {
   return; 
  }

  int has_to_increase_cap = *idx > file_arr->cap - 1;
  int has_to_increase_len = *idx == file_arr->len;

  if(has_to_increase_cap) {
    int new_cap = file_arr->cap * 2;
    FileMeta *new_file_arr = malloc(new_cap * sizeof(FileMeta));
    if(new_file_arr == NULL) {
      perror("Error allocating mem for FileMeta\n");
      return;
    }

    FileMeta *old_file_arr = file_arr->arr;
    for(int i = 0; i < new_cap; i++) {
      if(i < file_arr->len) {
        new_file_arr[i].path = old_file_arr[i].path;
        new_file_arr[i].path_str_len = old_file_arr[i].path_str_len;
        new_file_arr[i].last_modified = old_file_arr[i].last_modified;
        new_file_arr[i].inode = old_file_arr[i].inode;
      } else {
        new_file_arr[i].path = NULL;
        new_file_arr[i].path_str_len = 0;
        new_file_arr[i].last_modified = time(NULL);
        new_file_arr[i].inode = 0; 
      }
    }

    file_arr->arr = new_file_arr;
    file_arr->cap = new_cap; 
    free(old_file_arr);
  }
  if(has_to_increase_len) {
    file_arr->len = file_arr->len + 1;
  }
  
  int path_to_add_len = strlen(path_to_add) + 1; //NULL CHAR
  int has_to_increase_char_size = file_arr->arr[*idx].path_str_len < path_to_add_len;
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
    new_path[new_path_len - 1] = '\0';

    char *prev_allocd_str = file_arr->arr[*idx].path;

    file_arr->arr[*idx].path = new_path;
    file_arr->arr[*idx].path_str_len = new_path_len;
    file_arr->arr[*idx].inode = inode;
    file_arr->arr[*idx].last_modified = last_modified;

    free(prev_allocd_str);
    return;
  }

  for(int i = 0; i < path_to_add_len; i++) {
    if(i == path_to_add_len - 1) {
      file_arr->arr[*idx].path[i] = '\0';
      break;
    }
    file_arr->arr[*idx].path[i] = path_to_add[i];
  }

  file_arr->arr[*idx].inode = inode;
  file_arr->arr[*idx].last_modified = last_modified;
}

void file_arr_reset(Files *files) {
  for(int i = 0; i < files->len; i++) {
    for(int j = 0; j < files->arr[i].path_str_len; j++) {
      if(j == files->arr[i].path_str_len - 1) {
        files->arr[i].path[j] = '\0';
        break;
      }
      files->arr[i].path[j] = ' ';
    }
    files->arr[i].inode = 0;
    files->arr[i].last_modified = 0;
  }

  files->len = 0;
}

int is_invalid_path(char *path) {
  int path_len = strlen(path); 

  for(int i = 0; i < path_len; i++) {
    if(path_len + 1 < path_len && path[i] == '/' && path[i + 1] == '/') {
      printf("Invalid path. Check root path\n");
      exit(EXIT_FAILURE);
    }
  }

  return 1;
}

int is_ignored_path(char *path) {
  for(int i = 0; i < IGNORE_DIRS_LEN; i++) {
    if(strcmp(path, IGNORE_DIRS[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

Files *getFilePaths(Files *file_paths, char *root) {
  DIR *dir_to_get_files;
  struct dirent *found_dir;
  dir_to_get_files = opendir(root);

  if (dir_to_get_files) {
    while ((found_dir = readdir(dir_to_get_files)) != NULL) {
      if(found_dir->d_type == DT_DIR){
        if(is_ignored_path(found_dir->d_name)){
          continue;
        } else {
          char full_path[strlen(root) + strlen(found_dir->d_name) + (1 * sizeof(char))];
          sprintf(full_path, "%s/%s", root, found_dir->d_name);
          puts(full_path);
          is_invalid_path(full_path);
          getFilePaths(file_paths, full_path);
        }
      }
      if(found_dir->d_type == DT_REG && !is_ignored_path(found_dir->d_name)) {
        char full_path[strlen(root) + strlen(found_dir->d_name) + (1 * sizeof(char))];
        sprintf(full_path, "%s/%s", root, found_dir->d_name);
        puts(full_path);
        is_invalid_path(full_path);
        file_arr_add(file_paths, full_path, found_dir->d_ino, time(NULL), NULL);
      }
    }
    closedir(dir_to_get_files);
  }


  return file_paths;
}

void exec_cmd(char *cmd[]) {
  errno = 0;
  PID_OF_CMD = fork(); 

  if(PID_OF_CMD == -1) {
    perror("Fork Failed\n");
    exit(EXIT_FAILURE);
  }
  if(PID_OF_CMD == 0) {
    setpgid(0, 0);
    execvp(cmd[0], cmd);
    perror("Error while executing command\n");
    printf("Executing command: %s\n", cmd[0]);
    exit(EXIT_FAILURE);
  }
}

void kill_pid_and_restart(char *cmd[], int *process_status) {
  errno = 0;
  if(PID_OF_CMD > 0) {
    //KILL GROUP WIHT -pid
    kill(-PID_OF_CMD, SIGTERM);
    waitpid(PID_OF_CMD, process_status, 0);
    exec_cmd(cmd);
    return;
  }

  printf("INVALID PID\n");
  exit(EXIT_FAILURE);
}

void exec_cmd_and_watch(char *cmd[], Files *file_paths) {
  errno = 0;
  int stat_result = -1;
  struct stat file_stat;
  uint64_t file_curr_idx = 0;
  exec_cmd(cmd);

  if(PID_OF_CMD > 0) {
    while(1) {
      stat_result = stat(file_paths->arr[file_curr_idx].path, &file_stat);
      if(stat_result < 0) {
        perror("Error while getting inode file info\n");
        if(errno == ENOENT) {
          file_arr_reset(file_paths);
          getFilePaths(file_paths, CURRENT_DIRECTORY); 
          kill_pid_and_restart(cmd, &PROCESS_STATUS);
          file_curr_idx = 0;
          continue;
        }

        kill(-PID_OF_CMD, SIGTERM);
        waitpid(PID_OF_CMD, &PROCESS_STATUS, 0);
        break;
      }

      if(file_paths->arr[file_curr_idx].last_modified < file_stat.st_mtimespec.tv_sec) {
        kill_pid_and_restart(cmd, &PROCESS_STATUS);
      }

      file_paths->arr[file_curr_idx].last_modified = file_stat.st_mtimespec.tv_sec;

      if(file_curr_idx + 1 < file_paths->len) {
        file_curr_idx++;
      } else {
        file_curr_idx = 0;
      }
    } 
  }
}

void signal_catcher(int signum) {
  switch (signum) {
    case SIGINT:
      printf("Killing PID %d and exiting program \n", PID_OF_CMD);
      kill(-PID_OF_CMD, SIGTERM);
      waitpid(PID_OF_CMD, &PROCESS_STATUS, 0);
      exit(EXIT_SUCCESS);
  }
}

int is_signal_catcher_on(int signum) {
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
  int ignore_paths_start_idx = -1;
  for(int i = 0; i < length; i++) {
    if(strcmp(options[i], PROGRAM_OPTIONS.IGNORE_FILES) == 0 && i + 1 < length) {
      ignore_paths_start_idx = i + 1;
      break;
    }
  }

  if(ignore_paths_start_idx == -1) {
    return;
  }

  int arr_length = (length - ignore_paths_start_idx) + DEFAULT_IGNORES_LEN;
  IGNORE_DIRS = malloc((arr_length) * sizeof(char*));
  IGNORE_DIRS_LEN = arr_length;
  int ignore_default_dirs_start_idx = 0;
  for(int i = 0; i < arr_length; i++) {
    if(ignore_paths_start_idx < length) {
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

  if(!is_signal_catcher_on(SIGINT)) {
    perror("An error occurred while adding signal handler\n");
    exit(EXIT_FAILURE);
  }

  parseIgnoredFiles(argc, argv);
  
  Files *file_paths = file_arr_make(NULL);
  getFilePaths(file_paths, CURRENT_DIRECTORY);
  exec_cmd_and_watch(&argv[1], file_paths);
  
  return 0;
}

