#include <stdio.h>

int moonshot_num_errors();
char* moonshot_next_error();
void moonshot_init();
void moonshot_destroy();
int moonshot_compile(FILE* input);
