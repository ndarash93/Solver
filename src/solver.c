#include "solver.h"

struct Board *pcb;

int main(int argc, char **argv){
  //pcb = malloc(sizeof(struct Board));
  printf("Got Here\n");
  pcb = calloc(1, sizeof(struct Board));
  printf("Got Here1\n");
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
  print_footprints(pcb->footprints);
  free_pcb();

  return EXIT_SUCCESS;
}


void free_pcb(){
  a();
  struct Layer *layer = pcb->layers.layer;
  struct Net *net = pcb->nets;
  struct Footprint *footprint = pcb->footprints;
  struct Track *track = pcb->tracks;
  struct Zone *zone = pcb->zones;
  

  free(pcb->header.version.chars);
  free(pcb->header.generator.chars);
  free(pcb->header.generator_version.chars);
  free(pcb->page.paper.chars);
  while(layer){
    struct Layer *temp = layer;
    layer = layer->next;
    free(temp->canonical_name.chars);
    free(temp->user_name.chars);
    free(temp->material.chars);
    free(temp);
  }
  while(net){
    struct Net *temp = net;
    net = net->next;
    free(temp->name.chars);
    free(temp);
  }
  while(footprint){
    struct Footprint *temp = footprint;
    footprint = footprint->next;
      if(temp->properties){
        struct Footprint_Property *property = temp->properties;
        while(property){
          struct Footprint_Property *temp_property = property;
          property = property->next;
          free(temp_property->uuid.chars);
          free(temp_property->property->key.chars);
          free(temp_property->property->val.chars);
          free(temp_property->property);
          free(temp_property);
        }
      }
    free(temp->attr.chars);
    free(temp->description.chars);
    free(temp->library_link.chars);
    free(temp->path.chars);
    free(temp->uuid.chars);
    free(temp);
  }
  while(track){
    struct Track *temp = track;
    track = track->next;
    if(temp->type == TRACK_TYPE_ARC){
      free(temp->track.arc->uuid.chars);
    }else if(temp->type == TRACK_TYPE_SEG){
      free(temp->track.segment->uuid.chars);
    }else if(temp->type == TRACK_TYPE_VIA){
      free(temp->track.via->uuid.chars);
    }
    free(temp);
  }
  while(zone){
    struct Zone *temp = zone;
    zone = zone->next;
    free(temp->uuid.chars);
    free(temp);
  }
  free(pcb);
}