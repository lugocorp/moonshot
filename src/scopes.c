#include "./internal.h"
#include <stdlib.h>
#include <string.h>
static List* scopes;

// Variable scope tracking
void init_scopes(){
  scopes=new_default_list();
}
void dealloc_scopes(){
  dealloc_list(scopes);
}
void push_scope(){
  add_to_list(scopes,new_default_list());
}
void pop_scope(){
  List* ls=remove_from_list(scopes,scopes->n-1);
  dealloc_list(ls);
}
int add_scoped_var(BinaryNode* node){
  List* scope=get_from_list(scopes,scopes->n-1);
  for(int a=0;a<scope->n;a++){
    if(!strcmp(((BinaryNode*)get_from_list(scope,a))->text,node->text)){
      return 0;
    }
  }
  add_to_list(scope,node);
  return 1;
}
BinaryNode* get_scoped_var(char* name){
  for(int a=(scopes->n)-1;a>=0;a--){
    List* scope=(List*)get_from_list(scopes,a);
    for(int b=0;b<scope->n;b++){
      BinaryNode* n=(BinaryNode*)get_from_list(scope,b);
      if(!strcmp(n->text,name)) return n;
    }
  }
  return NULL;
}
