#include "../src/tokens.h"
#include "../src/types.h"
#include <string.h>
#include <stdio.h>

List* tokenize(FILE* f);

int main(int argc,char** argv){
  const char* code="function hello\n\tif red==5 then\n\t\tred=red+7\n\t\tprint(\"Hello there\")\n\tend\nend";
  FILE* f=fmemopen((char*)code,strlen(code),"r");
  List* ls=tokenize(f);
  fclose(f);
  for(int a=0;a<ls->n;a++){
    Token* tk=(Token*)get_from_list(ls,a);
    if(tk->type==TK_SPACE) printf("\n");
    else printf("%i: %s\n",tk->type,tk->text);
  }
  dealloc_list(ls);
  return 0;
}
