#include "./moonshot.h"
#include "./internal.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
static List* errors;
static int error_i;

// Error functions
int moonshot_num_errors(){
  return errors->n;
}
char* moonshot_next_error(){
  if(errors && error_i<errors->n) return (char*)get_from_list(errors,error_i++);
  return NULL;
}
void add_error(int line,const char* msg,...){
  char* e=(char*)malloc(sizeof(char)*256);
  char symbol[2]={0,0};
  int n=strlen(msg);
  e[0]=0;
  va_list args;
  va_start(args,msg);
  for(int a=0;a<n;a++){
    if(a<n-1 && msg[a]=='%'){
      if(msg[a+1]=='s'){
        strcat(e,va_arg(args,char*));
        a++;
        continue;
      }
      if(msg[a+1]=='t'){
        AstNode* type=(AstNode*)va_arg(args,AstNode*);
        char* str=stringify_type(type);
        strcat(e,str);
        free(str);
        a++;
        continue;
      }
    }
    symbol[0]=msg[a];
    strcat(e,symbol);
  }
  if(line>=0){
    char suffix[256];
    sprintf(suffix," on line %i",line);
    strcat(e,suffix);
  }
  va_end(args);
  add_to_list(errors,e);
}

// Deallocation
static void dealloc_deep(List* ls){
  for(int a=0;a<ls->n;a++){
    free(get_from_list(ls,a));
  }
  dealloc_list(ls);
}

// Main functions
void moonshot_init(){
  errors=NULL;
  error_i=0;
}
void moonshot_destroy(){
  if(errors) dealloc_deep(errors);
}
int moonshot_compile(FILE* f){
  if(errors) dealloc_deep(errors);
  errors=new_default_list();

  // Tokenize
  List* ls=tokenize(f);
  fclose(f);

  // Parse tokens
  AstNode* root=parse(ls);
  if(!root){
    dealloc_deep(ls);
    return 0;
  }

  // AST traversal
  traverse(root,1);
  if(!errors->n) traverse(root,0);
  // TODO dealloc root
  dealloc_deep(ls);
  return (errors->n)?0:1;
}
