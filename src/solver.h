#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define ERROR -1
#define SUCCESS 1


extern struct Board {
  int version;
  char *generator;
  char *generator_version;
  struct General *general;
  struct Layers *layers;
  struct Setup *setup;
  //struct Stackup stackup;
  struct Properties *properties;
  struct Nets *nets;
  struct Footprints *footprints;
  struct Graphic *graphic;
  struct Images *images;
  struct Tracks *tracks;
  struct Zones *zones;
  struct Groups *groups;
} *pcb;


struct General {
  float thickness;
  int legacy_teardrops;
};

struct Layers {
  int *layer;
};

struct Layer {
  int ordinal;
  char *name, *user_name;
  int type;
};

struct Setup {
  //void STACK_UP_SETTINGS s;
  float pad_to_mask_clearance;
  float solder_mask_min_width;
  float pad_to_paste_clearance;
  float pad_to_paste_clearance_ratio;
  struct Point *aux_axis_origin;
  struct Point *grid_origin;
  struct Plot_settings *plotsettings;
};

struct Point {
  float x;
  float y;
};

struct Net {
  int ordinal;
  char *name;
};

// https://dev-docs.kicad.org/en/file-formats/sexpr-intro/index.html#_footprint
struct Footprint {
  char *library_link;
  unsigned long long flags; // locked placed etc.
  struct Layer layer;
  int tedit;
  char *uuid, description;
  struct at *pos_id;
  char **tags;
};



struct at {
  float x, y, angle;
};

  
// Parser
int open_pcb(const char *path);
void token_table_init();
