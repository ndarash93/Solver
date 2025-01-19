#include "solver.h"

struct Board *pcb;

int main(int argc, char **argv){
  pcb = malloc(sizeof(struct Board));
  
  printf("Got Here %s\n", argv[1]);
  if (argc < 2){
    printf("No file specified\n");
    return EXIT_FAILURE;
  }
  printf("Got Here 2\n");
  open_pcb(argv[1]);
  token_table_init();
  
  free(pcb);

  return EXIT_SUCCESS;
}

