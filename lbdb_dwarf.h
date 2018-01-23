#include<libdwarf.h>
#include<libelf.h>
#include<dwarf.h>
#include<fcntl.h> 
#include<elf.h>
#include"function_list.h"
#include<stdbool.h>

int get_function_name(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Error err, functionList **head);
int get_function_tag(Dwarf_Debug dbg, Dwarf_Error err, functionList **head);
