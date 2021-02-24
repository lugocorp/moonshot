#include "../src/enums.h"
#include "../src/types.h"
#include <string.h>
#include <stdio.h>

AstNode* parse(List* ls);
List* tokenize(FILE* f);

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
  AstNode* node=parse(ls);
  dealloc_list(ls);
  return node?0:1;
}
