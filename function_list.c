#include"function_list.h"

//print list with function and addresses from DWARF info
void print_function_list(functionList *head){
  functionList *f = head;
  while(f != NULL){
    printf("Function name: %s, address: %x\n", f->function_name, f->address);
    f = f->next;
  }
}

//add function and address from DWARF info
void add_to_function_list(functionList *f, int64_t address, const char* function_name){
  functionList *new;
  while(f->next != NULL){
    f = f->next;
  }
  new = malloc(sizeof(functionList));
  new->address = address;
  new->function_name = function_name;
  new->next = NULL;
  f->next = new;
}

//remove list
void remove_function_list(functionList **head){
  functionList *tmp, *q;
  tmp = *head;
  while(tmp != NULL){
    q = tmp->next;
    free(tmp);
    tmp = q;
  }
  //*head = tmp;
  free(tmp);

}
