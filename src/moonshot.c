#include "./moonshot.h"
#include "./internal.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#define ERROR_BUFFER_LENGTH 256
static char* error_error;
static List* errors;
static int error_i;
static FILE* _input;
static int _write;

// Error functions
int moonshot_num_errors(){
  return errors->n;
}
char* moonshot_next_error(){
  if(errors && error_i<errors->n) return (char*)get_from_list(errors,error_i++);
  return NULL;
}
void add_error(int line,const char* msg,...){
  char* err=(char*)malloc(sizeof(char)*ERROR_BUFFER_LENGTH);
  char symbol[2]={0,0};
  int n=strlen(msg);
  int total=0;
  err[0]=0;
  va_list args;
  va_start(args,msg);
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
  va_end(args);
  add_to_list(errors,err);
}

// Deallocation
static void dealloc_token_buffer(List* ls){
  for(int a=0;a<ls->n;a++) dealloc_token((Token*)get_from_list(ls,a));
  dealloc_list(ls);
}
static void dealloc_errors(){
  for(int a=0;a<errors->n;a++) free(get_from_list(errors,a));
  dealloc_list(errors);
}

// Main functions
void moonshot_init(){
  error_error=(char*)malloc(sizeof(char)*22);
  sprintf(error_error,"error buffer overflow");
  errors=NULL;
  _input=NULL;
  _write=1;
  error_i=0;
}
void moonshot_destroy(){
  if(errors) dealloc_errors();
}
void moonshot_configure(FILE* input,FILE* output,int write){
  set_output(output);
  _write=write;
  _input=input;
}
int moonshot_compile(){
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
  if(_write && !errors->n) traverse(root,0);
  dealloc_traverse();
  dealloc_ast_node(root);
  dealloc_token_buffer(ls);
  return (errors->n)?0:1;
}
