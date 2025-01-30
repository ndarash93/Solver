#include "solver.h"

#define HASH_CAP 5000
#define TOKEN_SZ 250

#define BUFF pcb->file_buffer.buffer.chars
#define INDEX pcb->file_buffer.index
#define LENGTH pcb->file_buffer.buffer.length
#define CHAR pcb->file_buffer.buffer.chars[pcb->file_buffer.index]

// Hash table
static int token_lookup(const char *token);
static unsigned long hash(const char *token);
static struct token *create_token(const char *key, int (*handler)());
static struct table *create_table(int size);
static void free_token(struct token *token);
static void free_table(struct table *table);
static void insert(struct table *table, char* key, int (*handler)());
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
static void handle_value_token(String *token);

// Parsing
static void parse_pcb();
static int index_sections(void);

// Handler prototype
static int handle_version();
static int handle_kicadpcb();
static int handle_generator();
static int handle_generator_version();
static int handle_general();
static int handle_thickness();
static int handle_paper();

struct token{
  char *key;
  int (*handler)();
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
  //print_search(tokens, (char *)"kicad_pcb");
  //print_search(tokens, (char *)"version");
  //print_table(tokens);
  //free_table(tokens);
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

  index_sections();
  //parse_pcb();

clean_up:
  free(buffer);
  free_table(tokens);
  return SUCCESS;
}

static void parse_pcb(){
  int (*handler)() = NULL;
  char token[TOKEN_SZ];
  for(int i = 0; i < TOKEN_SZ; i++)
    token[i] = '\0';
  int token_index = 0;
  printf("Parsing PCB\n");
  while(INDEX < pcb->file_buffer.buffer.length){
    if (CHAR == '('){
      pcb->opens++;
      token_index = 0;
      //printf("Inc opens: %d\n", pcb->opens);
    }else if (CHAR == ')'){
      pcb->opens--;
      token_index = 0;
      //printf("Dec opens: %d\n", pcb->opens);
    }
    else if(CHAR == ' ')
      token_index = 0;
    else if(CHAR == '\n')
      token_index = 0;
    else if(CHAR == '\t')
      token_index = 0;
    else{
      while(CHAR != ' ' && CHAR != ')' && CHAR != '(' && CHAR != '\n' && CHAR != '\t' && CHAR != '\0' && token_index < TOKEN_SZ-1){
        token[token_index++] = CHAR;
        INDEX++;
        //printf("Char: %c\n", CHAR);
      }
      token[token_index] = '\0';
      token_index = 0;
      //printf("Token: %s\n", token);
      if((handler = search_token(tokens, token)) != NULL)
        if(handler() == ERROR)
          return;
      if (CHAR == ')'){
        pcb->opens--;
        token_index = 0;
        //printf("Dec2 opens: %d\n", pcb->opens);
      }
    }
    INDEX++;
    //printf("Char: %c\n", CHAR);
  }
}

static int index_sections(void){
  int opens = 0;
  pcb->header.index.set = SECTION_UNSET;
  pcb->general.index.set = SECTION_UNSET;
  pcb->page.index.set = SECTION_UNSET;
  pcb->layers.index.set = SECTION_UNSET;
  pcb->setup.index.set = SECTION_UNSET;
  //pcb->stackup.index.set = SECTION_UNSET;
  pcb->setup.pcbplotparams.index.set = SECTION_UNSET;
  pcb->nets.index.set = SECTION_UNSET;
  


  for(INDEX = 0; INDEX < LENGTH; INDEX++){
    if(CHAR == '('){
      if(1);
      opens++;
    }else if(CHAR == ')'){
      opens--;
    }
    //printf("Opens %d\n", opens);
  }
}

static unsigned long hash(const char *token){
  unsigned long i = 0;
  for (int ii = 0; token[ii]; ii++){
    i += token[ii];
  }
  return i % HASH_CAP;
}


static struct token *create_token(const char *key, int (*handler)()){
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

static void insert(struct table *table, char* key, int (*handler)()){
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

static void handle_value_token(String *token){
  char val[TOKEN_SZ];
  int token_index = 0;
  while(BUFF[++INDEX] == ' '); 
  while(BUFF[INDEX] != ')')
    val[token_index++] = BUFF[INDEX++];
  val[token_index++] = '\0';
  INDEX--;
  token->chars = malloc(token_index * sizeof(char));
  strncpy(token->chars, val, token_index);
  token->length = token_index;
}

// Handle Token Functions
static int handle_version(){
  String version;
  handle_value_token(&version);
  pcb->header.version = version;
  printf("Found version: %s\n",pcb->header.version.chars);
  return SUCCESS;
}

static int handle_kicadpcb(){
  printf("Found kicad_pcb\n");
  return SUCCESS;
}

static int handle_generator(){
  String generator;
  handle_value_token(&generator);
  pcb->header.generator = generator;
  printf("Found generator: %s\n",pcb->header.generator.chars);
  return SUCCESS;
}

static int handle_generator_version(){
  String generator_version;
  handle_value_token(&generator_version);
  pcb->header.generator_version = generator_version;
  printf("Found generator version: %s\n", pcb->header.generator_version.chars);
  return SUCCESS;
}

static int handle_general(){
  printf("Found general\n");
  if(pcb->opens != 2){
    printf("Interesting Error\n");
    return ERROR;
  }else{
    //struct General *general = malloc(sizeof(struct General));
    //pcb->general;
    //pcb->general = 1;
  }
  return SUCCESS;
}

static int handle_thickness(){
  //printf("Found thickness\n");
  if(pcb->opens == 3){
    String thickness;
    handle_value_token(&thickness);    
    char *endptr;
    pcb->general.thickness = strtof(thickness.chars, &endptr);
    if(thickness.chars == endptr)
      return ERROR;
    printf("Found thickness: %fmm\n", pcb->general.thickness);
    return SUCCESS;
  }else{
    printf("Interesting Error10\n");
    return SUCCESS;
  }
}

static int handle_paper(){
  String paper;
  handle_value_token(&paper);
  pcb->page.paper = paper;
  printf("Found paper: %s\n", pcb->page.paper.chars);
  return SUCCESS;
}