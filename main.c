#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


typedef struct {
  char *path;
  uint64_t path_str_len;
  ino_t inode;
  time_t last_modified;
} FileMeta;

//TODO: ADD TESTS. ALLOC A SHITTON
typedef struct {
  FileMeta *arr;
  uint64_t len;
  uint64_t cap;
} Files;

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

    FileMeta file_placeholder = { default_path_size, 5 };
    memcpy(&file_arr[i], &file_placeholder, sizeof(FileMeta));
  }

  Files *files = (Files*)malloc(sizeof(Files));
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
    FileMeta *new_file_arr = malloc((file_arr->cap * 2) * sizeof(FileMeta));
    if(new_file_arr == NULL) {
      perror("Error allocating mem for FileMeta\n");
      return;
    }
    FileMeta *old_file_arr = file_arr->arr;
    for(int i = 0; i < file_arr->len; i++) {
      new_file_arr[i].path = old_file_arr[i].path;
      new_file_arr[i].path_str_len = old_file_arr[i].path_str_len;
    }

    file_arr->arr = new_file_arr;
    file_arr->cap = file_arr->cap * 2;
    free(old_file_arr);
  }
  if(has_to_increase_len) {
    file_arr->len = file_arr->len + 1;
  }
  
  int path_to_add_len = strlen(path_to_add);
  int has_to_increase_char_size = file_arr->arr[*idx].path_str_len < path_to_add_len; 
  if(has_to_increase_char_size) {
    char *new_str = malloc((path_to_add_len * 2) * sizeof(char));
    if(new_str == NULL) {
      perror("Error allocating mem for new FileMeta.path size\n");
      return;
    }

    for(int i = 0; i < path_to_add_len; i++) {
      new_str[i] = path_to_add[i];
    }
    char *prev_allocd_str = file_arr->arr[*idx].path;

    file_arr->arr[*idx].path = new_str;
    file_arr->arr[*idx].path_str_len = path_to_add_len * 2;
    file_arr->arr[*idx].inode = inode;
    file_arr->arr[*idx].last_modified = last_modified;

    free(prev_allocd_str);
    return;
  }

  for(int i = 0; i < path_to_add_len; i++) {
    file_arr->arr[*idx].path[i] = path_to_add[i];
  }
  file_arr->arr[*idx].inode = inode;
  file_arr->arr[*idx].last_modified = last_modified;
}


void str_remove(Files *str_arr, uint64_t position) {
  int is_pos_out_of_bounds = position < str_arr->len;
  if(is_pos_out_of_bounds) {
    return;
  }
  
  char *str_mem_to_release = str_arr->arr[position].path;
  for(int i = position; i < str_arr->len - 1; i++) {
    str_arr->arr[i] = str_arr->arr[i + 1];
  }

  str_arr->len = str_arr->len - 1;
}

#ifdef __linux__
int file_updated() {
  return 1;
}
#endif

#ifdef __APPLE__
int file_updated() {

  return 0;
}
#endif

void updateFilePaths(Files *file_paths) {
    
}

void watchFiles(Files *file_paths) {
  int stat_result = -1;
  struct stat file_stat;
  uint64_t file_curr_idx = 0;
  while(1) {
    stat_result = stat(file_paths->arr[file_curr_idx].path, &file_stat);
    if(stat_result < 0) {
      perror("Error while getting inode file info\n");
      break;
    }

    file_paths->arr[file_curr_idx].last_modified = file_stat.st_mtimespec.tv_sec;
    printf("FILE IDX %llu, NAME %s, WAS LAST MODIFIED %jd seconds ago", file_curr_idx, file_paths->arr[file_curr_idx].path, file_stat.st_mtimespec.tv_sec);

    if(file_curr_idx < file_paths->len) {
      file_curr_idx++;
    } else {
      file_curr_idx = 0;
    }
  } 
}

Files *getFilePaths(Files *file_paths, char *root) {
  DIR *dir_to_get_files;
  struct dirent *found_dir;
  dir_to_get_files = opendir(root);

  if (dir_to_get_files) {
    while ((found_dir = readdir(dir_to_get_files)) != NULL) {
      if(found_dir->d_type == DT_DIR){
        if(strcmp(found_dir->d_name, ".") && strcmp(found_dir->d_name, "..")) {
          char full_path[strlen(root) + strlen(found_dir->d_name) + (1 * sizeof(char))];
          sprintf(full_path, "%s/%s", root, found_dir->d_name);
          puts(full_path);
          getFilePaths(file_paths, full_path);
        } else {
          continue;
        }
      }
      if(found_dir->d_type == DT_REG) {
        char full_path[strlen(root) + strlen(found_dir->d_name) + (1 * sizeof(char))];
        sprintf(full_path, "%s/%s", root, found_dir->d_name);
        puts(full_path);
        file_arr_add(file_paths, full_path, found_dir->d_ino, time(NULL), NULL);
      }
    }
    closedir(dir_to_get_files);
  }


  return file_paths;
}

int main() {
  // if(str_arr == NULL) {
  //   if(errno) {
  //     printf("errno: %d\n", errno);
  //     return 1;
  //   }
  // }
  // printf("PID: %d\n", getpid());

  // struct stat file_stat;
  // int stat_result = stat("./main.c", &file_stat);
  // if(stat_result < 0) {
  //   perror("Error while getting inode file info\n");
  //   return 1;
  // }
  //
  // printf("Inode Number: %ld\n", (long) file_stat.st_ino);
  
  Files *file_paths = file_arr_make(NULL);
  getFilePaths(file_paths, "../hot_reload");
  
  watchFiles(file_paths); 
  return 0;
}

