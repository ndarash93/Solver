#include "solver.h"

struct Board *pcb;

int main(int argc, char **argv){
  
  printf("Got Here %s\n", argv[1]);
  if (argc < 2){
    printf("No file specified\n");
    return EXIT_FAILURE;
  }
  printf("Got Here 2\n");
  open_pcb(argv[1]);
  token_table_init();
  

  /*
  file = fopen(argv[1], "r");
  if (file == NULL){
    perror("Error opening file");
    return EXIT_FAILURE;
  }

  printf("File contents:\n");
    while(1){
      ch = fgetc(file);
      if(ch == EOF)
        break;
      printf("%c", ch);
  }
  printf("\n");
  fclose(file);
*/
  return EXIT_SUCCESS;
}

