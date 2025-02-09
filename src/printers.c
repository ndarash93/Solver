#include "solver.h"

void print_layer(){
  printf("(ORDINAL: %d; CANONICAL_NAME: \"%s\"; TYPE: %d; USER_NAME: \"%s\")\n", pcb->footprints->layer->ordinal, pcb->footprints->layer->canonical_name.chars, pcb->footprints->layer->type, pcb->footprints->layer->user_name.chars);
}

void print_footprints(struct Footprint *footprint){
  while(footprint){
    printf("(footprint \"%s\"\n", footprint->library_link.chars);
    printf("(layer %s)\n", footprint->layer->canonical_name.chars);
    printf("(uuid \"%s\")\n", footprint->uuid.chars);
    printf("(at %f %f %f)\n", footprint->at.x, footprint->at.y, footprint->at.angle);
    printf("(descr \"%s\")\n", footprint->description.chars);
    print_footprint_properties(footprint->properties);
    footprint = footprint->next;
  }
  printf("(ORDINAL: %d; CANONICAL_NAME: \"%s\"; TYPE: %d; USER_NAME: \"%s\")\n", pcb->footprints->layer->ordinal, pcb->footprints->layer->canonical_name.chars, pcb->footprints->layer->type, pcb->footprints->layer->user_name.chars);    
}

void print_properties(struct Property *property){
  while(property){
    printf("(property %s %s)\n", property->key.chars, property->val.chars);
    property = property->next;
  }
}

void print_footprint_properties(struct Footprint_Property *property){
  while(property){
    printf("(property %s %s\n", property->property->key.chars, property->property->val.chars);
    printf("(at %f %f %f)\n", property->at.x, property->at.y, property->at.angle);
    printf("(layer %s)\n", property-> layer ? property->layer->canonical_name.chars : (char *)"NULL");
    printf("(uuid %s)\n", property->uuid.chars);
    printf(")\n");
    property = property->next;
  }
}