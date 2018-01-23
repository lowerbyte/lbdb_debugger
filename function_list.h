#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>

typedef struct structure{
  uint32_t address;
  const char* function_name;
  struct structure *next;
} functionList;

void print_function_list(functionList *head);
void add_to_function_list(functionList *f, int64_t adress, const char* function_name);
void remove_function_list(functionList **head);
