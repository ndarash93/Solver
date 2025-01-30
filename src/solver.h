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

struct Layer {
  int ordinal;
  String name, user_name;
  String type, material;
  float thickness, permittivity, loss_tangent; 
  struct Layer *prev, *next;
};

struct Layers {
  struct Section_Index index;
  struct Layer  *layer;
};

struct Stackup{
  struct Section_Index index;
  // Add
}; 

struct Property{
  String key;
  union {
    String sval;
    float fval;
  } val;
  struct Property *next, *prev;
};

struct Setup {
  struct Section_Index index;
  float pad_to_mask_clearance;
  float solder_mask_min_width;
  float pad_to_paste_clearance;
  float pad_to_paste_clearance_ratio;
  struct Point *aux_axis_origin;
  struct Point *grid_origin;
  struct Stackup stackup;
  struct Property *properties;
  struct Plot_settings {
    struct Section_Index index;
    struct Property *properties;
  } pcbplotparams;
};

struct Net {
  int ordinal;
  String name;
};

struct Nets{
  struct Section_Index index;
  uint32_t count;
  struct Net *net; 
};

// https://dev-docs.kicad.org/en/file-formats/sexpr-intro/index.html#_footprint
struct Footprint {
  struct Section_Index index;
  String library_link;
  struct Layer *layer;
  String uuid, description;
  struct at at;
  // Properties properties; // I think text properties are needed to be treated different
  struct Footprint_Property{
    struct Property *property;
    struct at at;
    struct Layer *layer;
    String uuid; 
    struct Footprint_Property *next;
  } *properties;
  String path, sheetname, sheetfile, attr;
  struct Line *fp_lines;
  struct Pad *pads;
};

struct at {
  float x, y, angle;
};

struct Point {
  float x;
  float y;
};

struct XYZ{
  float x, y, z;
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
  struct Pad *next, *prev;
};

struct Model{
  struct XYZ offset, scale, rotate;
};

struct File_Buffer {
  String buffer;
  uint32_t index;
};

struct Graphic {
  struct Section_Index index;
};

struct Image{
  struct Section_Index index;
};

struct Tracks{
  struct Section_Index index;
};

struct Segment{
  struct Point start, end;
  float width;
  struct Layer *layer;
  struct Net *net;
  String uuid;
};

struct Arc{
  struct Point start, end, width, mid;
  struct Layer *layer;
  struct Net *net;
  String uuid;
};

struct Via{
  struct at at;
  float size, drill;
  struct Layers layers;
  struct Net *net;
  String uuid;
};

struct Zones{
  struct Section_Index index;
  struct Zone *zone;
};

struct Zone {
  struct Net net;
  struct Layer layer;
  String uuid;
  uint32_t priority;
  struct Point *points;
  struct Zone *next, *prev;
};

extern struct Board {
  // Buffer
  struct File_Buffer file_buffer;
  int opens;

  // Kicad PCB
  int kicad_pcb;

  // Sections
  struct Header header;
  struct General general;
  struct Page page;
  struct Layers layers;
  struct Setup setup;
  struct Nets nets;
  struct Footprints *footprints;
  struct Graphic *graphics;
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
