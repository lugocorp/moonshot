#include "./internal.h"
#include <stdlib.h>
#include <string.h>

// Node constructors
AstNode* new_node(int type,void* data){
  AstNode* node=(AstNode*)malloc(sizeof(AstNode));
  node->type=type;
  node->data=data;
  return node;
}

FunctionNode* new_function_node(char* name,AstNode* type,List* args,List* body){
  FunctionNode* node=(FunctionNode*)malloc(sizeof(FunctionNode));
  strcpy(node->name,name);
  node->args=args;
  node->type=type;
  node->body=body;
  return node;
}

AstListNode* new_ast_list_node(AstNode* ast,List* list){
  AstListNode* node=(AstListNode*)malloc(sizeof(AstListNode));
  node->list=list;
  node->node=ast;
  return node;
}

TableNode* new_table_node(List* keys,List* vals){
  TableNode* node=(TableNode*)malloc(sizeof(TableNode));
  node->keys=keys;
  node->vals=vals;
  return node;
}

AstAstNode* new_ast_ast_node(AstNode* l,AstNode* r){
  AstAstNode* node=(AstAstNode*)malloc(sizeof(AstAstNode));
  node->l=l;
  node->r=r;
  return node;
}

StringAstNode* new_string_ast_node(char* text,AstNode* ast){
  StringAstNode* node=(StringAstNode*)malloc(sizeof(StringAstNode));
  node->text=text;
  node->node=ast;
  return node;
}

StringAstNode* new_primitive_node(char* text,const char* type){
  char* stype=(char*)malloc(strlen(type)+1);
  strcpy(stype,type);
  char* stext=(char*)malloc(strlen(text)+1);
  strcpy(stext,text);
  return new_string_ast_node(stext,new_node(AST_TYPE_BASIC,stype));
}

FornumNode* new_fornum_node(char* name,AstNode* num1,AstNode* num2,AstNode* num3,List* body){
  FornumNode* node=(FornumNode*)malloc(sizeof(FornumNode));
  strcpy(node->name,name);
  node->num1=num1;
  node->num2=num2;
  node->num3=num3;
  node->body=body;
  return node;
}

ForinNode* new_forin_node(AstNode* lhs,AstNode* tuple,List* body){
  ForinNode* node=(ForinNode*)malloc(sizeof(ForinNode));
  node->tuple=tuple;
  node->body=body;
  node->lhs=lhs;
  return node;
}

BinaryNode* new_binary_node(char* text,AstNode* l,AstNode* r){
  BinaryNode* node=(BinaryNode*)malloc(sizeof(BinaryNode));
  strcpy(node->text,text);
  node->r=r;
  node->l=l;
  return node;
}

InterfaceNode* new_interface_node(char* name,char* parent,List* ls){
  InterfaceNode* node=(InterfaceNode*)malloc(sizeof(InterfaceNode));
  if(parent) strcpy(node->parent,parent);
  else node->parent[0]=0;
  strcpy(node->name,name);
  node->ls=ls;
  return node;
}

ClassNode* new_class_node(char* name,char* parent,List* interfaces,List* ls){
  ClassNode* node=(ClassNode*)malloc(sizeof(ClassNode));
  if(parent) strcpy(node->parent,parent);
  else node->parent[0]=0;
  strcpy(node->name,name);
  node->interfaces=interfaces;
  node->ls=ls;
  return node;
}

char* new_string_node(char* msg){
  char* text=(char*)malloc(sizeof(char)*(strlen(msg)+1));
  strcpy(text,msg);
  return text;
}
BinaryNode* new_unary_node(char* op,AstNode* e){
  AstNode* type=new_node(AST_TYPE_BASIC,strcmp(op,"#")?PRIMITIVE_BOOL:PRIMITIVE_INT);
  return new_binary_node(op,e,type);
}
