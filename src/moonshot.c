#include "./moonshot.h"
#include "./internal.h"
#include <stdlib.h>
static List* errors;
static int error_i;

// Error functions
void add_error(char* msg){
  add_to_list(errors,msg);
}
int num_errors(){
  return errors->n;
}
char* next_error(){
  if(errors && error_i<errors->n) return (char*)get_from_list(errors,error_i++);
  return NULL;
}

// Deallocation
static void dealloc_deep(List* ls){
  for(int a=0;a<ls->n;a++){
    free(get_from_list(ls,a));
  }
  dealloc_list(ls);
}

// Main functions
void init_moonshot(){
  errors=NULL;
  error_i=0;
}
void destroy_moonshot(){
  if(errors) dealloc_deep(errors);
}
int moonshot(FILE* f){
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
  traverse(root);
  // TODO dealloc root
  dealloc_deep(ls);
  return errors->n?0:1;
}
