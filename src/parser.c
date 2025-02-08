#include "solver.h"

#define HASH_CAP 5000
#define TOKEN_SZ 250

#define BUFF pcb->file_buffer.buffer.chars
#define INDEX pcb->file_buffer.index
#define LENGTH pcb->file_buffer.buffer.length
#define CHAR pcb->file_buffer.buffer.chars[pcb->file_buffer.index]

#define COPY(token, index, buff) (token[(index++)] = BUFF[(buff++)])

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
static int *handle_pad_to_mask_clearance(uint64_t start, uint64_t end);
static int *handle_solder_mask_min_width(uint64_t start, uint64_t end);
static int *handle_pad_to_paste_clearance(uint64_t start, uint64_t end);
static int *handle_pad_to_paste_clearance_ratio(uint64_t start, uint64_t end);
static int *handle_pcbplotparams(uint64_t start, uint64_t end);
static int *handle_net(uint64_t start, uint64_t end);
static int *handle_footprint(uint64_t start, uint64_t end);
static int *handle_zone(uint64_t start, uint64_t end);
static int *handle_track(uint64_t start, uint64_t end);
static int *handle_uuid(uint64_t start, uint64_t end);

// Handler Helpers
static int handle_quotes(uint64_t *start, uint64_t end, String *quote);
static void set_section_index(uint64_t start, uint64_t end, struct Section_Index *index);
static void handle_value_token(uint64_t *start, uint64_t end, String *token);

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
  insert(tokens, (char *)"pad_to_mask_clearance", handle_pad_to_mask_clearance);
  insert(tokens, (char *)"solder_mask_min_width", handle_solder_mask_min_width);
  insert(tokens, (char *)"pad_to_paste_clearance", handle_pad_to_paste_clearance);
  insert(tokens, (char *)"pad_to_paste_clearance_ratio", handle_pad_to_paste_clearance_ratio);
  insert(tokens, (char *)"pcbplotparams", handle_pcbplotparams);
  insert(tokens, (char *)"net", handle_net);
  insert(tokens, (char *)"footprint", handle_footprint);
  insert(tokens, (char *)"zone", handle_zone);
  insert(tokens, (char *)"via", handle_track);
  insert(tokens, (char *)"segment", handle_track);
  insert(tokens, (char *)"arc", handle_track);
  insert(tokens, (char *)"uuid", handle_uuid);
}

int open_pcb(const char *path){
  long length, bytes_read = 0;
  char *buffer = NULL;
  FILE *file;

  printf("Opening file %s\n", path);
  file = fopen(path, "rb");

  if (file == NULL){
    perror("Error opening file");
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
  if (token != NULL){
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
  while(BUFF[(*start)++] != ' '); 
  while(BUFF[(*start)] != ')')
    val[token_index++] = BUFF[(*start)++];
  val[token_index++] = '\0';
  //(*start)--;
  token->chars = malloc(token_index * sizeof(char));
  strncpy(token->chars, val, token_index);
  token->length = token_index;
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
    String thickness;
    handle_value_token(&start, end, &thickness);    
    char *endptr;
    pcb->general.thickness = strtof(thickness.chars, &endptr);
    if(thickness.chars == endptr){
      //printf("Float Error\n");
      return NULL;
    }
    //printf("Found thickness: %fmm\n", pcb->general.thickness);
    return NULL;
  }else{
    //printf("Interesting Error10\n");
    return NULL;
  }
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
  }else if(pcb->footprints && pcb->footprints->index.set == SECTION_SET){
    String name;
    name.chars = NULL;
    while(++index < end){
      if(BUFF[index] == '\"'){
        handle_quotes(&index, end, &name);
      }
    }
    for(struct Layer *layer = pcb->layers.layer; layer; layer = layer->next){
      if(string_compare(name, layer->canonical_name) == TRUE){
        pcb->footprints->layer = layer;
        //printf("(ORDINAL: %d; CANONICAL_NAME: \"%s\"; TYPE: %d; USER_NAME: \"%s\")\n", pcb->footprints->layer->ordinal, pcb->footprints->layer->canonical_name.chars, pcb->footprints->layer->type, pcb->footprints->layer->user_name.chars);
      }
    }
    free(name.chars);
  }
  //printf("Handle Layer4\n");
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

static int *handle_pad_to_mask_clearance(uint64_t start, uint64_t end){
  //printf("Hhandle_pad_to_mask_clearance\n");
  if(pcb->setup.index.set == SECTION_SET){
    String clearance;
    float f_clearance;
    handle_value_token(&start, end, &clearance);    
    char *endptr;
    f_clearance = strtof(clearance.chars, &endptr);
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
    String width;
    float f_width;
    handle_value_token(&start, end, &width);
    char *endptr;
    f_width = strtof(width.chars, &endptr);    
    if(width.chars == endptr){
      return NULL;
    }
    pcb->setup.solder_mask_min_width = f_width;
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
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET){
  
  }else if(pcb->tracks && pcb->tracks->index.set == SECTION_SET){

  }else if (pcb->zones && pcb->zones->index.set == SECTION_SET){

  }else{
    //printf("Handle Net2\n");
    char s_ordinal[TOKEN_SZ];
    int ordinal_index = 0;
    String name;
    while(BUFF[++start] != ' ');
    while(start++ < end){
      if(BUFF[start] < '9' && BUFF[start] > '0'){
        //printf("Ordinal: %c\n", BUFF[start]);
        COPY(s_ordinal, ordinal_index, start);
      }
      else if(BUFF[start] == '\"'){
        handle_quotes(&start, end, &name);
      }
    }
    struct Net *net = malloc(sizeof(struct Net));
    net->index.section_start = start;
    net->index.section_end = end;
    net->index.set = SECTION_SET;
    net->name = name;
    net->ordinal = atoi(s_ordinal);
    net->next = NULL;
    net->prev = NULL;

    s_ordinal[ordinal_index++] = '\0';
    //printf("%s\n", s_ordinal);
    if(pcb->nets == NULL){
      pcb->nets = net;
    }else{
      pcb->nets->prev = net;
      net->next = pcb->nets;
      pcb->nets = net;
    }
    //printf("Handle Net3\n");
    return &net->index.set;
  }
  //printf("Handle Net4\n");
  return NULL;
}

static int *handle_footprint(uint64_t start, uint64_t end){
  //printf("Handle Foot1\n");
  struct Footprint *footprint = malloc(sizeof( struct Footprint));
  footprint->index.section_start = start;
  footprint->index.section_end = end;
  footprint->index.set = SECTION_SET;

  String library;
  library.chars = NULL;
  while(++start < end){
    if(BUFF[start] == '\"'){
      handle_quotes(&start, end, &library);
      break;
    }
  }
  footprint->library_link = library;
  footprint->properties = NULL;
  footprint->fp_lines = NULL;
  footprint->pads = NULL;

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

static int *handle_zone(uint64_t start, uint64_t end){
  //printf("Handle Zone1\n");
  struct Zone *zone = malloc(sizeof(struct Zone));
  zone->index.section_start = start;
  zone->index.section_end = end;
  zone->index.set = SECTION_SET;
  if(pcb->zones == NULL){
    pcb->zones = zone;
  }else{
    pcb->zones->prev = zone;
    zone->next = pcb->zones;
    pcb->zones = zone;
  }
  //printf("Handle Zone2\n");
  return &pcb->zones->index.set;
}

static int *handle_track(uint64_t start, uint64_t end){
  //printf("Handle Track1\n");
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
  //printf("Handle Track2\n");
  return &pcb->tracks->index.set;
}

static int *handle_uuid(uint64_t start, uint64_t end){
  //printf("Handle_UUID\n");
  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->properties && pcb->footprints->properties->index.set == SECTION_SET){

  }
  else if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->properties && pcb->footprints->fp_lines && pcb->footprints->pads){ // Needs work
    String uuid;
    uuid.chars = NULL;
    while(++start < end){
      if(BUFF[start] == '\"'){
        handle_quotes(&start, end, &uuid);
      }
    }
    pcb->footprints->uuid = uuid;
  }
  return NULL;
}

static int *handle_at(uint64_t start, uint64_t end){
  //printf("Handle_at\n");
  float x = 0.0, y = 0.0, angle = 0.0;
  if(sscanf(&BUFF[start], "(at %f %f %f)", &x, &y, &angle) == 3){
    printf("(at %f %f %f)\n", x, y, angle);
  }else if(sscanf(&BUFF[start], "(at %f %f)", &x, &y) == 2){
    printf("(at %f, %f)\n", x, y);
  }else{
    printf("Failed (at 0 0 0)");
  }
  struct at at;
  at.x = x;
  at.y = y;
  at.angle = angle;

  if(pcb->footprints && pcb->footprints->index.set == SECTION_SET && pcb->footprints->properties){
    
  }else if(pcb->footprints && pcb->footprints->index.set == SECTION_SET){
    pcb->footprints->at = at;
  }
  return NULL;
}