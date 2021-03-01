#include "../src/moonshot.h"
#include <string.h>
#include <stdio.h>

int main(int argc,char** argv){
  //const char* code="class hello extends hi where function yo() end (int,string,int) msg end";
  const char* code="*string(int) yo=string function(int a) return \"Hi\" end";
  FILE* f=fmemopen((char*)code,strlen(code),"r");

  // Compile
  moonshot_init();
  moonshot_compile(f);
  int n=moonshot_num_errors();
  if(n==1) printf("Moonshot compiler returned 1 error\n");
  if(n>1) printf("Moonshot compiler returned %i errors\n",n);
  for(int a=0;a<n;a++) printf("\033[31;1merror:\033[0m %s\n",moonshot_next_error());
  moonshot_destroy();
  return 0;
}
