#include <string.h>
#include <stdint.h>


#ifdef DEBUG 
// Debug
#include "debug.h"

#define malloc(size) mem_track_malloc(size, __LINE__, __func__, __FILE__)
#define calloc(num, size) mem_track_calloc(num, size, __LINE__, __func__, __FILE__)
#define realloc(ptr, size) mem_track_realloc(ptr, size, __LINE__, __func__, __FILE__)
#define free(ptr) mem_track_free(ptr)

#define a() debug_helper(__LINE__, __func__, __FILE__)

#else

#define a() //

#include <stdlib.h>
#include <stdio.h>

#endif





#define UNUSED 0
#define OPEN 1
#define CLOSED -1

#define SECTION_UNSET 0
#define SECTION_SET 1
#define SECTION_CLOSED 2

#define LAYER_TYPE_JUMPER 1
#define LAYER_TYPE_MIXED 2
#define LAYER_TYPE_POWER 3
#define LAYER_TYPE_SIGNAL 4
#define LAYER_TYPE_USER 5

#define TRACK_TYPE_SEG 1
#define TRACK_TYPE_VIA 2
#define TRACK_TYPE_ARC 3

#define ERROR -1
#define SUCCESS 1

#define TRUE 1
#define FALSE 0

#define THRU_HOLE 1
#define SMD 2
#define CONNECT 3
#define NP_THRU_HOLE 4

#define CIRCLE 1
#define RECT 2
#define OVAL 3
#define TRAPEZOID 4
#define ROUNDRECT 5
#define CUSTOM 6

#define NO_CONNECT 0
#define THERMAL_RELIEF 1
#define SOLID_FILL 2

typedef struct {
    char *chars;
    uint64_t length;
} String;


struct Section_Index{
  int set;
  uint64_t section_start;
  uint64_t section_end;
  uint64_t start_line, end_line;
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
  struct Section_Index index;
  int ordinal, type;
  String canonical_name, user_name;
  String material;
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
  String val;
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
  struct Section_Index index;
  int ordinal;
  String name;
  struct Net *next, *prev;
};

struct Nets{
  struct Section_Index index;
  uint32_t count;
  struct Net *net; 
};

struct at {
  float x, y, angle;
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
    struct Section_Index index;
    struct Property *property;
    struct at at;
    struct Layer *layer;
    String uuid; 
    struct Footprint_Property *next, *prev;
  } *properties;
  String path, sheetname, sheetfile, attr;
  struct Text *fp_texts;
  struct Line *fp_lines;
  struct Rect *fp_rects;
  struct Circule *fp_circles;
  struct Arc *fp_arcs;
  struct Polygon *fp_poly;
  struct Curve *fp_curve;
  struct Pad *pads;
  struct Footprint *prev, *next;
  struct Model *model;
};

struct Point {
  float x;
  float y;
};

struct XYZ{
  float x, y, z;
};

struct Size{
  float width, height;
};

struct Line {
  struct Section_Index index;
  struct Point start, end;
  struct Layer *layer;
  struct Stroke {
    float width;
    String type;
  } stroke;
  String uuid;
  struct Line *prev, *next;
};

struct Pad {
  struct Section_Index index;
  String num;
  int type, shape, function, layer_count;
  struct at at;
  struct Size size;
  struct Layer **layers;
  struct Net *net;
  String uuid;
  struct Pad *next, *prev;
};

struct Offset{
  struct Section_Index index;
  struct XYZ xyz;
};

struct Scale{
  struct Section_Index index;
  struct XYZ xyz;
};

struct Rotate{
  struct Section_Index index;
  struct XYZ xyz;
};

struct Model{
  struct Section_Index index;
  String model;
  struct Offset offset;
  struct Scale scale;
  struct Rotate rotate;
};

struct File_Buffer {
  String buffer;
  uint64_t index;
};

struct Graphic {
  struct Section_Index index;
  struct Text *gr_text;
  struct Text_Box *gr_text_box;
  struct Rect *gr_rect;
  struct Circle *gr_circle;
  struct Arc *gr_arc;
  struct Polygon *polygon;
  struct Curve *curve;
};

struct Text{
  struct Section_Index index;
  struct Point pos;
  struct Layer *layer;
  String uuid;
};

struct Text_Box{
  struct Section_Index index;
  struct Point pos;
  struct Layer *layer;
  String uuid;
};

struct Rect{
  struct Section_Index index;
  struct Point start, end;
  struct Layer *layer;
  float width;
  int fill;
  String uuid;
  struct Graphical_Rect *next, *prev;
};

struct Circle{
  struct Section_Index index;
  struct Point center, end;                                 
  struct Layer *layer;
  float width;
  int fill;
  String uuid;
};

/*  Duplicate struct except track arcs has a net pointer
    can be NULL for all other uses except tracks
struct Arc{
  struct Point start, mid, end;
  struct Layer *layer;
  float width;
  String uuid;
};
*/

struct Polygon{
  struct Section_Index index;
  struct Point **points;
  struct Layer *layer;
  float width;
  int fill;
  String uuid;
};

struct Curve{
  struct Section_Index index;
  struct Point **points;
  struct Layer *layer;
  float width;
  String uuid;
};

struct Drill{
  struct Section_Index index;
  int oval;
  float diameter, width;
  struct Point offset;
};

struct Images{
  struct Section_Index index;
};

struct Groups{
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
  struct Point start, end, mid;
  float width;
  struct Layer *layer;
  struct Net *net;
  String uuid;
};

struct Via{
  struct at at;
  float size;
  struct Drill drill;
  struct Layer **layers;
  struct Net *net;
  String uuid;
};

struct Track{
  struct Section_Index index;
  int type;
  union {
    struct Segment segment;
    struct Arc arc;
    struct Via via;
  } track;
  struct Track *prev, *next;
};

/*
struct Track{
  struct Section_Index index;
  int type;
  struct Point start, end, mid;

  struct Track *prev, *next;
};
*/

struct Zone {
  struct Section_Index index;
  struct Net *net;
  struct Layer *layer;
  String uuid;
  uint32_t priority;
  int hatch_style, connect_pads, fill;
  float min_thickness, hatch_pitch;
  struct Point **points, **filled_points;
  struct Zone *next, *prev;
};

extern struct Board {
  // Buffer
  struct File_Buffer file_buffer;
  int opens;

  // Kicad PCB
  struct Section_Index kicad_pcb;

  // Sections
  struct Header header;
  struct General general;
  struct Page page;
  struct Layers layers;
  struct Setup setup;
  struct Net *nets;
  struct Footprint *footprints;
  struct Graphic graphics;
  struct Images images;
  struct Track *tracks;
  struct Zone *zones;
  struct Groups groups;
} *pcb;

// Solver
void free_pcb();  

// Parser
int open_pcb(const char *path);
void token_table_init();

// Utils
int string_compare(String _1, String _2);

// Printers
void print_layer();
void print_footprints(struct Footprint *footprint);
void print_properties(struct Property *property);
void print_footprint_properties(struct Footprint_Property *property);
void print_line(struct Line *line);
void print_pad(struct Pad *pad);
void print_model(struct Model *model);