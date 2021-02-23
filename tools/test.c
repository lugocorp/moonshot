#include "../src/tokens.h"
#include "../src/types.h"
#include <string.h>
#include <stdio.h>

void traverse(AstNode* node);
AstNode* parse(List* ls);
char* get_parse_error();
List* tokenize(FILE* f);

int main(int argc,char** argv){
  const char* code="if a then print() end";
  FILE* f=fmemopen((char*)code,strlen(code),"r");
  List* ls=tokenize(f);
  fclose(f);
  printf("Step 1: Tokenization\n");
  for(int a=0;a<ls->n;a++){
    Token* tk=(Token*)get_from_list(ls,a);
    if(tk->type!=TK_SPACE) printf("%i: %s\n",tk->type,tk->text);
  }

  printf("\nStep 2: parsing\n");
  AstNode* root=parse(ls);
  if(root){
    printf("\nStep 3: AST traversal\n");
    traverse(root);
  }else printf("%s\n",get_parse_error());
  dealloc_list(ls);
  return root?0:1;
}