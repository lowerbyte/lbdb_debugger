#include<libdwarf.h>
#include<libelf.h>
#include<dwarf.h>
#include<fcntl.h> 
#include<elf.h>
#include"function_list.h"
#include<stdbool.h>

//function to retrieve address and name of each function in debugged file
int get_function_name(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Error err, functionList **head){

  Dwarf_Half attr = 0x3; //DWA_AT_name = 0x3 from DWARF documentation
  Dwarf_Attribute return_attr = NULL;
  Dwarf_Addr return_lowpc;
  char *f_name;
  int32_t res = -2;

  if((res = dwarf_attr(die, attr, &return_attr, &err)) != DW_DLV_OK){
    if(res == DW_DLV_NO_ENTRY){
      puts("No attribute: name");
      return 10;
    }
    else{
      puts("dwarf_attr failed!");
      dwarf_dealloc(dbg, err, DW_DLA_ERROR);
      return 11;
    }
  }

  if(dwarf_formstring(return_attr, &f_name, &err) != DW_DLV_OK){
      puts("dwarf_formstring failed!");
      dwarf_dealloc(dbg,err,DW_DLA_ERROR);
      return 12;
  }

  if((res = dwarf_lowpc(die, &return_lowpc, &err)) != DW_DLV_OK){
    if(res == DW_DLV_NO_ENTRY){
      puts("No attribute: lowpc");
      return 13;
    }
    else{
      puts("dwarf_lowpc failed!");
      dwarf_dealloc(dbg, err, DW_DLA_ERROR);
      return 14;
    }
  }
  add_to_function_list(*head, return_lowpc, f_name); 

  dwarf_dealloc(dbg, f_name, DW_DLA_STRING);
  dwarf_dealloc(dbg, return_attr, DW_DLA_ATTR);

  return 0;
}

//getting function tag. If tag == DW_TAG_subprogram this what we want
int get_function_tag(Dwarf_Debug dbg, Dwarf_Error err, functionList **head){
  
  Dwarf_Unsigned cu_header_length, abbrev_size, next_cu_header;
  Dwarf_Half version_stamp, address_size;
  Dwarf_Die die = NULL, sib_die, kid_die;
  int res, next_cu;

  if((next_cu = dwarf_next_cu_header(dbg, &cu_header_length, &version_stamp, &abbrev_size, &address_size, &next_cu_header, &err)) == DW_DLV_ERROR){
    puts("dwarf_next_cu_header failed!");
    dwarf_dealloc(dbg, err, DW_DLA_ERROR);
    return 6;
  }
    
  while(next_cu != DW_DLV_NO_ENTRY){
  /*
   * If die is NULL, the Dwarf_Die descriptor of the first die 
   * in the compilation-unit is returned. This die has the 
   * DW_TAG_compile_unit tag.
  */
    if(dwarf_siblingof(dbg, die, &sib_die, &err) == DW_DLV_ERROR){
      puts("dwarf_siblingof failed!");
      dwarf_dealloc(dbg,err,DW_DLA_ERROR);
      return 7;      
    }


    if(dwarf_child(sib_die, &kid_die, &err) != DW_DLV_OK){
      puts("dwarf_child failed!");
      dwarf_dealloc(dbg,err,DW_DLA_ERROR);
      return 8;
    }

    while(true){
      Dwarf_Half tag;
      if(dwarf_tag(kid_die, &tag, &err) != DW_DLV_OK){
        puts("dwarf_tag failed!");
        dwarf_dealloc(dbg,err,DW_DLA_ERROR);
        return 9;
      }

      if(tag == DW_TAG_subprogram){
        get_function_name(dbg, kid_die, err, head);

      }
      res = dwarf_siblingof(dbg, kid_die, &kid_die, &err);

      if (res == DW_DLV_ERROR)
        puts("Error getting sibling of DIE\n");
      else if (res == DW_DLV_NO_ENTRY)
        break;
    }

    next_cu = dwarf_next_cu_header(dbg, &cu_header_length, &version_stamp, &abbrev_size, &address_size, &next_cu_header, &err);
  }

  dwarf_dealloc(dbg, die, DW_DLA_DIE);
  dwarf_dealloc(dbg, kid_die, DW_DLA_ATTR);
  dwarf_dealloc(dbg, sib_die, DW_DLA_ATTR);

  return 0;
}

/* ONLY FOR TESTING PURPOSE
int main(int argc, char* argv[]){
  
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
    return 1;
  }

  elf_name = argv[1];

  if((fd = open(elf_name, O_RDONLY)) < 0){  //open() (fnctl.h) to establish the connection 
                                            //between a file and a file descriptor
    puts("Oh no! The file failed on open!");
    return 1;
  }
   
  if (elf_version(EV_CURRENT) == EV_NONE){ 
	  puts("elf_version return with error");
    return 1;
  }

  if((elf = elf_begin(fd, ELF_C_READ, (Elf*)0)) == 0){
    puts("elf_begin failed.");
    printf("Value of elf pointer: %p\n", elf);
    return 1;
  }

  if((res = dwarf_elf_init(elf, DW_DLC_READ, 0, 0, &dbg, &err)) != DW_DLV_OK){
    if(res == DW_DLV_ERROR){
      puts("dwarf_init returned DW_DLV_ERROR");
      *
       * An Dwarf_Error returned from dwarf_init() or dwarf_elf_init() in case 
       * of a failure cannot be free’d using dwarf_dealloc(). The only way to 
       * free the Dwarf_Error from either of those calls is to use free(3) 
       * directly. Every Dwarf_Error must be free’d by dwarf_dealloc() except 
       * those returned by dwarf_init() or dwarf_elf_init().
       *
      free(err);
      return 2;
    }
    else{
      //If DW_DLV_NO_ENTRY is returned no pointer value in the interface returns a value.
      puts("dwarf_init returned DW_DLV_NO_ENTRY");
      return 3;
    }
  }
  
  puts("Checking");
  printf("elf adress 1: %p %p \n", &elf, elf);
  if(dwarf_get_elf(dbg, &elf, &err) != DW_DLV_OK){
    printf("dwarf_get_elf return with error: %i\n", res);
    return 4;
  }else puts("Getting_elf");
  printf("elf adress 2: %p %p \n", &elf, elf);
 
  get_function_tag(dbg, err, &head);
  print_function_list(head->next);
  remove_function_list(&head);


  if(dwarf_finish(dbg, &err) != DW_DLV_OK){
    printf("dwarf_finish return with error: %i\n", res);
    return 5;
  }
  puts("Finishing dwarf");
  *
   * Because int dwarf_init() opens an Elf descriptor on its fd and 
   * dwarf_finish() does not close that descriptor, an app should 
   * use dwarf_get_elf and should call elf_end with the pointer 
   * returned thru the Elf** handle created by int dwarf_init().
   * *
  elf_end(elf);
  puts("Cleaning done!");
  return 0;

}*/
