#include "../src/tokens.h"
#include "../src/types.h"
#include <string.h>
#include <stdio.h>

void set_token_string(List* ls);
List* tokenize(FILE* f);
int parse_stmt();

int main(int argc,char** argv){
  const char* code="function hello()\n\tif red==5 then\n\t\thello(\"Yo lol\")\n\t\tprint(\"Hello there\")\n\tend\nend";
  FILE* f=fmemopen((char*)code,strlen(code),"r");
  List* ls=tokenize(f);
  printf("Step 1: Tokenization\n");
  for(int a=0;a<ls->n;a++){
    Token* tk=(Token*)get_from_list(ls,a);
    if(tk->type!=TK_SPACE) printf("%i: %s\n",tk->type,tk->text);
  }
  printf("\nStep 2: parsing\n");
  set_token_string(ls);
  int e=parse_stmt();
  if(e) printf("There was an error\n");
  dealloc_list(ls);
  fclose(f);
  return 0;
}
