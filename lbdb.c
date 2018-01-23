#include"debug.h"

//gcc -Wall -Wextra lbdb.c debug.c lbdb_dwarf.c function_list.c -fsanitize=address -ldwarf -lelf -o main


int main(int argc, char** argv){
  pid_t child;
  functionList *head;
  head = (functionList*) malloc (sizeof(functionList));
  head->next = NULL;
  const char* elf_name;
  Elf *elf = NULL;
  Dwarf_Error err;
  Dwarf_Debug dbg; //An instance of the Dwarf_Debug type is created as 
                   //a result of a successful call to dwarf_init()
  int fd = -1; //file descriptor. At initialization has to be lower than 0, as 0 is stdin.
  int res = -2;

  if(argc < 2){
    fprintf(stderr, "Usage: ./lbdb_debugger <filename>");
    remove_function_list(&head);
    return 1;
  }

  elf_name = argv[1];

  if((fd = open(elf_name, O_RDONLY)) < 0){  //open() (fnctl.h) to establish the connection 
                                            //between a file and a file descriptor
    puts("Oh no! The file failed on open!");
    remove_function_list(&head);
    return 1;
  }
   
  if (elf_version(EV_CURRENT) == EV_NONE){ 
	  puts("elf_version return with error");
    remove_function_list(&head);
    return 1;
  }

  if((elf = elf_begin(fd, ELF_C_READ, (Elf*)0)) == 0){
    puts("elf_begin failed.");
    printf("Value of elf pointer: %p\n", elf);
    remove_function_list(&head);
    return 1;
  }

  if((res = dwarf_elf_init(elf, DW_DLC_READ, 0, 0, &dbg, &err)) != DW_DLV_OK){
    if(res == DW_DLV_ERROR){
      puts("dwarf_init returned DW_DLV_ERROR");
      /*
       * An Dwarf_Error returned from dwarf_init() or dwarf_elf_init() in case 
       * of a failure cannot be free’d using dwarf_dealloc(). The only way to 
       * free the Dwarf_Error from either of those calls is to use free(3) 
       * directly. Every Dwarf_Error must be free’d by dwarf_dealloc() except 
       * those returned by dwarf_init() or dwarf_elf_init().
       */
      free(err);
      remove_function_list(&head);
      return 2;
    }
    else{
      //If DW_DLV_NO_ENTRY is returned no pointer value in the interface returns a value.
      puts("dwarf_init returned DW_DLV_NO_ENTRY");
      remove_function_list(&head);
      return 3;
    }
  }

  get_function_tag(dbg, err, &head);
  print_function_list(head->next);

  child = fork();
  if(child == 0)
    debugged_program(argv[1]);
  else
    debugger(child, &head);

  if(dwarf_finish(dbg, &err) != DW_DLV_OK){
    printf("dwarf_finish return with error: %i\n", res);
    remove_function_list(&head);    
    return 5;
  }
  puts("Finishing dwarf");
  /*
   * Because int dwarf_init() opens an Elf descriptor on its fd and 
   * dwarf_finish() does not close that descriptor, an app should 
   * use dwarf_get_elf and should call elf_end with the pointer 
   * returned thru the Elf** handle created by int dwarf_init().
   * */
  elf_end(elf);
  puts("Cleaning done!");

  remove_function_list(&head);

  return 0;
}
