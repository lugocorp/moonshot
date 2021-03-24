#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "internal.h"
static List* files; // List of Requires

/*
  Initializes this module
*/
void init_requires(){
  files=new_default_list();
}

/*
  Deallocates a list of required files
*/
void dealloc_requires(){
  for(int a=files->n-1;a>=0;a--){
    Require* r=(Require*)get_from_list(files,a);
    dealloc_ast_node(r->tree);
    dealloc_token_buffer(r->tokens);
    free(r->filename);
    free(r);
  }
  dealloc_list(files);
}

/*
  Tokenizes, parses and traverses another file to import external Moon types
  Will only bother if the filename ends in .moon (is Moonshot source code)
  Also checks to ensure that we're not processing a file we've already processed
*/
void require_file(char* filename){
  char* copy=(char*)malloc(sizeof(char)*(strlen(filename)-1));
  strncpy(copy,filename+1,strlen(filename)-2);
  copy[strlen(filename)-2]=0;
  int l=strlen(copy);
  if(l>5 && strcmp(copy+l-5,".moon")){
    free(copy);
    return;
  }
  for(int a=0;a<files->n;a++){
    Require* r=(Require*)get_from_list(files,a);
    if(!strcmp(r->filename,copy)){
      free(copy);
      return;
    }
  }
  FILE* f=fopen(copy,"r");
  if(!f){
    printf("%i\n",errno);
    add_error(-1,"cannot open file %s",copy);
    free(copy);
    return;
  }
  List* ls=tokenize(f);
  fclose(f);
  if(!ls){
    add_error(-1,"tokenization buffer overflow");
    free(copy);
    return;
  }
  AstNode* root=parse(ls);
  if(!root){
    dealloc_token_buffer(ls);
    free(copy);
    return;
  }
  traverse(root,1);
  Require* r=(Require*)malloc(sizeof(Require));
  r->filename=copy;
  r->tokens=ls;
  r->tree=root;
  add_to_list(files,r);
}
