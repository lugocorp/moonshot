#include "./internal.h"
#include <stdlib.h>
#include <string.h>
static List* scopes;

/*
  Initialize a new scope object
*/
static Scope* new_scope(int type,void* data){
  Scope* scope=(Scope*)malloc(sizeof(Scope));
  scope->defs=new_default_list();
  scope->type=type;
  scope->data=data;
  return scope;
}

/*
  Stuff to be done before we start working with scopes
*/
void preempt_scopes(){
  scopes=NULL;
}

/*
  Initializes the scope lists used by this module
*/
void init_scopes(){
  scopes=new_default_list();
}

/*
  Deallocs all scope Lists
*/
void dealloc_scopes(){
  dealloc_list(scopes);
}

/*
  Enters a new innermost scope
*/
void push_scope(){
  add_to_list(scopes,new_scope(SCOPE_NONE,NULL));
}

/*
  Exits the current innermost scope
*/
void pop_scope(){
  Scope* scope=remove_from_list(scopes,scopes->n-1);
  for(int a=0;a<scope->defs->n;a++){
    StringAstNode* var=(StringAstNode*)get_from_list(scope->defs,a);
    if(scope->type==SCOPE_CLASS && !strcmp(var->text,"this") && var->node->type==AST_TYPE_BASIC){
      ClassNode* class=(ClassNode*)(scope->data);
      char* type=(char*)(var->node->data);
      if(!strcmp(class->name,type)){
        free(var->text);
        free(var->node);
      }
    }
    free(var);
  }
  dealloc_list(scope->defs);
  free(scope);
}

/*
  Enters a new inner function scope
*/
void push_function_scope(FunctionNode* node){
  add_to_list(scopes,new_scope(SCOPE_FUNCTION,node));
  for(int a=0;a<node->args->n;a++){
    StringAstNode* arg=(StringAstNode*)get_from_list(node->args,a);
    StringAstNode* var=new_string_ast_node(arg->text,arg->node);
    if(!add_scoped_var(var)){
      // This should never ever happen
      free(var);
    }
  }
}

/*
  Returns the innermost function scope
  Returns NULL if the current scope is not within any function
*/
FunctionNode* get_function_scope(){
  for(int a=scopes->n-1;a>=0;a--){
    Scope* scope=(Scope*)get_from_list(scopes,a);
    if(scope->type==SCOPE_FUNCTION){
      return (FunctionNode*)(scope->data);
    }
  }
  return NULL;
}

/*
  Returns the innermost class's method scope
  Returns NULL if the current scope is not within any class's method
*/
FunctionNode* get_method_scope(){
  for(int a=scopes->n-1;a>=0;a--){
    Scope* scope=(Scope*)get_from_list(scopes,a);
    if(scope->type==SCOPE_FUNCTION && a){
      FunctionNode* func=(FunctionNode*)(scope->data);
      scope=(Scope*)get_from_list(scopes,a-1);
      if(scope->type==SCOPE_CLASS){
        return func;
      }
    }
  }
  return NULL;
}

/*
  Enters a new inner class scope
*/
void push_class_scope(ClassNode* node){
  add_to_list(scopes,new_scope(SCOPE_CLASS,node));
  char* this=(char*)malloc(sizeof(char)*5);
  sprintf(this,"this");
  AstNode* type=new_node(AST_TYPE_BASIC,node->name);
  StringAstNode* var=new_string_ast_node(this,type);
  if(!add_scoped_var(var)){
    // This should never ever happen
    free(type);
    free(this);
    free(var);
  }
}

/*
  Returns the innermost class scope
  Returns NULL if the current scope is not within any class
*/
ClassNode* get_class_scope(){
  for(int a=scopes->n-1;a>=0;a--){
    Scope* scope=(Scope*)get_from_list(scopes,a);
    if(scope->type==SCOPE_CLASS){
      return (ClassNode*)(scope->data);
    }
  }
  return NULL;
}

/*
  Adds a new typed variable to the current scope
  node must be allocated specifically for this function
  node will be freed automatically in pop_scope
*/
int add_scoped_var(StringAstNode* node){
  Scope* scope=(Scope*)get_from_list(scopes,scopes->n-1);
  for(int a=0;a<scope->defs->n;a++){
    if(!strcmp(((StringAstNode*)get_from_list(scope->defs,a))->text,node->text)){
      return 0;
    }
  }
  add_to_list(scope->defs,node);
  return 1;
}

/*
  Returns the BinaryNode representing the typed variable called name
  Return NULL if no such typed variable exists
  Searches through every scope from innermost to outermost
*/
StringAstNode* get_scoped_var(char* name){
  for(int a=(scopes->n)-1;a>=0;a--){
    Scope* scope=(Scope*)get_from_list(scopes,a);
    for(int b=0;b<scope->defs->n;b++){
      StringAstNode* n=(StringAstNode*)get_from_list(scope->defs,b);
      if(!strcmp(n->text,name)){
        return n;
      }
    }
  }
  return NULL;
}

/*
  Returns 1 if the given field is defined as a class field
*/
int field_defined_in_class(char* name){
  StringAstNode* node=get_scoped_var(name);
  for(int a=scopes->n-1;a>=0;a--){
    Scope* scope=(Scope*)get_from_list(scopes,a);
    for(int b=0;b<scope->defs->n;b++){
      StringAstNode* n=(StringAstNode*)get_from_list(scope->defs,b);
      if(!strcmp(n->text,name)){
        return scope->type==SCOPE_CLASS;
      }
    }
  }
  return 0;
}
