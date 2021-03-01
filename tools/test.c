#include "../src/moonshot.h"
#include <string.h>
#include <stdio.h>

int main(int argc,char** argv){
  //const char* code="class hello extends hi where function yo() end (int,string,int) msg end";
  const char* code="typedef msg (string,int) typedef msg string";
  FILE* f=fmemopen((char*)code,strlen(code),"r");

  init_moonshot();
  moonshot(f);
  for(int a=0;a<num_errors();a++){
    char* e=next_error();
    printf("ERROR %s\n",e);
  }
  destroy_moonshot();
  return 0;
}
