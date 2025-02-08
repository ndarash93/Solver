#include "solver.h"


void print_layer(){
  printf("(ORDINAL: %d; CANONICAL_NAME: \"%s\"; TYPE: %d; USER_NAME: \"%s\")\n", pcb->footprints->layer->ordinal, pcb->footprints->layer->canonical_name.chars, pcb->footprints->layer->type, pcb->footprints->layer->user_name.chars);
}

void print_footprints(){
  for(struct Footprint *footprint = pcb->footprints; footprint; footprint = footprint->next){
    printf("(footprint \"%s\"\n", footprint->library_link.chars);
  }
  printf("(ORDINAL: %d; CANONICAL_NAME: \"%s\"; TYPE: %d; USER_NAME: \"%s\")\n", pcb->footprints->layer->ordinal, pcb->footprints->layer->canonical_name.chars, pcb->footprints->layer->type, pcb->footprints->layer->user_name.chars);    
}