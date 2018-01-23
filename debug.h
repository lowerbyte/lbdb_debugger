//#pragma once
#include <unistd.h>
#include <stdint.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include"lbdb_dwarf.h"
#include <sys/user.h>
#include <stdlib.h>

//struct which has defined all registers from sys/user.h
struct user_regs_struct regs;

//member of breakpoint list
//NOTE: parameter function is added, but not used
typedef struct deb{
  uint64_t data;
  uint64_t addr;
  int8_t *function;
  struct deb *next;
}breakpointsList;

void print_breakpoints_list(breakpointsList *head);
void add_to_breakpoints_list(breakpointsList *list, uint32_t address, uint32_t data, int8_t *function);
void remove_breakpoints_list(breakpointsList **head);
int debugged_program(const char* programName);
int debugger(pid_t childPID, functionList **head);
int split_string(int8_t **instr, const char *input);
int instruction_switch(int8_t **instr, functionList **head, pid_t childPID, breakpointsList **bHead);
uint64_t hash(uint8_t *str);
int make_a_break(functionList *head, int8_t *function, pid_t childPID, breakpointsList **bHead, int32_t *counter);
int continue_execution(functionList *head, pid_t childPID, int8_t *function, breakpointsList *bHead);
int run_the_program(pid_t childPID);
void remove_breakpoint(breakpointsList **head, breakpointsList *bHead, int32_t num);
int delete_breakpoint(breakpointsList **head, breakpointsList *bHead, int32_t breakpoint_number, pid_t childPID, int32_t *counter);
void remove_last_breakpoint(breakpointsList **head, breakpointsList *bHead, int32_t num);
void print_registers(pid_t childPID);
