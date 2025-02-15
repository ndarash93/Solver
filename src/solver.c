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
  open_pcb(argv[1]);
  
  //print_footprints(pcb->footprints);
  //print_tracks(pcb->tracks);
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
  
  a();
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
  a();
  while(net){
    struct Net *temp = net;
    net = net->next;
    free(temp->name.chars);
    free(temp);
  }
  a();
  while(footprint){
    struct Footprint *temp = footprint;
    footprint = footprint->next;
      if(temp->properties){
        struct Footprint_Property *property = temp->properties;
        struct Footprint_Property *temp_property;
        while(property){ 
          temp_property = property;
          property = property->next;
          free(temp_property->uuid.chars);
          free(temp_property->property->key.chars);
          free(temp_property->property->val.chars);
          free(temp_property->property);
          free(temp_property);
        }
      }
      a();
      if(temp->fp_lines){
        struct Line  *line = temp->fp_lines;
        struct Line  *temp_line;
        while(line){
          temp_line = line;
          line = line->next;
          free(temp_line->uuid.chars);
          free(temp_line);
        }
      }
      a();
      if(temp->pads){
        struct Pad *pad = temp->pads;
        struct Pad * temp_pad;
        while(pad){
          a();
          temp_pad = pad;
          a();
          printf("Pad: %p\n", pad);
          pad = pad->next;
          a();
          free(temp_pad->num.chars);
          a();
          free(temp_pad->uuid.chars);
          a();
          free(temp_pad->layers);
          a();
          free(temp_pad);
        }
      }
      a();
      if(temp->model){
        free(temp->model->model.chars);
        free(temp->model);
      }
    free(temp->attr.chars);
    free(temp->description.chars);
    free(temp->library_link.chars);
    free(temp->path.chars);
    free(temp->uuid.chars);
    free(temp);
  }
  a();
  while(track){
    struct Track *temp = track;
    track = track->next;
    if(temp->type == TRACK_TYPE_ARC){
      free(temp->uuid.chars);
    }else if(temp->type == TRACK_TYPE_SEG){
      free(temp->uuid.chars);
    }else if(temp->type == TRACK_TYPE_VIA){
      free(temp->uuid.chars);
      free(temp->track.via.layers);
    }
    free(temp);
  }
  a();
  while(zone){
    struct Zone *temp = zone;
    zone = zone->next;
    free(temp->uuid.chars);
    free(temp);
  }
  free(pcb);
}