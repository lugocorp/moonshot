#include "./moonshot.h"
#include "./internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#define ERROR_BUFFER_LENGTH 256 // Maximum length for an error message
static char* error_error; // Error message for when ERROR_BUFFER_LENGTH is overflows
static List* requires; // List of required files
static List* errors; // List of error strings
static int error_i; // Index of currently consumed error
static FILE* _input; // Input for source code

/*
  Return the number of compilation errors
*/
int moonshot_num_errors(){
  return errors->n;
}

/*
  Returns the next error message from compilation
  Returns NULL if there's no more errors
*/
char* moonshot_next_error(){
  if(errors && error_i<errors->n) return (char*)get_from_list(errors,error_i++);
  return NULL;
}

/*
  Adds an error into the compiler error stream
  Must include at least one parameter after msg or you'll get an error
  Exits with a special error if your msg and va_args cause the error buffer to overflow
*/
void add_error(int line,const char* msg,...){
  va_list args;
  va_start(args,msg);
  add_error_internal(line,msg,args);
  va_end(args);
}
void add_error_internal(int line,const char* msg,va_list args){
  char* err=(char*)malloc(sizeof(char)*ERROR_BUFFER_LENGTH);
  char symbol[2]={0,0};
  int n=strlen(msg);
  int total=0;
  err[0]=0;
  for(int a=0;a<n;a++){
    if(a<n-1 && msg[a]=='%'){
      if(msg[a+1]=='s'){
        char* str=va_arg(args,char*);
        total+=strlen(str);
        if(total+1>ERROR_BUFFER_LENGTH){
          add_to_list(errors,error_error);
          free(err);
          return;
        }
        strcat(err,str);
        a++;
        continue;
      }
      if(msg[a+1]=='t'){
        AstNode* type=(AstNode*)va_arg(args,AstNode*);
        char* str=stringify_type(type);
        total+=strlen(str);
        if(total+1>ERROR_BUFFER_LENGTH){
          add_to_list(errors,error_error);
          free(err);
          return;
        }
        strcat(err,str);
        free(str);
        a++;
        continue;
      }
      if(msg[a+1]=='i'){
        char str[4];
        int v=va_arg(args,int);
        if(v>=1000 || v<=-100){
          add_to_list(errors,error_error);
          free(err);
          return;
        }
        sprintf(str,"%i",v);
        total+=strlen(str);
        if(total+1>ERROR_BUFFER_LENGTH){
          add_to_list(errors,error_error);
          free(err);
          return;
        }
        strcat(err,str);
        a++;
        continue;
      }
    }
    total++;
    if(total+1>ERROR_BUFFER_LENGTH){
      add_to_list(errors,error_error);
      free(err);
      return;
    }
    symbol[0]=msg[a];
    strcat(err,symbol);
  }
  if(line>=0){
    char suffix[256];
    sprintf(suffix," on line %i",line);
    strcat(err,suffix);
  }
  add_to_list(errors,err);
}

/*
  Deallocates all tokens within the token list and the list itself
*/
void dealloc_token_buffer(List* ls){
  for(int a=0;a<ls->n;a++) dealloc_token((Token*)get_from_list(ls,a));
  dealloc_list(ls);
}

/*
  Deallocates all error strings in the errors list and the list itself
*/
static void dealloc_errors(){
  for(int a=0;a<errors->n;a++) free(get_from_list(errors,a));
  dealloc_list(errors);
  errors=NULL;
}

/*
  Initializes this module
*/
void init_requires(){
  requires=new_default_list();
}

/*
  Deallocates a list of required files
*/
static void dealloc_requires(){
  for(int a=requires->n-1;a>=0;a--){
    Require* r=(Require*)get_from_list(requires,a);
    if(r->tree) dealloc_ast_node(r->tree);
    if(r->tokens) dealloc_token_buffer(r->tokens);
    free(r->filename);
    free(r);
  }
  dealloc_list(requires);
  requires=NULL;
}

/*
  Manually adds a dummy node for the given filename
*/
void dummy_required_file(char* filename){
  char* copy=(char*)malloc(sizeof(char)*(strlen(filename)+1));
  strcpy(copy,filename);
  Require* r=(Require*)malloc(sizeof(Require));
  r->filename=copy;
  r->tokens=NULL;
  r->tree=NULL;
  add_to_list(requires,r);
}

/*
  Tokenizes, parses and traverses another file to import external Moon types
  Will only bother if the filename ends in .moon (is Moonshot source code)
  Also checks to ensure that we're not processing a file we've already processed
  Returns 1 if the required file is a Moonshot source file
*/
int require_file(char* filename,int validate){
  char* copy=(char*)malloc(sizeof(char)*(strlen(filename)-1));
  strncpy(copy,filename+1,strlen(filename)-2);
  copy[strlen(filename)-2]=0;
  int l=strlen(copy);
  if(l<5 || strcmp(copy+l-5,".moon")){
    free(copy);
    return 0;
  }
  if(validate){
    for(int a=0;a<requires->n;a++){
      Require* r=(Require*)get_from_list(requires,a);
      if(!strcmp(r->filename,copy)){
        free(copy);
        return 1;
      }
    }
    FILE* f=fopen(copy,"r");
    if(!f){
      add_error(-1,"cannot open file %s",copy);
      free(copy);
      return 1;
    }
    List* ls=tokenize(f);
    fclose(f);
    if(!ls){
      add_error(-1,"tokenization buffer overflow");
      free(copy);
      return 1;
    }
    AstNode* root=parse(ls);
    if(!root){
      dealloc_token_buffer(ls);
      free(copy);
      return 1;
    }
    traverse(root,1);
    Require* r=(Require*)malloc(sizeof(Require));
    r->filename=copy;
    r->tokens=ls;
    r->tree=root;
    add_to_list(requires,r);
  }else{
    for(int a=0;a<requires->n;a++){
      Require* r=(Require*)get_from_list(requires,a);
      if(!strcmp(r->filename,copy)){
        traverse(r->tree,0);
        free(copy);
        return 1;
      }
    }
    free(copy); // Should never get here
  }
  return 1;
}

/*
  Initializes data used by the Moonshot library
*/
void moonshot_init(){
  error_error=(char*)malloc(sizeof(char)*22);
  sprintf(error_error,"error buffer overflow");
  requires=NULL;
  errors=NULL;
  _input=NULL;
  error_i=0;
}

/*
  Deallocate remaining memory from Moonshot's compilation process
*/
void moonshot_destroy(){
  if(errors) dealloc_errors();
}

/*
  Sets configuration for compilation
  Sets source code and output I/O
  Also controls whether or not to write any output
*/
void moonshot_configure(FILE* input,FILE* output){
  set_output(output);
  _input=input;
}

/*
  Read from your configured input and compile Moonshot code
  Will only write Lua code to output if it's set in the configuration
*/
int moonshot_compile(){
  if(!requires) init_requires();
  if(errors) dealloc_errors();
  errors=new_default_list();

  // Tokenize
  List* ls=tokenize(_input);
  if(!ls){
    add_error(-1,"tokenization buffer overflow");
    return 0;
  }

  // Parse tokens
  AstNode* root=parse(ls);
  if(!root){
    dealloc_token_buffer(ls);
    return 0;
  }

  // AST traversal
  init_traverse();
  traverse(root,1);
  check_broken_promises();
  if(!errors->n) traverse(root,0);
  dealloc_traverse();
  dealloc_requires();
  dealloc_ast_node(root);
  dealloc_token_buffer(ls);
  return (errors->n)?0:1;
}
