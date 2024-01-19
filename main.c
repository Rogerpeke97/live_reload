#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *value;
  uint64_t len;
} String;

typedef struct {
  String *arr;
  uint64_t len;
  uint64_t cap;
} StrArray;

StrArray* str_arr_make(uint64_t len, void *cap) {
  errno = 0;
  String *str_arr = malloc((len * 2) * sizeof(String));
  if (str_arr == NULL) {
    perror("Error allocating mem for String\n");
    return NULL;
  }

  char str_val[] = "HALLO BRAH";

  for(int i = 0; i < len * 2; i++) {
    char *default_str = malloc((strlen(str_val) * 2) * sizeof(char));
    if(default_str == NULL) {
      perror("Error allocating mem of default string for String.value\n");
      return NULL;
    }

    for(int j = 0; j < strlen(str_val); j++) {
      default_str[j] = str_val[j];
    }
    String str_default = { default_str, strlen(str_val) * 2 };
    memcpy(&str_arr[i], &str_default, sizeof(String));
  }

  StrArray *arr_struct = (StrArray*)malloc(sizeof(StrArray));
  if (arr_struct == NULL) {
    perror("Error allocating mem for StrArray\n");
    return NULL;
  }

  arr_struct->arr = str_arr;
  arr_struct->len = len;
  arr_struct->cap = len * 2;


  return arr_struct;
}

void str_arr_add(StrArray *str_arr, char *str_to_add, uint64_t position) {
  int is_valid_position = position <= str_arr->arr->len + 1;
  if (!is_valid_position) {
   return; 
  }
  
  int str_to_add_len = strlen(str_to_add);
  int has_enough_size_to_alloc_str = str_arr->arr[position].len >= str_to_add_len; 
  if(!has_enough_size_to_alloc_str) {
    char *new_str = malloc((str_to_add_len * 2) * sizeof(char));
    if(new_str == NULL) {
      perror("Error allocating mem for new String.value size\n");
      return;
    }

    for(int i = 0; i < strlen(str_to_add); i++) {
      new_str[i] = str_to_add[i];
    }

    char *prev_allocd_str = str_arr->arr[position].value;

    str_arr->arr[position].value = new_str;
    free(prev_allocd_str);
    return;
  }

  for(int i = 0; i < str_to_add_len; i++) {
    str_arr->arr[position].value[i] = str_to_add[i];
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



int main() {
  int cap = 10;
  StrArray *str_arr = str_arr_make(4, &cap);
  if(str_arr == NULL) {
    if(errno) {
      printf("errno: %d\n", errno);
      return 1;
    }
  }
  char *should_be_able_to_add_this = "asdj jasd jasd s djas djas djas djas djas djasdjasdjasjfh asjdh ajsd ajsdh ajsd ";

  printf("str to modify before %s\n", str_arr->arr[7].value);

  str_arr_add(str_arr, should_be_able_to_add_this, 7);

  printf("str to modify after %s\n", str_arr->arr[7].value);
  return 0;
}

