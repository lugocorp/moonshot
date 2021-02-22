#include "../src/tokens.h"
#include "../src/types.h"
#include <string.h>
#include <stdio.h>

AstNode* parse(List* ls);
char* get_parse_error();
List* tokenize(FILE* f);

int main(int argc,char** argv){
  const char* code="do y=a.b() ::hello:: goto hello return y end while true do print(\"hello\") end";
  FILE* f=fmemopen((char*)code,strlen(code),"r");
  List* ls=tokenize(f);
  fclose(f);
  printf("Step 1: Tokenization\n");
  for(int a=0;a<ls->n;a++){
    Token* tk=(Token*)get_from_list(ls,a);
    if(tk->type!=TK_SPACE) printf("%i: %s\n",tk->type,tk->text);
  }
  printf("\nStep 2: parsing\n");
  AstNode* node=parse(ls);
  if(!node) printf("%s\n",get_parse_error());
  dealloc_list(ls);
  return node?0:1;
}
