#include "solver.h"

static void print_pcb();

struct Board *pcb;

int main(int argc, char **argv){
  pcb = malloc(sizeof(struct Board));
  
  if (argc < 2){
    printf("No file specified\n");
    return EXIT_FAILURE;
  }
  token_table_init();
  open_pcb(argv[1]);
  
  //print_pcb();

  free_pcb();

  return EXIT_SUCCESS;
}


void free_pcb(){
  free(pcb->header.generator.chars);
  free(pcb->header.generator_version.chars);
  free(pcb->page.paper.chars);
  free(pcb);
}

static void print_pcb(){
  //printf();
}