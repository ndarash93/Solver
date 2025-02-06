#include <stdint.h>

#define ERROR -1
#define SUCCESS 1

#define TRUE 1
#define FALSE 0

typedef struct {
    char *chars;
    uint64_t length;
} String;

int string_compare(String _1, String _2);