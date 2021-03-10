#include "./internal.h"
#include <stdlib.h>
#include <string.h>
static List* scopes;
static List* funcs;

// Variable scope tracking
void preempt_scopes(){
  scopes=NULL;
  funcs=NULL;
}
void init_scopes(){
  scopes=new_default_list();
  funcs=new_default_list();
}
void dealloc_scopes(){
  dealloc_list(scopes);
  dealloc_list(funcs);
}
void push_scope(){
  add_to_list(scopes,new_default_list());
}
void pop_scope(){
  List* ls=remove_from_list(scopes,scopes->n-1);
  dealloc_list(ls);
}

// Functions
void push_function(FunctionNode* node){
  add_to_list(funcs,node);
  for(int a=0;a<node->args->n;a++){
    StringAstNode* arg=(StringAstNode*)get_from_list(node->args,a);
    BinaryNode* bn=new_binary_node(arg->text,arg->node,NULL);
    add_scoped_var(bn);
  }
}
void pop_function(FunctionNode* node){
  remove_from_list(funcs,funcs->n-1);
}
FunctionNode* get_function_scope(){
  if(funcs->n) return (FunctionNode*)get_from_list(funcs,funcs->n-1);
  return NULL;
}

// Variables
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
  if(scopes){
    for(int a=(scopes->n)-1;a>=0;a--){
      List* scope=(List*)get_from_list(scopes,a);
      for(int b=0;b<scope->n;b++){
        BinaryNode* n=(BinaryNode*)get_from_list(scope,b);
        if(!strcmp(n->text,name)) return n;
      }

    }
  }
  return NULL;
}
