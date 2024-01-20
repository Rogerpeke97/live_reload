#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  char *value;
  uint64_t str_len;
} String;

typedef struct {
  String *arr;
  uint64_t len;
  uint64_t cap;
} StrArray;

StrArray* str_arr_make(uint64_t len, uint64_t cap) {
  errno = 0;
  String *str_arr = malloc((len * 2) * sizeof(String));
  if (str_arr == NULL) {
    perror("Error allocating mem for String\n");
    return NULL;
  }

  for(int i = 0; i < len * 2; i++) {
    char *default_str = malloc((20 * 2) * sizeof(char));
    if(default_str == NULL) {
      perror("Error allocating mem of default string for String.value\n");
      return NULL;
    }

    String str_default = { default_str, 20 * 2 };
    memcpy(&str_arr[i], &str_default, sizeof(String));
  }

  StrArray *arr_struct = (StrArray*)malloc(sizeof(StrArray));
  if (arr_struct == NULL) {
    perror("Error allocating mem for StrArray\n");
    return NULL;
  }

  arr_struct->arr = str_arr;
  arr_struct->len = len;
  arr_struct->cap = (len * 2) - 1;


  return arr_struct;
}

void str_arr_add(StrArray *str_arr, char *str_to_add, uint64_t *idx) {
  int has_to_increase_len = 0;
  uint64_t new_idx = str_arr->len;
  if(idx == NULL) {
    idx = &new_idx;
    has_to_increase_len = 1;
  }
  assert(idx != NULL);
  assert(*idx == str_arr->len);
  

  int is_valid_position = *idx <= str_arr->len; 
  if (!is_valid_position) {
   return; 
  }

  if(has_to_increase_len) {
    String *new_str_arr = malloc((str_arr->len * 2) * sizeof(String));
    if(new_str_arr == NULL) {
      perror("Error allocating mem for String\n");
      return;
    }
    String *old_str_arr = str_arr->arr;
    for(int i = 0; i < str_arr->len; i++) {
      new_str_arr[i].value = old_str_arr[i].value;
      new_str_arr[i].str_len = old_str_arr[i].str_len;
    }

    str_arr->arr = new_str_arr;
    str_arr->len = str_arr->len + 1;
    str_arr->cap = str_arr->len * 2;
    free(old_str_arr);
  }
  
  int str_to_add_len = strlen(str_to_add);
  int has_enough_size_to_alloc_str = str_arr->arr[*idx].str_len >= str_to_add_len; 
  if(has_enough_size_to_alloc_str) {
    char *new_str = malloc((str_to_add_len * 2) * sizeof(char));
    if(new_str == NULL) {
      perror("Error allocating mem for new String.value size\n");
      return;
    }

    for(int i = 0; i < str_to_add_len; i++) {
      new_str[i] = str_to_add[i];
    }
    char *prev_allocd_str = str_arr->arr[*idx].value;

    str_arr->arr[*idx].value = new_str;
    str_arr->arr[*idx].str_len = str_to_add_len * 2;

    free(prev_allocd_str);
    return;
  }

  for(int i = 0; i < str_to_add_len; i++) {
    str_arr->arr[*idx].value[i] = str_to_add[i];
  }
}


void str_remove(StrArray *str_arr, uint64_t position) {
  int is_pos_out_of_bounds = position < str_arr->len;
  if(is_pos_out_of_bounds) {
    return;
  }
  
  char *str_mem_to_release = str_arr->arr[position].value;
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

StrArray *getFilePaths(char *root) {
  DIR *dir_to_get_files;
  struct dirent *found_dir;
  dir_to_get_files = opendir(root);
  StrArray *file_paths = str_arr_make(2, 3);

  if (dir_to_get_files) {
      int times = 0;
      while ((found_dir = readdir(dir_to_get_files)) != NULL) {
          // printf("%s\n", found_dir->d_name); 
          // if(times == 1) {
          //   break;
          // }
          str_arr_add(file_paths, found_dir->d_name, NULL);
          times++;
      }
      closedir(dir_to_get_files);
  }


  return file_paths;
}

int main() {
  // StrArray *str_arr = str_arr_make(4, 10);
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
  
  StrArray *file_paths = getFilePaths("../hot_reload");

  for(int i = 0; i < file_paths->len; i++) {
    printf("FILE %d IS %s\n", i, file_paths->arr[i].value);
  }
  return 0;
}

