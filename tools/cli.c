#include "../src/enums.h"
#include "../src/types.h"
#include <string.h>
#include <stdio.h>

void traverse(AstNode* root);
AstNode* parse(List* ls);
char* get_parse_error();
List* tokenize(FILE* f);

int main(int argc,char** argv){
  // FILE* f=fmemopen((char*)code,strlen(code),"r");
  FILE* f=stdin;
  List* ls=tokenize(f);
  AstNode* root=parse(ls);
  if(root){
    traverse(root);
  }else{
    printf("%s\n",get_parse_error());
  }
  dealloc_list(ls);
  fclose(f);
  return root?0:1;
}
