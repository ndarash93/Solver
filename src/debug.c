#include "debug.h"

/*
void print_layer(){
  printf("(ORDINAL: %d; CANONICAL_NAME: \"%s\"; TYPE: %d; USER_NAME: \"%s\")\n", pcb->footprints->layer->ordinal, pcb->footprints->layer->canonical_name.chars, pcb->footprints->layer->type, pcb->footprints->layer->user_name.chars);
}

void print_footprints(){
  for(struct Footprint *footprint = pcb->footprints; footprint; footprint = footprint->next){
    printf("(footprint \"%s\"\n", footprint->library_link.chars);
  }
  printf("(ORDINAL: %d; CANONICAL_NAME: \"%s\"; TYPE: %d; USER_NAME: \"%s\")\n", pcb->footprints->layer->ordinal, pcb->footprints->layer->canonical_name.chars, pcb->footprints->layer->type, pcb->footprints->layer->user_name.chars);    
}*/



#ifdef DEBUG
struct Mem_Tracker{
  void *ptr;
  size_t size;
  struct Mem_Tracker *next;
};

static struct Mem_Tracker *tracker_list = NULL;

void mem_track(void *ptr, size_t size){
  struct Mem_Tracker *tracker = malloc(sizeof(struct Mem_Tracker));
  if(!tracker){
    fprintf(stderr, "Failed to allocate tracker\n");
    return;
  }
  tracker->ptr = ptr;
  tracker->size = size;
  tracker->next = tracker_list;
  tracker_list = tracker;
}

void mem_untrack(void *ptr){
  struct Mem_Tracker *temp = NULL;
  for(struct Mem_Tracker *curr = tracker_list; curr; curr = curr->next){
    if(curr->ptr == ptr){
      tracker_list = curr->next;
      free(curr);
      return;
    }else if(curr->next && curr->next->ptr == ptr){
      temp = curr->next->next;
      free(curr->next);
      curr->next = temp;
      return;
    }
  }
  fprintf(stderr, "Trying to untrack a non-tracked pointer\n");
}

void *mem_track_malloc(size_t size){
  void *ptr = malloc(size);
  if(ptr){
    mem_track(ptr, size);
    printf("Mallocated %zu bytes at %p\n", size, ptr);
  }
  return ptr;
}

void *mem_track_calloc(size_t num, size_t size){
  printf("Callocing\n");
  void *ptr = calloc(num, size);
  if(ptr){
    mem_track(ptr, num*size);
    printf("Callocated %zu bytes at %p\n", num*size, ptr);
  }
  return ptr;
}

void *mem_track_realloc(void *ptr, size_t size){
  if(ptr){
    mem_untrack(ptr);
  }
  void *new_ptr = realloc(ptr, size);
  if(new_ptr){
    mem_track(new_ptr, size);
    printf("Reallocated %zu bytes at %p\n", size, new_ptr);
  }
  return new_ptr;
}

void mem_track_free(void *ptr){
  mem_untrack(ptr);
  printf("Untracked memory at %p\n", ptr);
  free(ptr);
}

void mem_check_leaks(){
  printf("Memory tracking...\n");
  for (struct Mem_Tracker *tracker = tracker_list; tracker; tracker = tracker->next){
    fprintf(stderr, "Leaked %zu bytes at %p\n", tracker->size, tracker->ptr);
  }
  printf("End memory tracking...\n");
}

__attribute__((destructor)) void cleanup(){
  mem_check_leaks();
}

#endif