#include <stdio.h>
#define VERSION "1.0.0"

void moonshot_configure(FILE* input,FILE* output);
char* moonshot_next_error();
int moonshot_num_errors();
void moonshot_destroy();
int moonshot_compile();
void moonshot_init();
