#include "./internal.h"
#include <stdlib.h>
#include <string.h>
static List* classes; // List of ClassNodes
static List* scopes; // List of Lists of BinaryNodes
static List* funcs; // List of FunctionNodes

/*
  Stuff to be done before we start working with scopes
*/
void preempt_scopes(){
  classes=NULL;
  scopes=NULL;
  funcs=NULL;
}

/*
  Initializes the scope lists used by this module
*/
void init_scopes(){
  classes=new_default_list();
  scopes=new_default_list();
  funcs=new_default_list();
}

/*
  Deallocs all scope Lists
*/
void dealloc_scopes(){
  dealloc_list(classes);
  dealloc_list(scopes);
  dealloc_list(funcs);
}

/*
  Enters a new innermost scope
*/
void push_scope(){
  add_to_list(scopes,new_default_list());
}

/*
  Exits the current innermost scope
*/
void pop_scope(){
  List* ls=remove_from_list(scopes,scopes->n-1);
  dealloc_list(ls);
}

/*
  Enters a new inner function scope
*/
void push_function(FunctionNode* node){
  add_to_list(funcs,node);
  for(int a=0;a<node->args->n;a++){
    StringAstNode* arg=(StringAstNode*)get_from_list(node->args,a);
    BinaryNode* bn=new_binary_node(arg->text,arg->node,NULL);
    add_scoped_var(bn);
  }
}

/*
  Exits the innermost function scope
*/
void pop_function(){
  remove_from_list(funcs,funcs->n-1);
}

/*
  Returns the innermost function scope
  Returns NULL if the current scope is not within any function
*/
FunctionNode* get_function_scope(){
  if(funcs->n) return (FunctionNode*)get_from_list(funcs,funcs->n-1);
  return NULL;
}

/*
  Enters a new inner class scope
*/
void push_class(ClassNode* node){
  add_to_list(classes,node);
  char* this=(char*)malloc(sizeof(char)*5);
  sprintf(this,"this");
  AstNode* type=new_node(AST_TYPE_BASIC,node->name);
  BinaryNode* bn=new_binary_node(this,type,NULL);
  add_scoped_var(bn);
}

/*
  Exits the innermost class scope
*/
void pop_class(){
  remove_from_list(classes,classes->n-1);
}

/*
  Returns the innermost class scope
  Returns NULL if the current scope is not within any class
*/
ClassNode* get_class_scope(){
  if(classes->n) return (ClassNode*)get_from_list(classes,classes->n-1);
  return NULL;
}

/*
  Adds a new typed variable to the current scope
*/
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

/*
  Returns the BinaryNode representing the typed variable called name
  Return NULL if no such typed variable exists
  Searches through every scope from innermost to outermost
*/
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
