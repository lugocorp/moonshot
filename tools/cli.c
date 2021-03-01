#include "../src/moonshot.h"
#include <string.h>
#include <stdio.h>

int main(int argc,char** argv){
  init_moonshot();
  moonshot(stdin);
  for(int a=0;a<num_errors();a++){
    char* e=next_error();
    printf("ERROR %s\n",e);
  }
  destroy_moonshot();
  return 0;
}
