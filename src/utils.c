#include "utils.h"

int string_compare(String _1, String _2){
    if(_1.length != _2.length){
        return FALSE;
    }
    for(uint64_t index = 0; index <= _1.length; index++){
      if(_1.chars[index] != _2.chars[index]){
        return FALSE;
      }
    }
    return TRUE;
}