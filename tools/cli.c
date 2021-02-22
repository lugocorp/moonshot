#include "../src/tokens.h"
#include "../src/types.h"
#include <string.h>
#include <stdio.h>

void set_token_string(List* ls);
List* tokenize(FILE* f);
int finish_parsing();
int parse_stmt();

int main(int argc,char** argv){
  // FILE* f=fmemopen((char*)code,strlen(code),"r");
  FILE* f=stdin;
  List* ls=tokenize(f);
  fclose(f);
  printf("Step 1: Tokenization\n");
  for(int a=0;a<ls->n;a++){
    Token* tk=(Token*)get_from_list(ls,a);
    if(tk->type!=TK_SPACE) printf("%i: %s\n",tk->type,tk->text);
  }
  printf("\nStep 2: parsing\n");
  set_token_string(ls);
  int res=parse_stmt() || finish_parsing();
  dealloc_list(ls);
  return res;
}
