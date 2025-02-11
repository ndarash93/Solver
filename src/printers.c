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
    print_line(footprint->fp_lines);
    print_pad(footprint->pads);
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

void print_line(struct Line *line){
  while(line){
    printf("fp_line\n");
    printf("(start %f %f)\n", line->start.x, line->start.y);
    printf("(end %f %f)\n", line->end.x, line->end.y);
    printf("(layer %s)\n", line->layer ? line->layer->canonical_name.chars : (char *)"NULL");
    printf("(uuid %s)\n)\n", line->uuid.chars);
    line = line->next;
  }
}

void print_pad(struct Pad *pad){
  printf("Pad: %p\n", pad);
  while(pad){
    printf("(pad \"%s\" %d %d\n", pad->num.chars, pad->type, pad->shape);
    printf("(at %f %f)\n", pad->at.x, pad->at.y);
    printf("(size %f %f)\n", pad->size.width, pad->size.height);
    printf("layers");
    for(int i = 0; i < pad->layer_count; i++){
      printf(" \"%s\"", pad->layers[i]->canonical_name.chars);
    }
    printf(")\n");
    printf("net %d \"%s\")\n", pad->net->ordinal, pad->net->name.chars);
    printf("uuid \"%s\")\n)\n", pad->uuid.chars);
    pad = pad->next;
  }
}

/*(pad "1" smd circle
			(at 0 0)
			(size 0.5 0.5)
			(layers "F.Cu" "F.Mask")
			(net 56 "/Power/VOUT4")
			(pinfunction "1")
			(pintype "passive")
			(solder_mask_margin 0.08)
			(thermal_bridge_angle 0)
			(uuid "206ded5f-75ba-4846-91c5-bb7ba245a4d4")
		)*/