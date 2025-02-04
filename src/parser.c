#include "solver.h"

#define HASH_CAP 5000
#define TOKEN_SZ 250

#define BUFF pcb->file_buffer.buffer.chars
#define INDEX pcb->file_buffer.index
#define LENGTH pcb->file_buffer.buffer.length
#define CHAR pcb->file_buffer.buffer.chars[pcb->file_buffer.index]

static int close;
static int line_count;
static int parse_index = 0, end_index = 0;

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
  buffer = malloc(length);

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
        printf("\n(", opens);
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
        printf("%s", token.chars);
        free(token.chars);
        parse_pcb(new_start+1, new_end-1);
        printf(")");
        if(section_set){
          printf("SECTION UNSET\n");
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
    token[token_index] = BUFF[index];
    token_index++;
    index++;
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
  pcb->header.index.section_start = parse_index;
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
  //printf("Start %ld End %ld\n", start, end);
  if(pcb->page.index.set == SECTION_CLOSED){
    int opens = 0;
    uint64_t layer_start = 0, layer_end = 0;
    pcb->layers.index.set = SECTION_SET;
    //printf("Handle Layers\n");
    //printf("Start %c End %c\n", BUFF[start], BUFF[end]);
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
  //printf("Start %c End %c\n", BUFF[start], BUFF[end]);
  //printf("Handle Layer\n");
  if(pcb->layers.index.set == SECTION_SET){
     //printf("Handle Layer2\n");
    char s_ordinal[TOKEN_SZ], type[TOKEN_SZ];
    String cononical_name, user_name;
    cononical_name.chars = NULL, user_name.chars = NULL;
    int ordinal, layer_index = 0, type_index = 0;
    //printf("Handle Layer3\n");
    //printf("Start %ld End %ld\n", start, end);
    if(BUFF[start] != '('){
      printf("Weird Error in handle_layer\n");
      return NULL;
    }
    //printf("Handle Layer4\n");
    while(++start < end){
      if(BUFF[start] == '\"'){
        handle_quotes(&start, end, (cononical_name.chars == NULL ? &cononical_name : &user_name)); // Handle error
      }
      if(BUFF[start] == ')'){
        //printf("End of layer");
        break;
      }
      if(cononical_name.chars == NULL && BUFF[start] != ' '){
        s_ordinal[layer_index++] = BUFF[start];
      }
      if(cononical_name.chars != NULL && user_name.chars == NULL && BUFF[start] != ' '){
        type[type_index++] = BUFF[start];
      }
    }
    s_ordinal[layer_index] = 0;
    type[type_index] = 0;
    ordinal = atoi(s_ordinal);
    printf("(ORDINAL: %d; CONONICAL_NAME: \"%s\"; TYPE: %s; USER_NAME: \"%s\")\n", ordinal, cononical_name.chars, type, user_name.chars);
  }
  // Allocate linked list node for each layer;
  return NULL;
}