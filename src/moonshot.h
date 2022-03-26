#include <stdio.h>
#define VERSION "0.9.0 (beta)"

void moonshot_configure(FILE *input, FILE *output);
void dummy_required_file(char *filename);
char *moonshot_next_error();
int moonshot_num_errors();
void moonshot_destroy();
int moonshot_compile();
void moonshot_init();
void init_requires();
