#include "solver.h"

struct Board *pcb;

int main(int argc, char **argv){
  pcb = malloc(sizeof(struct Board));
  
  if (argc < 2){
    printf("No file specified\n");
    return EXIT_FAILURE;
  }
  token_table_init();
  open_pcb(argv[1]);
  
  free_pcb();

  return EXIT_SUCCESS;
}


void free_pcb(){
  free(pcb->generator.chars);
  free(pcb->generator_version.chars);
  free(pcb->paper.chars);
  free(pcb);
}
