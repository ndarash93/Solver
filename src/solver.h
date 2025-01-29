#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "utils.h"

#define ERROR -1
#define SUCCESS 1


#define UNUSED 0
#define OPEN 1
#define CLOSED -1

#define SECTION_SET 1
#define SECTION_UNSET 0

struct Section_Index{
  int set;
  uint64_t section_start;
  uint64_t section_end;
};

struct Header{
  struct Section_Index index;
  String generator, generator_version, version;
};

struct General {
  struct Section_Index index;
  float thickness;
  int legacy_teardrops;
};

struct Page {
  struct Section_Index index;
  String paper;
};

struct Layers {
  struct Section_Index index;
  int *layer;
};

struct Layer {
  int ordinal;
  String name, user_name;
  String type, material;
  float thickness, permittivity, loss_tangent; 
};

struct Setup {
  struct Section_Index index;
  float pad_to_mask_clearance;
  float solder_mask_min_width;
  float pad_to_paste_clearance;
  float pad_to_paste_clearance_ratio;
  struct Point *aux_axis_origin;
  struct Point *grid_origin;
  struct Plot_settings *plotsettings;
  struct Stackup {
    struct Section_Index index;
    // Add
  } stackup;
};

struct Properties{
  struct Property{
    String key, val;
    struct Property *next, *prev;
  };
};

struct Point {
  float x;
  float y;
};

struct Nets{
  struct Section_Index index;
  struct Net {
    int ordinal;
    String name;
    struct Net *prev, *next;
  };
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

struct File_Buffer {
  String buffer;
  uint32_t index;
};

extern struct Board {
  // Buffer
  struct File_Buffer file_buffer;
  int opens;

  // Kicad PCB
  int kicad_pcb;

  // Header
  struct Header header;
  struct General general;
  struct Page page;
  struct Layers layers;
  struct Setup setup;
  struct Properties properties;
  struct Nets nets;
  struct Footprints *footprints;
  struct Graphic *graphic;
  struct Images *images;
  struct Tracks *tracks;
  struct Zones *zones;
  struct Groups *groups;
} *pcb;

// Solver
void free_pcb();  

// Parser
int open_pcb(const char *path);
void token_table_init();
