#include "solver.h"

static void print_pcb();

struct Board *pcb;

int main(int argc, char **argv){
  //pcb = malloc(sizeof(struct Board));
  pcb = calloc(1, sizeof(struct Board));
  if (argc < 2){
    printf("No file specified\n");
    return EXIT_FAILURE;
  }
  token_table_init();
  printf("1\n");
  open_pcb(argv[1]);
  
  //print_pcb();
  //for (struct Net *temp = pcb->nets; temp; temp = temp->next){
  //    printf("(\n\tNet: %p,\n\tOrdinal: %d,\n\tName: \"%s\",\n\tnext: %p,\n\tprev: %p\n)\n", temp, temp->ordinal, temp->name.chars, temp->next, temp->prev);
  //  }
  free_pcb();

  return EXIT_SUCCESS;
}


void free_pcb(){
  struct Layer *temp_layer, *layer = pcb->layers.layer;
  struct Net *temp_net, *net = pcb->nets;
  free(pcb->header.generator.chars);
  free(pcb->header.generator_version.chars);
  free(pcb->page.paper.chars);
  /*while(layer){
    temp = layer;
    layer = layer->next;
    free(temp);
  }*/
  FREE(temp_layer, layer);
  FREE(temp_net, net);

  free(pcb);
}

static void print_pcb(){
  //printf();
}