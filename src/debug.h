#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void print_layer();
void mem_track(void *ptr, size_t size, const int line, const char *function, const char *file);
void mem_untrack(void *ptr);
void *mem_track_malloc(size_t size, const int line, const char *function, const char *file);
void *mem_track_calloc(size_t num, size_t size, const int line, const char *function, const char *file);
void *mem_track_realloc(void *ptr, size_t size, const int line, const char *function, const char *file);
void mem_track_free(void *ptr);
void mem_check_leaks();
void cleanup();