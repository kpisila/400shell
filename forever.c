#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
  int i;
  while(1){
    i++;
    if(i == RAND_MAX){
      printf("MAX\n");
      break;
    }
  }
  return 0;
}
