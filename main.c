#include <stdio.h>
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
  String *str_arr = malloc((len * 2) * sizeof(String));
  if (str_arr == NULL) {
    return NULL;
  }

  char str_val[] = "HALLO BRAH";

  for(int i = 0; i < len * 2; i++) {
    char *default_str = malloc((strlen(str_val) * 2) * sizeof(char));
    for(int j = 0; j < strlen(str_val); j++) {
      default_str[j] = str_val[j];
    }
    String str_default = { default_str, strlen(str_val) * 2 };
    memcpy(&str_arr[i], &str_default, sizeof(String));
  }

  StrArray *arr_struct = (StrArray*)malloc(sizeof(StrArray));
  if (arr_struct == NULL) {
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




/* void int_arr_add(struct IntArray *arr_to_modify, int num_to_add, int position) {
  int is_enough_len = arr_to_modify->len + 1 <= arr_to_modify->cap;
  if(!is_enough_len) {
    arr_to_modify->int_arr = realloc(arr_to_modify->int_arr, ((arr_to_modify->len + 1) * 2) * sizeof(int));
    arr_to_modify->cap = (arr_to_modify->len + 1) * 2;
    arr_to_modify->len = arr_to_modify->len + 1;
  }
  
  int can_add_to_arr = position <= (arr_to_modify->len - 1);

  if(can_add_to_arr) {
    arr_to_modify->int_arr[position] = num_to_add;
  }

}; */

/* void int_arr_remove(struct IntArray *arr_to_modify, int position) {
  int is_position_to_remove_valid = position <= arr_to_modify->len - 1;
  if (is_position_to_remove_valid) {
    for(int i = 0; i < arr_to_modify->len - 1; i++) {
      if (i == position && i + 1 <= arr_to_modify->len - 1) {
        arr_to_modify->int_arr[i] = arr_to_modify->int_arr[i + 1];
      }
    }
    arr_to_modify->len = arr_to_modify->len - 1;
  }
} */

int main() {
  int cap = 10;
  StrArray *str_arr = str_arr_make(4, &cap);
  char *should_be_able_to_add_this = "asdj jasd jasd s djas djas djas djas djas djasdjasdjasjfh asjdh ajsd ajsdh ajsd ";

  printf("str to modify before %s\n", str_arr->arr[7].value);

  str_arr_add(str_arr, should_be_able_to_add_this, 7);

  printf("str to modify after %s\n", str_arr->arr[7].value);
  return 0;
}

