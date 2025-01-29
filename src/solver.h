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
  struct Layer {
    int ordinal;
    String name, user_name;
    String type, material;
    float thickness, permittivity, loss_tangent; 
    struct Layer *prev, *next;
  }; *layer;
};

struct Setup {
  struct Section_Index index;
  float pad_to_mask_clearance;
  float solder_mask_min_width;
  float pad_to_paste_clearance;
  float pad_to_paste_clearance_ratio;
  struct Point *aux_axis_origin;
  struct Point *grid_origin;
  struct Plot_settings *pcbplotsettings;
  struct Stackup {
    struct Section_Index index;
    // Add
  } stackup;
  struct Properties{
    struct Property{
      String key, val;
      struct Property *next, *prev;
    } *property;
  } properties;
};

struct Nets{
  struct Section_Index index;
  struct Net {
    int ordinal;
    String name;
  } **net;
};

// https://dev-docs.kicad.org/en/file-formats/sexpr-intro/index.html#_footprint
struct Footprint {
  String library_link;
  struct Layer *layer;
  String uuid, description;
  struct at at;
  // Properties properties; // I think text properties are needed to be treated different
  String path, sheetname, sheetfile, attr;
  
};

struct at {
  float x, y, angle;
};

struct Point {
  float x;
  float y;
};

struct Size{
  float length, width;
};

struct Line {
  struct Point start, end;
  struct Layer *layer;
  struct Stroke {
    float width;
    String type;
  };
  String uuid;
  struct Line *prev, *next;
};

#define THRU_HOLE 1
#define SMD 2
#define CONNECT 4
#define NP_THRU_HOLE 8

#define CIRCLE 1
#define RECT 2
#define OVAL 4
#define TRAPEZOID 8
#define ROUNDRECT 16
#define CUSTOM 32

struct Pad {
  String num;
  int type, shape, function, type;
  struct at at;
  struct Size size;
  struct Layers layers;
  struct Net *net;
  String uuid;
};

struct XYZ{
  float x, y, z;
};

struct Model{
  struct XYZ offset, scale, rotate;
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
