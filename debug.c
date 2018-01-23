#include "debug.h"
#include<stdlib.h>
#include<string.h>

#define INPUT_SIZE 50
#define TAB_ROW 10
#define STRING_LEN 10
#define BREAK 210707980106
#define C 177672
#define DEL 193489338
#define R 177687
#define I 177678

int8_t is_program_running = 1;
int32_t cnt = 0;   
int32_t breakpoint_counter = 0;  //for number of breakpoints


//https://stackoverflow.com/questions/7666509/hash-function-for-string
uint64_t hash(uint8_t *str){
    uint64_t hash = 5381;
    int32_t c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

//function to print breakpoints
void print_breakpoints_list(breakpointsList *head){
  breakpointsList *f = head;
  while(f != NULL){
    printf("Breakpoint address: 0x%x, breakpoint data: 0x%x\n", f->addr, f->data, f->function);
    f = f->next;
  }
}

//adding breakpoints
void add_to_breakpoints_list(breakpointsList *f, uint32_t address, uint32_t data, int8_t *function){
  breakpointsList *new;
  while(f->next != NULL){
    f = f->next;
  }
  new = malloc(sizeof(breakpointsList));
  new->addr = address;
  new->data = data;
  new->function = function;
  new->next = NULL;
  f->next = new;
}

//removing whole breakpoints list
void remove_breakpoints_list(breakpointsList **head){
  breakpointsList *tmp, *q;
  tmp = *head;
  while(tmp != NULL){
    q = tmp->next;
    free(tmp);
    tmp = q;
  }
  free(tmp);
}

//remove single breakpoint
void remove_breakpoint(breakpointsList **head, breakpointsList *bHead, int32_t num){

  breakpointsList *p;
  p = (*head);
  while((*head)->next != bHead) p=p->next;
  p->next = bHead->next;
  free(bHead);
}

//remove breakpoint from the end of the linked list
void remove_last_breakpoint(breakpointsList **head, breakpointsList *bHead, int32_t num){

  breakpointsList *p;
  p = (*head);
  if(p){
    if(p->next){
      while(p->next->next) p = p->next;
      free(p->next);
      p->next = NULL;
    }
    else{
      free(p);
      (*head) = NULL;
    }
  }
}

//split user input, eg. break main ---> break, main
int split_string(int8_t **instr, const char *input){
  int32_t j = 0, ctr = 0;  
  for(uint32_t i = 0; i < strlen(input); i++){
      if(input[i] == 0x20 || input[i] == '\0' || input[i] == 0xA)
        continue;
      instr[ctr][j] = input[i];
      if(input[i+1] == 0x20 || input[i+1] == '\0'  || input[i+1] == 0xA){
        instr[ctr][j+1] = '\0';
        ctr++;
        j = 0;
        continue;
      }
      j++;
    }
  return 0;
}

//function responsible for making breakpoints (injecting 0xcc byte)
int make_a_break(functionList *head, int8_t *function, pid_t childPID, breakpointsList **bHead, int32_t *counter){
  uint32_t addr;

  while(head != NULL){
    if(strcmp(head->function_name,function)==0){
      addr = head->address;
      printf("Breakpoint set at function: %s\n", head->function_name);
      break;
    }
    head = head->next;
  }

  uint64_t data = ptrace(PTRACE_PEEKTEXT, childPID, (void*)addr, 0);
  printf("Breakpoint set at: 0x%08x (0x%08x)\n", addr, data);
  
  add_to_breakpoints_list((*bHead), addr, data, function);

  uint64_t trap = (data & ~0xFF) | 0xCC; //last byte as architecture is little endian
  ptrace(PTRACE_POKETEXT, childPID, (void*)addr, (void*)trap);

  uint64_t data2 = ptrace(PTRACE_PEEKTEXT, childPID, (void*)addr, 0);
  printf("0x%08x --> 0x%08x\n", data, data2);
  
  (*counter)++;
  printf("Number of breakpoints: %i\n", (*counter));
  return 0;
}

//function removing the 0xcc byte
//NOTE: I do not restore the breakpoint after continuation of the program
int continue_execution(functionList *head, pid_t childPID, int8_t *function, breakpointsList *bHead){
  int32_t wait_status;

  for(int32_t i = 0; i < cnt; i++){
    printf("BHead addr: 0x%08x BHead data: 0x%08x\n", bHead->addr, bHead->data);
    bHead = bHead->next;
  }
  
  cnt++;  
  
  int64_t data = ptrace(PTRACE_PEEKTEXT, childPID, (void*)bHead->addr, 0);
  data = data & ~0xff;
  uint64_t disable_trap = (bHead->data & 0xFF) | data;
  
  int64_t data2 = ptrace(PTRACE_PEEKTEXT, childPID, (void*)bHead->addr, 0);
  printf("Before break: 0x%08x (0x%08x)\n", bHead->addr, data2);
  ptrace(PTRACE_POKETEXT, childPID, (void*)bHead->addr, (void*)disable_trap);
  data2 = ptrace(PTRACE_PEEKTEXT, childPID, (void*)bHead->addr, 0);
  printf("After break: 0x%08x (0x%08x)\n", bHead->addr, data2);
  ptrace(PTRACE_GETREGS, childPID, 0, &regs);
  regs.rip = bHead->addr;
  ptrace(PTRACE_SETREGS, childPID, 0, &regs);
  ptrace(PTRACE_CONT, childPID, 0, 0);
  
  wait(&wait_status);

  if (WIFSTOPPED(wait_status)) { 
    printf("Child got a signal: %s\n", strsignal(WSTOPSIG(wait_status)));
  }
  else if(wait_status == 0){ 
    puts("Program ended.");
    is_program_running = 1;
    return 2;
  }
  else{
    printf("Child got a signal: %s\n", strsignal(WSTOPSIG(wait_status)));
    return -1;
  }

  return 0;
}

//Let's run the execution!
int run_the_program(childPID){
  ptrace(PTRACE_CONT, childPID, 0, 0);
  int32_t wait_status;
  
  wait(&wait_status);
  if (WIFSTOPPED(wait_status)) {
    ptrace(PTRACE_GETREGS, childPID, 0, &regs);
    printf("Child got a signal: %s on 0x%08x\n", strsignal(WSTOPSIG(wait_status)), regs.rip);
  }
  else if(wait_status == 0){ 
    puts("Program ended.");
    is_program_running = 1;
    return 2;
  }
  else{
    printf("Child got a signal: %s\n", strsignal(WSTOPSIG(wait_status)));
    return -1;
  }

  return 0;
}

//delete chosen breakpoint
//NOTE: if you have two breakpoints like:
//1 --> main
//2 --> add
//when you delete the first one with 'del 1', the second breakpoint will become
//first! So to delete it you will have to write 'del 1' one more time.
int delete_breakpoint(breakpointsList **head, breakpointsList *bHead, int32_t breakpoint_number, pid_t childPID, int32_t *counter){
  for(int32_t i = 1; i < breakpoint_number; i++){
    bHead = bHead->next;
  }

  int64_t data = ptrace(PTRACE_PEEKTEXT, childPID, (void*)bHead->addr, 0);
  data = data & ~0xff;
  
  uint64_t disable_trap = (bHead->data & 0xFF) | data;
  ptrace(PTRACE_POKETEXT, childPID, (void*)bHead->addr, (void*)disable_trap);
  
  if(breakpoint_number == 1)
    remove_breakpoint(head, bHead, breakpoint_number);
  else if(breakpoint_number > 1 && breakpoint_number < breakpoint_counter)
    remove_breakpoint(head, bHead, breakpoint_number);
  else remove_last_breakpoint(head, bHead, breakpoint_number);
  (*counter)--;
  cnt--;
  return 0;
}

//printing actual state of the registers
void print_registers(pid_t childPID){
  ptrace(PTRACE_GETREGS, childPID, 0, &regs);
  printf("rip: 0x%08x\n"
      "rbp: 0x%08x\n"
      "rsp: 0x%08x\n"
      "rax: 0x%08x\n"
      "rcx: 0x%08x\n"
      "rdx: 0x%08x\n"
      "rsi: 0x%08x\n"
      "rdi: 0x%08x\n", regs.rip, regs.rbp, regs.rsp, regs.rax, regs.rcx, regs.rdx, regs.rsi, regs.rdi);
  
}

//executing the program we want to debug
int debugged_program(const char* programName){
  printf("Target started. Will run '%s'\n", programName);
  ptrace(PTRACE_TRACEME, 0, NULL, NULL);
  execl(programName, programName, NULL);

  return 0;
}

//switch depending on user input
int instruction_switch(int8_t **instr, functionList **head, pid_t childPID, breakpointsList **bHead){
  int32_t is_function_name = -1;
  switch(hash(instr[0])){
    case BREAK: ;
      functionList *head2 = (*head)->next;
      while(head2 != NULL){
        if(strcmp(head2->function_name, instr[1])==0){
         is_function_name = make_a_break(head2, instr[1], childPID, bHead, &breakpoint_counter);
         break;
        }
        head2 = head2->next;
      }
      if(is_function_name)
        printf("There is no function named %s\n", instr[1]);
      break;
    case C:{
      printf("Is program running: %i\n", is_program_running);
      if(is_program_running != 0){
        puts("The program is not being run.");
        break;
      }
      puts("Continue execution!");
      int32_t ret = continue_execution((*head)->next, childPID, instr[1], (*bHead)->next);
      if(ret == 2) return 3;
      break;
           }
    case DEL:{
      char *end;
      long i = strtol(instr[1], &end, 10);
      if(i < 1 || i > breakpoint_counter){
        printf("There is no breakpoint with number: %i\n", i);
        break;
      }
      printf("Deleted breakpoint: %i\n", i);
      delete_breakpoint(bHead, (*bHead)->next, i, childPID, &breakpoint_counter);
      break;
      }
    case R:
      if(!is_program_running){
        puts("The program is running already!");
        break;
    }
      printf("Is program running: %i\n", is_program_running);
      puts("Run the program");
      is_program_running = run_the_program(childPID);
      if(is_program_running == 2) return 3;
      break;
    case I:
      if(*instr[1] == 'b'){
        print_breakpoints_list((*bHead)->next);
      }
      else if(*instr[1] == 'r'){
        print_registers(childPID);
      }
      else{
        puts("Usage: \"i b\" <-- to view breakpoints\n\"i r\" <-- to view current register values");
      }
      break;
    default:
      printf("There is no instruction named %s\n", instr[0]);
      break;
  }
  return 0;
}

//our main debugging function - our debugger
int debugger(pid_t childPID, functionList **head){
  int8_t input[INPUT_SIZE] = {0};
  int8_t *instr_tab[TAB_ROW] = {NULL};
  int8_t **instr = &(*instr_tab);
  int32_t wait_status, ret;
  uint32_t counter = 0;
  breakpointsList *break_head;
  break_head = (breakpointsList*)malloc(sizeof(breakpointsList));
  break_head->next = NULL;
  
  for(int i = 0; i < TAB_ROW; i++){
    if((instr_tab[i] = (int8_t*)malloc(sizeof(int8_t) * INPUT_SIZE)) == NULL){
      puts("Unable to allocate memory for string table!");
      return -1;
    }
  }
  puts("Debugger started!\n");

  waitpid(-1, &wait_status, 0);

  while(WIFSTOPPED(wait_status)){
    printf(">>");
    fgets(input, sizeof(int8_t)*INPUT_SIZE, stdin);
    split_string(instr, (const char*)input);
    ret = instruction_switch(instr, head, childPID, &break_head);
    if(ret == 3) break;
  }
 
  for(int32_t i = 0 ; i < TAB_ROW; i++)
        free(instr_tab[i]);
  remove_breakpoints_list(&break_head);
  
  return 0;
}
