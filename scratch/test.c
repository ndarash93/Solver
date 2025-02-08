#include <stdio.h>

void func1();

int main(int argc, char** argv){
  printf("Function: %s\n", __func__);
  func1();
  return 0;
}

void func1(){
  printf("Function: %s\n", __func__);
}