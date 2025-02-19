#include "solver.h"

#define HASH_CAP 5000
#define TOKEN_SZ 300

#define BUFF pcb->file_buffer.buffer.chars
#define INDEX pcb->file_buffer.index
#define LENGTH pcb->file_buffer.buffer.length
#define CHAR pcb->file_buffer.buffer.chars[pcb->file_buffer.index]

#define COPY(token, index, buff) (token[(index++)] = BUFF[(buff++)])

#define PUSH(new, list) (list == NULL) ? (list = new, new->next = NULL) : (list->prev = new, new->next = list, list = new)

// Hash table
static int token_lookup(const char *token);
static unsigned long hash(const char *token);
static struct token *create_token(const char *key, int* (*handler)());
static struct table *create_table(int size);
static void free_token(struct token *token);
static void free_table(struct table *table);
static void insert(struct table *table, char* key, int* (*handler)());
static void *search_token(struct table *table, char* key);
static void print_table(struct table *table);
static void handle_collision(struct table *table, unsigned long index, struct token *token);
static void print_search(struct table *table, char *key);
static struct collision_list* allocate_list();
static struct collision_list* insert_collision(struct collision_list *list, struct token *token);
static struct token *remove_collision(struct collision_list *list);
static void free_list(struct collision_list *list);
static struct collision_list **create_overflow(struct table *table);
static void free_overflow(struct table *table);
static void table_delete(struct table *table, char *key);

// Parsing
static void parse_pcb(uint64_t start, uint64_t end);
static int parse_token(uint64_t start, uint64_t end, String *token);

// Handler prototype
static int *handle_version(uint64_t start, uint64_t end);
static int *handle_kicadpcb(uint64_t start, uint64_t end);
static int *handle_generator(uint64_t start, uint64_t end);
static int *handle_generator_version(uint64_t start, uint64_t end);
static int *handle_general(uint64_t start, uint64_t end);
static int *handle_thickness(uint64_t start, uint64_t end);
static int *handle_paper(uint64_t start, uint64_t end);
static int *handle_layers(uint64_t start, uint64_t end);
static int *handle_layer(uint64_t start, uint64_t end);
static int *handle_setup(uint64_t start, uint64_t end);
static int *handle_stackup(uint64_t start, uint64_t end);
static int *handle_pad_to_mask_clearance(uint64_t start, uint64_t end);
static int *handle_solder_mask_min_width(uint64_t start, uint64_t end);
static int *handle_pad_to_paste_clearance(uint64_t start, uint64_t end);
static int *handle_pad_to_paste_clearance_ratio(uint64_t start, uint64_t end);
static int *handle_pcbplotparams(uint64_t start, uint64_t end);
static int *handle_net(uint64_t start, uint64_t end);
static int *handle_footprint(uint64_t start, uint64_t end);
static int *handle_zone(uint64_t start, uint64_t end);
//static int *handle_track(uint64_t start, uint64_t end);
static int *handle_uuid(uint64_t start, uint64_t end);
static int *handle_property(uint64_t start, uint64_t end);
static int *handle_descr(uint64_t start, uint64_t end);
static int *handle_at(uint64_t start, uint64_t end);
static int *handle_fp_line(uint64_t start, uint64_t end);
static int *handle_start(uint64_t start, uint64_t end);
static int *handle_end(uint64_t start, uint64_t end);
static int *handle_pad(uint64_t start, uint64_t end);
static int *handle_size(uint64_t start, uint64_t end);
static int *handle_model(uint64_t start, uint64_t end);
static int *handle_offset(uint64_t start, uint64_t end);
static int *handle_scale(uint64_t start, uint64_t end);
static int *handle_rotate(uint64_t start, uint64_t end);
static int *handle_xyz(uint64_t start, uint64_t end);
static int *handle_via(uint64_t start, uint64_t end);
static int *handle_segment(uint64_t start, uint64_t end);
static int *handle_drill(uint64_t start, uint64_t end);
static int *handle_arc(uint64_t start, uint64_t end);
static int *handle_width(uint64_t start, uint64_t end);
static int *handle_material(uint64_t start, uint64_t end);
static int *handle_epsilon_r(uint64_t start, uint64_t end);
static int *handle_loss_tangent(uint64_t start, uint64_t end);
static int *handle_polygon(uint64_t start, uint64_t end);
static int *handle_filled_polygon(uint64_t start, uint64_t end);
static int *handle_pts(uint64_t start, uint64_t end);
static int *handle_xy(uint64_t start, uint64_t end);

// Handler Helpers
static int handle_quotes(uint64_t *start, uint64_t end, String *quote);
static void set_section_index(uint64_t start, uint64_t end, struct Section_Index *index);
static void handle_value_token(uint64_t *start, uint64_t end, String *token);
static struct Layer *find_layer(String name);
static struct Net *find_net(int ordinal);

struct token{
  char *key;
  int* (*handler)();
};

struct table{
  struct token **tokens;
  struct collision_list **overflow;
  int size, count;
};

struct collision_list{
  struct token *token;
  struct collision_list *next;
};

static struct table *tokens = NULL;

void token_table_init(){
  tokens = create_table(HASH_CAP);
  printf("Tokens %p\n", tokens);
  insert(tokens, (char *)"kicad_pcb", handle_kicadpcb);
  insert(tokens, (char *)"version", handle_version);
  insert(tokens, (char *)"generator", handle_generator);
  insert(tokens, (char *)"generator_version", handle_generator_version);
  insert(tokens, (char *)"general", handle_general);
  insert(tokens, (char *)"thickness", handle_thickness);
  insert(tokens, (char *)"paper", handle_paper);
  insert(tokens, (char *)"layers", handle_layers);
  insert(tokens, (char *)"layer", handle_layer);
  insert(tokens, (char *)"setup", handle_setup);
  insert(tokens, (char *)"stackup", handle_stackup);
  insert(tokens, (char *)"pad_to_mask_clearance", handle_pad_to_mask_clearance);
  insert(tokens, (char *)"solder_mask_min_width", handle_solder_mask_min_width);
  insert(tokens, (char *)"pad_to_paste_clearance", handle_pad_to_paste_clearance);
  insert(tokens, (char *)"pad_to_paste_clearance_ratio", handle_pad_to_paste_clearance_ratio);
  insert(tokens, (char *)"pcbplotparams", handle_pcbplotparams);
  insert(tokens, (char *)"net", handle_net);
  insert(tokens, (char *)"footprint", handle_footprint);
  insert(tokens, (char *)"zone", handle_zone);
  insert(tokens, (char *)"via", handle_via);
  insert(tokens, (char *)"segment", handle_segment);
  insert(tokens, (char *)"arc", handle_arc);
  insert(tokens, (char *)"uuid", handle_uuid);
  insert(tokens, (char *)"property", handle_property);
  insert(tokens, (char *)"descr", handle_descr);
  insert(tokens, (char *)"at", handle_at);
  insert(tokens, (char *)"fp_line", handle_fp_line);
  insert(tokens, (char *)"start", handle_start);
  insert(tokens, (char *)"end", handle_end);
  insert(tokens, (char *)"pad", handle_pad);
  insert(tokens, (char *)"size", handle_size);
  insert(tokens, (char *)"model", handle_model);
  insert(tokens, (char *)"offset", handle_offset);
  insert(tokens, (char *)"scale", handle_scale);
  insert(tokens, (char *)"rotate", handle_rotate);
  insert(tokens, (char *)"xyz", handle_xyz);
  insert(tokens, (char *)"width", handle_width);
  insert(tokens, (char *)"material", handle_material);
  insert(tokens, (char *)"epsilon_r", handle_epsilon_r);
  insert(tokens, (char *)"loss_tangent", handle_loss_tangent);
  insert(tokens, (char *)"polygon", handle_polygon);
  insert(tokens, (char *)"filled_polygon", handle_filled_polygon);
  insert(tokens, (char *)"pts", handle_pts);
  insert(tokens, (char *)"xy", handle_xy);
  //print_table(tokens);
}

int open_pcb(const char *path){
  long length, bytes_read = 0;
  char *buffer = NULL;
  FILE *file;

  printf("Opening file %s\n", path);
  file = fopen(path, "rb");

  if (file == NULL){
    perror("Error opening file");
    goto clean_up;
    return ERROR;
  }
  //pcb->file = file;
  fseek(file, 0, SEEK_END);
  length = ftell(file);
  fseek(file, 0, SEEK_SET);
  buffer = malloc(length * sizeof(char));

  if(buffer)
    bytes_read = fread(buffer, 1, length, file);
  fclose(file);
  if (bytes_read != length){
    printf("Read error");
    goto clean_up;
  }
  
  pcb->file_buffer.buffer.chars = buffer;
  pcb->file_buffer.buffer.length = length;
  pcb->file_buffer.index = 0;

  //index_sections();
  parse_pcb(0, 0);

clean_up:
  free(buffer);
  free_table(tokens);
  return SUCCESS;
}

static void parse_pcb(uint64_t start, uint64_t end){
  uint64_t opens = 0;
  uint64_t index = start, new_start, new_end;
  int *section_set = NULL;
  end = (end ? end : LENGTH);
  while(index <= end){
    if(BUFF[index] == '('){
      if(opens == 0){
        //printf("\n(");
        new_start = index;
      }
      opens++;
    }else if(BUFF[index] == ')'){
      opens--;
      if(opens == 0){
        new_end = index;
        //index = new_start;
        String token;
        if(parse_token(new_start, new_end, &token) == ERROR){
          //printf("ERROR\n");
        }
        int* (*handler)(uint64_t, uint64_t) = search_token(tokens, token.chars);
        if(handler){
          section_set = handler(new_start, new_end);
          
          //printf("SUCCESS\n");
        }
        
        //printf("OPENS: %ld\n", opens);
        //printf("%s", token.chars);
        free(token.chars);
        parse_pcb(new_start+1, new_end-1);
        //printf(")");
        if(section_set){
          //printf("SECTION UNSET\n");
          *section_set = SECTION_CLOSED;
        }
      }
    }
    index++;
  }
}

static int parse_token(uint64_t start, uint64_t end, String *token){
  //printf("Parse Token\n");
  int token_index = 0;
  char temp_token[TOKEN_SZ];
  if(BUFF[start++] != '('){
    //printf("Unexpected token");
    return ERROR;
  }
  while(start < end){
    if(BUFF[start] < 32){
      //line_count++;
      //while(BUFF[++start] != '('); // find next open
      start++;
      continue;
    }
    //else if(BUFF[start] == '\t'){
      //printf("Got Here\n");
    //} // Skip over tabs
    else if (BUFF[start] == ')'){
      //printf("Unexpected close");
      break;
    }else if(BUFF[start] == '('){
      //printf("Very unexpected error\n");
      break;
      //return ERROR;
    }else if (BUFF[start] == ' '){
      //printf("Got Here\n");
      break;
    }
    else{
      temp_token[token_index] = BUFF[start];
    }
    token_index++;
    start++;
  }
  //printf("Got Here\n");
  temp_token[token_index++] = 0;
  //printf("TEMP: %s, INDEX: %d\n", temp_token, token_index);
  token->chars = malloc(token_index * sizeof(char));
  strncpy(token->chars, temp_token, token_index);
  token->length = token_index;
  return SUCCESS;
}

static unsigned long hash(const char *token){
  unsigned long i = 0;
  for (int ii = 0; token[ii]; ii++){
    i += token[ii];
  }
  return i % HASH_CAP;
}


static struct token *create_token(const char *key, int* (*handler)()){
  struct token *token = (struct token *) malloc(sizeof(struct token));
  token->key = (char*) malloc(strlen(key) + 1);
  strcpy(token->key, key);
  token->handler = handler;
  return token;
}

static struct table *create_table(int size){
  struct table *table = (struct table *) malloc(sizeof(struct table));
  table->size = size;
  table->count = 0;
  table->tokens = (struct token **) calloc(table->size, sizeof(struct token));

  for (int i = 0; i < table->size; i++)
    table->tokens[i] = NULL;

  table->overflow = create_overflow(table);
  return table;
}

static void free_token(struct token *token){
  free(token->key);
  //free(token->value);
  free(token);
}

static void free_table(struct table *table){
  for(int i = 0; i < table->size; i++){
    struct token *token = table->tokens[i];
    if (token != NULL)
      free_token(token);

  }
  free_overflow(table);
  free(table->tokens);
  free(table);
}

static void insert(struct table *table, char* key, int* (*handler)()){
  struct token *token = create_token(key, handler);
  int index = hash(key);
  struct token *current = table->tokens[index];
  if (current == NULL){
    // Check if Hash talbe is full
    table->tokens[index] = token;
    table->count++;
  }else if(strcmp(current->key, key) == 0){
    //Handle double token entry
  }else{
    // Handle collision
    handle_collision(table, index, token);
    return;
  }
}

static void *search_token(struct table *table, char* key){
  int index = hash(key);
  struct token *token = table->tokens[index];
  struct collision_list *head = table->overflow[index];
  while (token != NULL){
    if (strcmp(token->key, key) == 0)
      return token->handler;

    if (head == NULL)
      return NULL;

    token = head->token;
    head = head->next;
  }
  return NULL;
}

static void print_table(struct table *table){
  printf("Printing entire table\n");
  for(int i = 0; i < table->size; i++)
    if(table->tokens[i]){
      printf("Index: %d, Key: %s, Value: %p ", i, table->tokens[i]->key, table->tokens[i]->handler);
      if(table->overflow[i])
        printf("Key: %s, Value: %p ", table->overflow[i]->token->key, table->overflow[i]->token->handler);
      printf("\n");
    }
}


static void print_token(struct token *token){
  
}


static void handle_collision(struct table *table, unsigned long index, struct token *token){
  printf("Collision %s\n", token->key);
  struct collision_list *head = table->overflow[index];
  if (head == NULL){
    head = allocate_list();
    head->token = token;
    table->overflow[index] = head;
    return;
  }
  else{
    table->overflow[index] = insert_collision(head, token);
    return;
  }
}

static void print_search(struct table *table, char *key){
  void (*handler)();
  if((handler = search_token(table, key)) == NULL){
    printf("Key: %s does not exist.\n", key);
    return;
  }else{
    printf("Key: %s, Val: %p\n", key, handler);
    if(handler){
      printf("key: %s\n", key);
      handler();
    }
  }
}


static struct collision_list* allocate_list(){
  return (struct collision_list*) malloc(sizeof(struct collision_list));
}

static struct collision_list* insert_collision(struct collision_list *list, struct token *token){
  if(!list){
    struct collision_list *head = allocate_list();
    head->token = token;
    head->next = NULL;
    list = head;
    return list;
  }
  else if(list->next == NULL){
    struct collision_list *node = allocate_list();
    node->token = token;
    node->next = NULL;
    list->next = node;
    return list;
  }
  struct collision_list *temp = list;
  while(temp->next->next)
    temp = temp->next;

  struct collision_list *node = allocate_list();
  node->token = token;
  node->next = NULL;
  temp->next = node;
  return list;
}

// Removes an item from the collision list
static struct token *remove_collision(struct collision_list *list){
  if(!list)
    return NULL;
  if(!list->next)
    return NULL;

  struct collision_list *node = list->next;
  struct collision_list *temp = list;
  temp->next = NULL;
  list = node;
  struct token *token = NULL;
  memcpy(temp->token, token, sizeof(struct token));
  free(temp->token->key);
  //free(temp->token->value);
  free(temp->token);
  free(temp);
  return token;
}

static void free_list(struct collision_list *list){
  struct collision_list *temp = list;
  while(list){
    temp = list;
    list = list->next;
    free(temp->token->key);
    //free(temp->token->value);
    free(temp->token);
    free(temp);
  }
}

static struct collision_list **create_overflow(struct table *table){
  struct collision_list **buckets = (struct collision_list**) calloc(table->size, sizeof(struct collision_list*));
  for(int i = 0; i < table->size; i++)
    buckets[i] = NULL;

  return buckets;
}

static void free_overflow(struct table *table){
  struct collision_list **overflow = table->overflow;
  for (int i = 0; i < table->size; i++)
    free_list(overflow[i]);

  free(overflow);
}

static void table_delete(struct table *table, char *key){
  int index = hash(key);
  struct token *token = table->tokens[index];
  struct collision_list *head = table->overflow[index];
  
  if(token == NULL)
    return;

  else{
    if(head == NULL && strcmp(token->key, key)){
      table->tokens[index] = NULL;
      free_token(token);
      table->count--;
      return;
    }
    else if(head != NULL){
      if (strcmp(token->key, key) == 0){
        free_token(token);
        struct collision_list *node = head;
        head = head->next;
        node->next = NULL;
        table->tokens[index] = create_token(node->token->key, node->token->handler);
        free_list(node);
        table->overflow[index] = head;
        return;
      }
    }
    struct collision_list *curr = head;
    struct collision_list *prev = NULL;

    while(curr){
      if(strcmp(curr->token->key, key) == 0){
        if(prev == NULL){
          free_list(head);
          table->overflow[index] = head;
          return;
        }else{
          prev->next = curr->next;
          curr->next = NULL;
          free_list(curr);
          table->overflow[index] = head;
          return;
        }
      }
      curr = curr->next;
      prev = curr;
    }
  }
}

// Handle helpers
static void handle_value_token(uint64_t *start, uint64_t end, String *token){
  //printf("Handle Value Token (%ld)\n", *start);
  char val[TOKEN_SZ];
  int token_index = 0;
  String quote;
  quote.chars = NULL;
  quote.length = 0;
  while(BUFF[(*start)++] != ' '); 
  if(BUFF[*start] == '\"'){
    handle_quotes(start, end, &quote);
    *token = quote;
  }else{
    while(BUFF[*start] != ')' && BUFF[*start] != '(' && BUFF[*start] != ' ' && BUFF[*start] > 32 && *start < end){
      val[token_index++] = BUFF[(*start)++];
      //(*start)++;
    }
    val[token_index++] = '\0';
    token->chars = malloc(token_index * sizeof(char));
    strncpy(token->chars, val, token_index);
    token->length = token_index;
  }
  //printf("Printing Token: %s\n", token->chars);
}


static int handle_quotes(uint64_t *start, uint64_t end, String *quote){
  //printf("Handle Quotes\n");
  //printf("Start %ld End %ld\n", *start, end);
  uint64_t index = *start;
  int token_index = 0;
  char token[TOKEN_SZ];
  if(BUFF[index++] != '\"'){
    printf("Weird weird\n");
    return ERROR;
  }
  while(index < end){
    if(BUFF[index] == '\"'){
      break;
    }
    token[token_index++] = BUFF[index++];
    //token_index++;
    //index++;
  }
  token[token_index++] = 0;
  *start = ++index;
  if(quote){
    quote->chars = malloc(token_index * sizeof(char));
    strncpy(quote->chars, token, token_index);
    quote->length = token_index;
  }
  return SUCCESS;
}

static void set_section_index(uint64_t start, uint64_t end, struct Section_Index *index){
  //printf("Handle Section Index\n");
  index->set = SECTION_SET;
  index->section_start = start;
  index->section_end = end;
}

static struct Layer *find_layer(String name){
  for(struct Layer *layer = pcb->layers.layer; layer; layer = layer->next){
    if(string_compare(name, layer->canonical_name) == TRUE){
      return layer;
      //printf("(ORDINAL: %d; CANONICAL_NAME: \"%s\"; TYPE: %d; USER_NAME: \"%s\")\n", pcb->footprints->layer->ordinal, pcb->footprints->layer->canonical_name.chars, pcb->footprints->layer->type, pcb->footprints->layer->user_name.chars);
    }
    //printf("%s = %s\n", name.chars, layer->canonical_name.chars);
  }
  return NULL;
}

static struct Net *find_net(int ordinal){
  for(struct Net *net = pcb->nets; net; net = net->next){
    if(ordinal == net->ordinal){
      return net;
    }
  }
  return NULL;
}

// Handlers
static int *handle_kicadpcb(uint64_t start, uint64_t end){
  //printf("Handling PCB\n");
  pcb->kicad_pcb.section_start = start;
  pcb->kicad_pcb.section_end = end;
  set_section_index(start, end, &pcb->kicad_pcb);
  pcb->header.index.set = SECTION_SET;
  return &pcb->kicad_pcb.set;
}

// Handle Token Functions
static int *handle_version(uint64_t start, uint64_t end){
  //printf("Handle Version (%ld)\n", start);
  String version;
  version.chars = NULL;
  if(pcb->header.index.set == SECTION_SET){
    handle_value_token(&start, end, &version);
    pcb->header.version = version;
    //printf("Found version: %s\n",pcb->header.version.chars);
    return NULL;
  }
  return NULL;
}

static int *handle_generator(uint64_t start, uint64_t end){
  //printf("Handle Generator\n");
  String generator;
  generator.chars = NULL;
  if(pcb->header.index.set == SECTION_SET){
    handle_value_token(&start, end, &generator);
    pcb->header.generator = generator;
    //printf("Found generator: %s\n",pcb->header.generator.chars);
    return NULL;
  }
  return NULL;
}

static int *handle_generator_version(uint64_t start, uint64_t end){
  //printf("Handle Generator Version\n");
  if(pcb->header.index.set == SECTION_SET){
    String generator_version;
    generator_version.chars = NULL;
    handle_value_token(&start, end, &generator_version);
    pcb->header.generator_version = generator_version;
    //printf("Found generator version: %s\n", pcb->header.generator_version.chars);
    return &pcb->header.index.set;
  }
  return NULL;
}

static int *handle_general(uint64_t start, uint64_t end){
  //printf("Handle General\n");
  if(pcb->header.index.set == SECTION_CLOSED){
    //printf("Found general\n");
    set_section_index(start, end, &pcb->general.index);
    return &pcb->general.index.set;
  }
  return NULL;
}

static int *handle_thickness(uint64_t start, uint64_t end){
  //printf("Handle Thickness\n");
  if(pcb->general.index.set == SECTION_SET){
    float thickness;
    if(sscanf(&BUFF[start], "(thickness %f)", &thickness) != 1){
      printf("thickness 1\n");
    }
    pcb->general.thickness = thickness;
  }else if(pcb->layers.layer->index.set == SECTION_SET){
    float thickness;
    if(sscanf(&BUFF[start], "(thickness %f)", &thickness) != 1){
      printf("thickness 2\n");
    }
    pcb->layers.layer->thickness = thickness;
  }
  return NULL;
}

static int *handle_paper(uint64_t start, uint64_t end){
  //printf("Handle Paper\n");
  if(pcb->general.index.set == SECTION_CLOSED){
    //printf("Handle Paper\n");
    pcb->page.index.set = SECTION_SET;
    set_section_index(start, end, &pcb->page.index);
    String paper;
    handle_value_token(&start, end, &paper);
    pcb->page.paper = paper;
    //printf("Found paper: %s\n", pcb->page.paper.chars);
    return &pcb->page.index.set;
  }
  return NULL;
}

static int *handle_layers(uint64_t start, uint64_t end){
  //printf("Handle Layers\n");
  if(pcb->page.index.set == SECTION_CLOSED && pcb->layers.index.set != SECTION_CLOSED){
    int opens = 0;
    uint64_t layer_start = 0, layer_end = 0;
    pcb->layers.index.set = SECTION_SET;
    pcb->layers.index.section_start = start;
    pcb->layers.index.section_end = end;
    pcb->layers.layer = NULL;
    while(++start < end){
      if(BUFF[start] == '\"'){
        handle_quotes(&start, end, NULL);
      }
      if(BUFF[start] == '('){
        layer_start = start;
        opens++;
        
      }else if(BUFF[start] == ')'){
        opens--;
        layer_end = start;
        if(opens != 0){
          printf("WTF\n");
          return NULL;
        }
      }
      if(layer_start && layer_end){
        handle_layer(layer_start, layer_end);
        layer_start = 0;
        layer_end = 0;
      }
    }
    return &pcb->layers.index.set;
  }else if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->pads && pcb->footprints->pads->index.set == SECTION_SET){
    int layer_count = 0, index = start;
    while(++index < end){
      //handle_value_token(&start, end, )
      if(BUFF[index] == ' '){
        layer_count++;
      }
    }
    struct Layer **layer = calloc(layer_count, sizeof(struct Layer *));
    String layer_name;
    for(int i = 0; i < layer_count; i++){
      handle_value_token(&start, end, &layer_name);
      layer[i] = find_layer(layer_name);
      free(layer_name.chars);
      //printf("Layer[%d] \"%s\"\n", i, layer[i]->canonical_name.chars);
    }
    pcb->footprints->pads->layer_count = layer_count;
    pcb->footprints->pads->layers = layer;
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET && pcb->tracks->type == TRACK_TYPE_VIA){
    int layer_count = 0, index = start;
    while(++index < end){
      if(BUFF[index] == ' '){
        layer_count++;
      }
    }
    struct Layer **layer = calloc(layer_count, sizeof(struct Layer *));
    String layer_name;
    for(int i = 0; i < layer_count; i++){
      handle_value_token(&start, end, &layer_name);
      layer[i] = find_layer(layer_name);
      free(layer_name.chars);
      //printf("Layer[%d] \"%s\"\n", i, layer[i]->canonical_name.chars);
    }
    pcb->tracks->track.via.layer_count = layer_count;
    pcb->tracks->track.via.layers = layer;
  }
  return NULL;
}

static int *handle_layer(uint64_t start, uint64_t end){
  //printf("Handling Layer\n");
  uint64_t index = start;
  if(pcb->layers.index.set == SECTION_SET){
    char s_ordinal[TOKEN_SZ], type[TOKEN_SZ];
    String cononical_name, user_name;
    cononical_name.chars = NULL, user_name.chars = NULL;
    int ordinal, layer_index = 0, type_index = 0;
    if(BUFF[index] != '('){
      printf("Weird Error in handle_layer\n");
      return NULL;
    }
    while(++index < end){
      if(BUFF[index] == '\"'){
        handle_quotes(&index, end, (cononical_name.chars == NULL ? &cononical_name : &user_name)); // Handle error
      }
      if(BUFF[index] == ')'){
        break;
      }
      if(cononical_name.chars == NULL && BUFF[index] != ' '){
        s_ordinal[layer_index++] = BUFF[index];
      }
      if(cononical_name.chars != NULL && user_name.chars == NULL && BUFF[index] != ' '){
        type[type_index++] = BUFF[index];
      }
    }
    s_ordinal[layer_index] = 0;
    type[type_index] = 0;
    ordinal = atoi(s_ordinal);
    struct Layer *layer = malloc(sizeof(struct Layer));
    layer->index.section_start = start;
    layer->index.section_end = end;
    layer->index.set = SECTION_SET;
    layer->ordinal = ordinal;
    layer->canonical_name = cononical_name;
    layer->material.chars = NULL;
    layer->material.length = 0;
    if(strcmp(type, (char *)"jumper") == 0){
      layer->type = LAYER_TYPE_JUMPER;
    }else if(strcmp(type, (char *)"mixed") == 0){
      layer->type = LAYER_TYPE_MIXED;
    }else if(strcmp(type, (char *)"power") == 0){
      layer->type = LAYER_TYPE_POWER;
    }else if(strcmp(type, (char *)"signal") == 0){
      layer->type = LAYER_TYPE_SIGNAL;
    }else{
      layer->type = LAYER_TYPE_USER;
    }
    layer->user_name = user_name;
    layer->next = NULL;
    layer->prev = NULL;

    if(pcb->layers.layer == NULL){
      pcb->layers.layer = layer;
    }else{
      pcb->layers.layer->prev = layer;
      layer->next = pcb->layers.layer;
      pcb->layers.layer = layer;
    }
    return &layer->index.set;
  }else if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->properties && pcb->footprints->properties->index.set == SECTION_SET){
    String name;
    name.chars = NULL;
    name.length = 0;
    handle_value_token(&start, end, &name);
    pcb->footprints->properties->layer = find_layer(name);
    free(name.chars);
  }else if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->layer == NULL){
    String name;
    name.chars = NULL;
    handle_value_token(&start, end, &name);
    pcb->footprints->layer = find_layer(name);
    free(name.chars);
  }else if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->fp_lines && pcb->footprints->fp_lines->index.set == SECTION_SET && pcb->footprints->fp_lines->layer == NULL){
    String name;
    name.chars = NULL;
    handle_value_token(&start, end, &name);
    pcb->footprints->fp_lines->layer = find_layer(name);
    free(name.chars);
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET){
    String name;
    name.chars = NULL;
    handle_value_token(&start, end, &name);
    if(pcb->tracks->type == TRACK_TYPE_ARC){
      pcb->tracks->track.arc.layer = find_layer(name);
    }else if(pcb->tracks->type == TRACK_TYPE_SEG){
      pcb->tracks->track.segment.layer = find_layer(name);
    }
    free(name.chars);
  }else if(pcb->stackup.index.set == SECTION_SET){
    String name;
    name.length = 0;
    name.chars = NULL;
    struct Layer *layer;
    int dielectric = 0;
    handle_value_token(&start, end, &name);
    layer = find_layer(name);
    if(layer == NULL){
      if(sscanf(name.chars, "dielectric %d", &dielectric) == 1){
        //printf("Dielectric layer\n");
        layer = malloc(sizeof(struct Layer));
        //while(BUFF[++start] != '\"');
        handle_value_token(&start, end, &layer->canonical_name);
        PUSH(layer, pcb->layers.layer);
      }
    }
    free(name.chars);
    layer->index.section_start = start;
    layer->index.section_end = end;
    layer->index.set = SECTION_SET;
    return &layer->index.set;
  }
  return NULL;
}

static int *handle_type(uint64_t start, uint64_t end){
  if(pcb->layers.layer && pcb->layers.layer->index.set == SECTION_SET){
    String type;
    //while(BUFF[++start] != '\"');
    handle_value_token(&start, end, &type);
    pcb->layers.layer->stackup_type = type;
  }
  return NULL;
}

static int *handle_setup(uint64_t start, uint64_t end){
  //printf("Handle setup\n");
  if(pcb->layers.index.set == SECTION_CLOSED && pcb->setup.index.set != SECTION_CLOSED){
    pcb->setup.index.section_start = start;
    pcb->setup.index.section_end = end;
    pcb->setup.index.set = SECTION_SET;
    return &pcb->setup.index.set;
  }
  return NULL;
}

static int *handle_stackup(uint64_t start, uint64_t end){
  if(pcb->setup.index.set == SECTION_SET){
    pcb->stackup.index.section_start = start;
    pcb->stackup.index.section_end = end;
    pcb->stackup.index.set = SECTION_SET;
    return &pcb->stackup.index.set;
  }
  return NULL;
}

static int *handle_pad_to_mask_clearance(uint64_t start, uint64_t end){
  //printf("Hhandle_pad_to_mask_clearance\n");
  if(pcb->setup.index.set == SECTION_SET){
    String clearance;
    float f_clearance;
    handle_value_token(&start, end, &clearance);    
    char *endptr;
    f_clearance = strtof(clearance.chars, &endptr);
    free(clearance.chars);
    if(clearance.chars == endptr){
      return NULL;
    }
    pcb->setup.pad_to_mask_clearance = f_clearance;
    //printf("Found pad to mask clearance: %f\n", pcb->setup.pad_to_mask_clearance);
    return NULL;
  }
  return NULL;
}

static int *handle_solder_mask_min_width(uint64_t start, uint64_t end){
  //printf("andle_solder_mask_min_width\n");
  if(pcb->setup.index.set == SECTION_SET){
    //String width;
    float width;
    //handle_value_token(&start, end, &width);
    //char *endptr;
    //f_width = strtof(width.chars, &endptr);    
    //if(width.chars == endptr){
      //return NULL;
    if(sscanf(&BUFF[start], "(solder_mask_min_width %f)", &width) != 1){
      fprintf(stderr, "No soldermask width\n");
    }
    pcb->setup.solder_mask_min_width = width;
    return NULL;
  }
  return NULL;
}

static int *handle_pad_to_paste_clearance(uint64_t start, uint64_t end){
  //printf("handle_pad_to_paste_clearance\n");
  if(pcb->setup.index.set == SECTION_SET){
    String clearance;
    float f_clearance;
    handle_value_token(&start, end, &clearance);
    char *endptr;
    f_clearance = strtof(clearance.chars, &endptr);    
    if(clearance.chars == endptr){
      return NULL;
    }
    pcb->setup.pad_to_paste_clearance = f_clearance;
    return NULL;
  }
  return NULL;
}

static int *handle_pad_to_paste_clearance_ratio(uint64_t start, uint64_t end){
  //printf("dle_pad_to_paste_clearance_ratio\n");
  if(pcb->setup.index.set == SECTION_SET){
    String ratio;
    float f_ratio;
    handle_value_token(&start, end, &ratio);
    char *endptr;
    f_ratio = strtof(ratio.chars, &endptr);    
    if(ratio.chars == endptr){
      return NULL;
    }
    pcb->setup.pad_to_paste_clearance = f_ratio;
    return NULL;
  }
  return NULL;
}

static int *handle_pcbplotparams(uint64_t start, uint64_t end){
  //printf("handle_pcbplotparams\n");
  if(pcb->setup.index.set == SECTION_SET){
    pcb->setup.pcbplotparams.index.section_start = start;
    pcb->setup.pcbplotparams.index.section_end = end;
    pcb->setup.pcbplotparams.index.set = SECTION_SET;
    // Maybe implement later i dont think we care
    return &pcb->setup.pcbplotparams.index.set;
  }
  return NULL;
}

static int *handle_net(uint64_t start, uint64_t end){
  //printf("Handle Net1\n");
  int ordinal = -1;
  struct Net *net;
  if(sscanf(&BUFF[start], "(net %d \"%*s\")", &ordinal) == 1){
    //printf("Net error 1\n");
  }else if(sscanf(&BUFF[start], "(net %d)", &ordinal) == 1){
    //printf("Net error 2\n");
  }
  net = find_net(ordinal);
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->pads && pcb->footprints->pads->index.set == SECTION_SET){
    pcb->footprints->pads->net = net;
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET){
    switch(pcb->tracks->type){
      case TRACK_TYPE_VIA:
        pcb->tracks->track.via.net = net;
        break;
      case TRACK_TYPE_ARC:
        pcb->tracks->track.arc.net = net;
        break;
      case TRACK_TYPE_SEG:
        pcb->tracks->track.segment.net = net;
        break;
    }
  }else if (pcb->zones && pcb->zones->index.set == SECTION_SET){
    pcb->zones->net = net;
  }else{
    String name;
    while(BUFF[++start] != ' ');
    while(start++ < end){
      if(BUFF[start] == '\"'){
        handle_quotes(&start, end, &name);
      }
    }
    struct Net *net = malloc(sizeof(struct Net));
    net->index.section_start = start;
    net->index.section_end = end;
    net->index.set = SECTION_SET;
    net->name = name;
    net->ordinal = ordinal;
    net->next = NULL;
    net->prev = NULL;

    PUSH(net, pcb->nets);
    //printf("Handle Net3\n");
    return &net->index.set;
  }
  //printf("Handle Net4\n");
  return NULL;
}

static int *handle_footprint(uint64_t start, uint64_t end){
  //printf("Handle Foot1\n");
  struct Footprint *footprint = malloc(sizeof(struct Footprint));
  footprint->index.section_start = start;
  footprint->index.section_end = end;
  footprint->index.set = SECTION_SET;

  String library;
  library.chars = NULL;
  handle_value_token(&start, end, &library);

  footprint->library_link = library;
  footprint->layer = NULL;
  footprint->properties = NULL;
  footprint->fp_lines = NULL;
  footprint->pads = NULL;
  footprint->model = NULL;

  

  if(pcb->footprints == NULL){
    pcb->footprints = footprint;
  }else{
    pcb->footprints->prev = footprint;
    footprint->next = pcb->footprints;
    pcb->footprints = footprint;
  }
  //printf("Handle Foot2\n");
  return &pcb->footprints->index.set;
}

static int *handle_property(uint64_t start, uint64_t end){
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET){
    struct Footprint_Property *footprint_property = malloc(sizeof(struct Footprint_Property));
    struct Property *property = malloc(sizeof(struct Property));
    String key, val;
    key.chars = NULL;
    val.chars = NULL;
    key.length = 0, val.length = 0;
    footprint_property->index.set = SECTION_SET;
    footprint_property->index.section_start = start;
    footprint_property->index.section_end = end;

    handle_value_token(&start, end, &key);
    handle_value_token(&start, end, &val);
    property->key = key;
    property->val = val;
    property->next = NULL;

    //printf("Key: %s\n", key.chars);
    //printf("Value: %s\n", val.chars);

    footprint_property->property = property;

    if(pcb->footprints && pcb->footprints->properties == NULL){
      pcb->footprints->properties = footprint_property;
    }else if(pcb->footprints->properties){
      pcb->footprints->properties->prev = footprint_property;
      footprint_property->next = pcb->footprints->properties;
      pcb->footprints->properties = footprint_property;
    }else{
      printf("IDK\n");
      free(footprint_property);
      free(property);
      free(key.chars);
      free(val.chars);
    }

    return &footprint_property->index.set;
  }
  return NULL;
}

static int *handle_zone(uint64_t start, uint64_t end){
  //printf("Handle Zone1\n");
  struct Zone *zone = malloc(sizeof(struct Zone));
  zone->index.section_start = start;
  zone->index.section_end = end;
  zone->index.set = SECTION_SET;
  PUSH(zone, pcb->zones);
  //printf("Handle Zone2\n");
  return &pcb->zones->index.set;
}

/*static int *handle_track(uint64_t start, uint64_t end){
  struct Track *track = malloc(sizeof(struct Track));
  track->index.section_start = start;
  track->index.section_end = end;
  track->index.set = SECTION_SET;
  if(pcb->tracks == NULL){
    pcb->tracks = track;
  }else{
    pcb->tracks->prev = track;
    track->next = pcb->tracks;
    pcb->tracks = track;
  }
  return &pcb->tracks->index.set;
}*/

static int *handle_uuid(uint64_t start, uint64_t end){
  //printf("Handle_UUID\n");
  String uuid;
  uuid.chars = NULL;
  uuid.length = 0;
  handle_value_token(&start, end, &uuid);
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->uuid.chars == NULL){   
    pcb->footprints->uuid = uuid;
  }
  else if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->properties && pcb->footprints->properties->index.set == SECTION_SET){ // Needs work
    pcb->footprints->properties->uuid = uuid;
  }
  else if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->fp_lines && pcb->footprints->fp_lines->index.set == SECTION_SET){ // Needs work
    pcb->footprints->fp_lines->uuid = uuid;
  }else if(pcb->footprints && pcb->footprints->pads && pcb->footprints->pads->index.set == SECTION_SET){
    pcb->footprints->pads->uuid = uuid;
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET){
    pcb->tracks->uuid = uuid;
  }else{
    free(uuid.chars);
  }
  return NULL;
}

static int *handle_at(uint64_t start, uint64_t end){
  //printf("Handle_at\n");
  float x = 0.0, y = 0.0, angle = 0.0;
  if(sscanf(&BUFF[start], "(at %f %f %f)", &x, &y, &angle) == 3){
    //printf("(at %f %f %f)\n", x, y, angle);
  }else if(sscanf(&BUFF[start], "(at %f %f)", &x, &y) == 2){
    //printf("(at %f, %f)\n", x, y);
  }else{
    printf("Failed (at 0 0 0)");
  }
  struct at at;
  at.x = x;
  at.y = y;
  at.angle = angle;

  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->properties && pcb->footprints->properties->index.set == SECTION_SET){
    pcb->footprints->properties->at = at;
  }else if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->pads && pcb->footprints->pads->index.set == SECTION_SET){
    pcb->footprints->pads->at = at;
  }else if(pcb->footprints && pcb->footprints->index.set == SECTION_SET){
    pcb->footprints->at = at;
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET && pcb->tracks->type == TRACK_TYPE_VIA){
    pcb->tracks->track.via.at = at;
  }
  return NULL;
}

static int *handle_descr(uint64_t start, uint64_t end){
  //printf("Handle description\n");
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET){
    String description;
    //printf("Handle description1\n");
    handle_value_token(&start, end, &description);
    //printf("Handle description2\n");
    pcb->footprints->description = description;
  }
  return NULL;
}

static int *handle_fp_line(uint64_t start, uint64_t end){
  //printf("FP_LIME\n");
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET){
    struct Line *line = malloc(sizeof(struct Line));
    line->index.section_start = start;
    line->index.section_end = end;
    line->index.set = SECTION_SET;
    line->next = NULL;
    if(pcb->footprints->fp_lines == NULL){
      pcb->footprints->fp_lines = line;
    }else{
      pcb->footprints->fp_lines->prev = line;
      line->next = pcb->footprints->fp_lines;
      pcb->footprints->fp_lines = line;
    }
    return &line->index.set;
  }
  return NULL;
}

static int *handle_start(uint64_t start, uint64_t end){
  struct Point point;
  if(sscanf(&BUFF[start], "(start %f %f)", &point.x, &point.y) == 2){

  }else{
    fprintf(stderr, "Weird start\n");
  }
  if(pcb->footprints->fp_lines && pcb->footprints->fp_lines->index.set == SECTION_SET){
    pcb->footprints->fp_lines->start = point;
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET && pcb->tracks->type == TRACK_TYPE_SEG){
    pcb->tracks->track.segment.start = point;
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET && pcb->tracks->type == TRACK_TYPE_ARC){
    pcb->tracks->track.arc.start = point;
  }
  return NULL;
}

static int *handle_end(uint64_t start, uint64_t end){
  struct Point point;
  if(sscanf(&BUFF[start], "(end %f %f)", &point.x, &point.y) == 2){

  }else{
    //fprintf(stderr, "Weird end\n");
  }
  if(pcb->footprints->fp_lines && pcb->footprints->fp_lines->index.set == SECTION_SET){
    pcb->footprints->fp_lines->end = point;
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET && pcb->tracks->type == TRACK_TYPE_SEG){
    pcb->tracks->track.segment.end = point;
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET && pcb->tracks->type == TRACK_TYPE_ARC){
    pcb->tracks->track.arc.end = point;
  }
  return NULL;
}

static int *handle_width(uint64_t start, uint64_t end){
  float width = 0.0;
  if(sscanf(&BUFF[start], "(width %f)", &width) == 1){

  }else{
    fprintf(stderr, "Weird width\n");
  }
  if(pcb->tracks && pcb->tracks->index.set == SECTION_SET && pcb->tracks->type == TRACK_TYPE_ARC){
    pcb->tracks->track.arc.width = width;
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET && pcb->tracks->type == TRACK_TYPE_SEG){
    pcb->tracks->track.segment.width = width;
  }
  return NULL;
}

static int *handle_pad(uint64_t start, uint64_t end){
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET){
    struct Pad *pad = malloc(sizeof(struct Pad));
    pad->index.section_start = start;
    pad->index.section_end = end;
    pad->index.set = SECTION_SET;

    String number, type, shape;
    handle_value_token(&start, end, &number);
    handle_value_token(&start, end, &type);
    handle_value_token(&start, end, &shape);

    pad->num = number;
    if(strncmp(type.chars, "thru_hole", type.length) == 0){
      pad->type = THRU_HOLE;
    }else if(strncmp(type.chars, "connect", type.length) == 0){
      pad->type = CONNECT;
    }else if(strncmp(type.chars, "np_thru_hole", type.length) == 0){
      pad->type = NP_THRU_HOLE;
    }else{
      pad->type = SMD;
    }
    free(type.chars);
    if(strncmp(shape.chars, "circle", shape.length) == 0){
      pad->shape = CIRCLE;
    }else if(strncmp(shape.chars, "oval", shape.length) == 0){
      pad->shape = OVAL;
    }else if(strncmp(shape.chars, "trapezoid", shape.length) == 0){
      pad->shape = TRAPEZOID;
    }else if(strncmp(shape.chars, "roundrect", shape.length) == 0){
      pad->shape = ROUNDRECT;
    }/*else if(strncmp(shape.chars, "custom", shape.length) == 0){
      
    }*/else{
      pad->shape = RECT;
    }
    free(shape.chars);
    //printf("Pad1: %p", pad);

    if(pcb->footprints->pads == NULL){
      pad->next = NULL;
      pcb->footprints->pads = pad;
    }else{
      pcb->footprints->pads->prev = pad;
      pad->next = pcb->footprints->pads;
      pcb->footprints->pads = pad;
    }
    return &pad->index.set;
  }
  return NULL;
}

static int *handle_size(uint64_t start, uint64_t end){
  struct Size size;
  size.height = 0.0;
  size.width = 0.0;
  if(sscanf(&BUFF[start], "(size %f %f)", &size.width, &size.height) == 2){
    //printf("Found size: %f %f\n", size.width, size.height);
  }else if(sscanf(&BUFF[start], "(size %f)", &size.width) == 1){
    //printf("Found size: %f\n", size.width);
  }else{
    printf("Didn\'t find size\n");
  }
  if(pcb->footprints &&pcb->footprints->index.set == SECTION_SET && pcb->footprints->pads && pcb->footprints->pads->index.set == SECTION_SET){
    pcb->footprints->pads->size = size;
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET && pcb->tracks->type == TRACK_TYPE_VIA){
    pcb->tracks->track.via.size = size.width;
  }
  return NULL;
}

static int *handle_model(uint64_t start, uint64_t end){
  //printf("Handle Model\n");
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->model == NULL){
    struct Model *model = malloc(sizeof(struct Model));
    model->index.section_start = start;
    model->index.section_end = end;
    model->index.set = SECTION_SET;
    handle_value_token(&start, end, &model->model);
    pcb->footprints->model = model;
    return &model->index.set;
  }  
  return NULL;
}

static int *handle_offset(uint64_t start, uint64_t end){
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->model && pcb->footprints->model->index.set == SECTION_SET){
    struct Offset offset;
    offset.index.set = SECTION_SET;
    offset.index.section_start = start;
    offset.index.section_end = end;
    pcb->footprints->model->offset = offset;
    return &pcb->footprints->model->offset.index.set;
  }
  return NULL;
}

static int *handle_scale(uint64_t start, uint64_t end){
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->model && pcb->footprints->model->index.set == SECTION_SET){
    struct Scale scale;
    scale.index.set = SECTION_SET;
    scale.index.section_start = start;
    scale.index.section_end = end;
    pcb->footprints->model->scale = scale;
    return &pcb->footprints->model->scale.index.set;
  }
  return NULL;
}

static int *handle_rotate(uint64_t start, uint64_t end){
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->model && pcb->footprints->model->index.set == SECTION_SET){
    struct Rotate rotate;
    rotate.index.set = SECTION_SET;
    rotate.index.section_start = start;
    rotate.index.section_end = end;
    pcb->footprints->model->rotate = rotate;
    return &pcb->footprints->model->rotate.index.set;
  }
  return NULL;
}

static int *handle_xyz(uint64_t start, uint64_t end){
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->model && pcb->footprints->model->index.set == SECTION_SET){
    struct Model *model = pcb->footprints->model;
    struct XYZ xyz;
    if(sscanf(&BUFF[start], "(xyz %f %f %f)", &xyz.x, &xyz.y, &xyz.z) != 3){
      printf("Failed to get XYZ\n");
    }
    if(model->offset.index.set == SECTION_SET){
      model->offset.xyz = xyz;
    }else if(model->scale.index.set == SECTION_SET){
      model->scale.xyz = xyz;
    }else if(model->rotate.index.set == SECTION_SET){
      model->rotate.xyz = xyz;
    }
  }
  return NULL;
}

static int *handle_via(uint64_t start, uint64_t end){
  //struct Via *via = malloc(sizeof(struct Via));
  struct Track *track = malloc(sizeof(struct Track));
  track->index.set = SECTION_SET;
  track->index.section_start = start;
  track->index.section_end = end;
  track->type = TRACK_TYPE_VIA;
  if(pcb->tracks == NULL){
    pcb->tracks = track;
  }else{
    pcb->tracks->prev = track;
    track->next = pcb->tracks;
    pcb->tracks = track;
  }
  return &track->index.set;
}

static int *handle_segment(uint64_t start, uint64_t end){
  struct Track *track = malloc(sizeof(struct Track));
  track->index.set = SECTION_SET;
  track->index.section_start = start;
  track->index.section_end = end;
  track->type = TRACK_TYPE_SEG;
  if(pcb->tracks == NULL){
    pcb->tracks = track;
  }else{
    pcb->tracks->prev = track;
    track->next = pcb->tracks;
    pcb->tracks = track;
  }
  return &track->index.set;
}



static int *handle_drill(uint64_t start, uint64_t end){
  float diameter;
  if(sscanf(&BUFF[start], "(drill %f", &diameter) != 1){
      printf("Missed capturing drill width\n");
    }
  if(pcb->tracks && pcb->tracks->index.set == SECTION_SET && pcb->tracks->type == TRACK_TYPE_VIA){
    pcb->tracks->track.via.drill.diameter = diameter;
  }
  return NULL;
}

static int *handle_arc(uint64_t start, uint64_t end){
  struct Track *track = malloc(sizeof(struct Track));
  track->index.set = SECTION_SET;
  track->index.section_start = start;
  track->index.section_end = end;
  track->type = TRACK_TYPE_SEG;
  if(pcb->tracks == NULL){
    pcb->tracks = track;
  }else{
    pcb->tracks->prev = track;
    track->next = pcb->tracks;
    pcb->tracks = track;
  }
  return &track->index.set;
}

static int *handle_material(uint64_t start, uint64_t end){
  if(pcb->layers.layer->index.set == SECTION_SET){
    String material;
    //while(BUFF[++start] != '\"');
    handle_value_token(&start, end, &material);
    pcb->layers.layer->material = material;
  }
  return NULL;
}

static int *handle_epsilon_r(uint64_t start, uint64_t end){
  if(pcb->layers.layer && pcb->layers.layer->index.set == SECTION_SET){
    float epsilon_r;
    if(sscanf(&BUFF[start], "(epsilon_r %f)", &epsilon_r) != 1){
      printf("Epsilon_r Error\n");
    }
    pcb->layers.layer->epsilon_r = epsilon_r;
  }
  return NULL;
}

static int *handle_loss_tangent(uint64_t start, uint64_t end){
  if(pcb->layers.layer && pcb->layers.layer->index.set == SECTION_SET){
    float loss;
    if(sscanf(&BUFF[start], "(loss_tangent %f)", &loss) != 1){
      printf("Loss error\n");
    }
    pcb->layers.layer->loss_tangent = loss;
  }
  return NULL;
}

static int *handle_polygon(uint64_t start, uint64_t end){
  if(pcb->zones && pcb->zones->index.set == SECTION_SET){
    pcb->zones->polygon.index.section_start = start;
    pcb->zones->polygon.index.section_end = end;
    pcb->zones->polygon.index.set = SECTION_SET;
    
    return &pcb->zones->polygon.index.set;
  }
  return NULL;
}

static int *handle_filled_polygon(uint64_t start, uint64_t end){
  if(pcb->zones && pcb->zones->index.set == SECTION_SET){
    pcb->zones->filled_polygon.index.section_start = start;
    pcb->zones->filled_polygon.index.section_end = end;
    pcb->zones->filled_polygon.index.set = SECTION_SET;
    return &pcb->zones->filled_polygon.index.set;
  }
  return NULL;
}

static int *handle_pts(uint64_t start, uint64_t end){
  int point_count = 0, open = 0;
  if(pcb->zones && pcb->zones->index.set == SECTION_SET && pcb->zones->polygon.index.set == SECTION_SET){
    while(++start < end){
      if(BUFF[start] == '('){
        open++;
        point_count++;
      }else if(BUFF[start] == ')'){
        open--;
      }
    }
    if (open != 0){
      printf("Weird error\n");
    }
    struct Point *pts = calloc(point_count, sizeof(struct Point));
    printf("Address: %p\n", pts);
    pcb->zones->polygon.points = pts;
    pcb->zones->polygon.point_index = 0;
    pcb->zones->polygon.point_count = point_count;
    //printf("Number of Points %d\n", point_count);
  }else if(pcb->zones && pcb->zones->index.set == SECTION_SET && pcb->zones->filled_polygon.index.set == SECTION_SET){
    while(++start < end){
      if(BUFF[start] == '('){
        open++;
        point_count++;
      }else if(BUFF[start] == ')'){
        open--;
      }
    }
    if (open != 0){
      printf("Weird error\n");
    }
    struct Point *pts = calloc(point_count, sizeof(struct Point));
    pcb->zones->filled_polygon.points = pts;
    pcb->zones->filled_polygon.point_index = 0;
    pcb->zones->filled_polygon.point_count = point_count;
    //printf("Number of Filled Points %d opens %d\n", point_count, open);
  }
  return NULL;
}

static int *handle_xy(uint64_t start, uint64_t end){
  struct Point point;
  point.x = 0, point.y = 0;
  if(sscanf(&BUFF[start], "(xy %f %f)", &point.x, &point.y) != 2){
    printf("XY Error\n");
  }
  if(pcb->zones && pcb->zones->polygon.index.set == SECTION_SET){
    pcb->zones->polygon.points[pcb->zones->polygon.point_index++] = point;
  }else if(pcb->zones && pcb->zones->filled_polygon.index.set == SECTION_SET){
    pcb->zones->filled_polygon.points[pcb->zones->filled_polygon.point_index++] = point;
  }
  return NULL;
}

/*
static int *handle_text(uint64_t start, uint64_t end){
  return NULL;
}

static int *handle_rect(uint64_t start, uint64_t end){
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET){
    struct Rect *rect = malloc(sizeof(struct Rect));
    rect->index.section_start = start;
    rect->index.section_end = end;
    rect->index.set == SECTION_SET;

    if(pcb->footprints->fp_rects == NULL){
      
    }

    return &rect->index.set;
  }
  return NULL;
}

static int *handle_circle(uint64_t start, uint64_t end){
  if(pcb->kicad_pcb.set == SECTION_SET){
    struct Circle *circle = malloc(sizeof(struct Circle));
  }
  return NULL;
}

static int *handle_arc(uint64_t start, uint64_t end){
  return NULL;
}

static int *handle_polygon(uint64_t start, uint64_t end){
  return NULL;
}

static int *handle_curve(uint64_t start, uint64_t end){
  return NULL;
}

static int *handle_bounding_box(uint64_t start, uint64_t end){
  return NULL;
}
*/
