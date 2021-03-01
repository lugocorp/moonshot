#include "../src/moonshot.h"
#include <stdio.h>

int main(int argc,char** argv){
  moonshot_init();
  moonshot_compile(stdin);
  int n=moonshot_num_errors();
  if(n==1) printf("Compilation returned 1 error\n");
  if(n>1) printf("Compilation returned %i errors\n",n);
  for(int a=0;a<n;a++) printf("ERROR %s\n",moonshot_next_error());
  moonshot_destroy();
  return 0;
}
