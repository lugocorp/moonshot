#include <stdio.h>

void moonshot_configure(FILE* input,FILE* output,int write);
char* moonshot_next_error();
int moonshot_num_errors();
void moonshot_destroy();
int moonshot_compile();
void moonshot_init();
