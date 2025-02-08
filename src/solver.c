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
  free_pcb();

  return EXIT_SUCCESS;
}


void free_pcb(){
  free(pcb->header.generator.chars);
  free(pcb->header.generator_version.chars);
  free(pcb->page.paper.chars);
  for(struct Layer *temp = NULL, *layer = pcb->layers.layer; layer; temp = layer, layer = layer->next){
    //printf("TEMP: %p, LAYER: %p, NEXT: %p\n", temp, layer, layer->next);
    free(layer->canonical_name.chars);
    free(layer->user_name.chars);
    free(layer->material.chars);
    free(temp);
  }
  for(struct Net *temp = NULL, *net = pcb->nets; net; temp = net, net = net->next){
    free(net->name.chars);
    free(temp);
  }
  for(struct Footprint *temp = NULL, *footprint = pcb->footprints; footprint; temp = footprint, footprint = footprint->next){
    free(footprint->attr.chars);
    free(footprint->description.chars);
    free(footprint->library_link.chars);
    free(footprint->path.chars);
    free(footprint->uuid.chars);
    free(temp);
  }
  for(struct Track *temp = NULL, *track = pcb->tracks; track; temp = track, track = track->next){
    if(track->type == TRACK_TYPE_ARC){
      free(track->track.arc->uuid.chars);
    }else if(track->type == TRACK_TYPE_SEG){
      free(track->track.segment->uuid.chars);
    }else if(track->type == TRACK_TYPE_VIA){
      free(track->track.via->uuid.chars);
    }
    free(temp);
  }
  for(struct Zone *temp = NULL, *zone = pcb->zones; zone; temp = zone, zone = zone->next){
    free(zone->uuid.chars);
    free(temp);
  }

  free(pcb);
}
