#include "./moonshot.h"
#include <string.h>
#include <stdlib.h>

// Type nodes
AstNode* get_type(AstNode* node){ // TODO implement the commented branches
  int type=node->type;
  switch(node->type){
    case AST_PRIMITIVE: return ((StringAstNode*)(node->data))->node;
    //case AST_FUNCTION: return process_function(node);
    //case AST_BINARY: return process_binary(node);
    case AST_PAREN: return get_type((AstNode*)(node->data));
    //case AST_UNARY: return process_unary(node);
    case AST_TUPLE: return ((AstListNode*)(node->data))->node;
    //case AST_FIELD: return process_field(node);
    //case AST_CALL: return process_call(node);
    //case AST_SUB: return process_sub(node);
    case AST_ID:{
      char* name=(char*)(node->data);
      BinaryNode* var=get_scoped_var(name);
      return var?(var->l):NULL;
    };
    default: return NULL;
  }
}
int typed_match(AstNode* l,AstNode* r){ // TODO implement function types
  // FUNC
  if(l->type==AST_TYPE_ANY) return 1;
  if(!r) return 0;
  if(r->type==AST_TYPE_BASIC){
    if(!strcmp((char*)(r->data),"nil")) return 1;
  }
  if(l->type==AST_TYPE_BASIC && r->type==AST_TYPE_BASIC){
    return !strcmp((char*)(l->data),(char*)(r->data));
  }
  if(l->type==AST_TYPE_TUPLE && r->type==AST_TYPE_TUPLE){
    List* lls=(List*)(l->data);
    List* rls=(List*)(r->data);
    if(lls->n!=rls->n) return 0;
    for(int a=0;a<lls->n;a++){
      AstNode* nl=(AstNode*)get_from_list(lls,a);
      AstNode* nr=(AstNode*)get_from_list(rls,a);
      int match=typed_match(nl,nr);
      if(!match) return 0;
    }
    return 1;
  }
  return 0;
}

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
  if(name) strcpy(node->name,name);
  else node->name[0]=0;
  node->ls=ls;
  return node;
}

ClassNode* new_class_node(char* name,char* parent,List* interfaces,List* ls){
  ClassNode* node=(ClassNode*)malloc(sizeof(ClassNode));
  if(parent) strcpy(node->parent,parent);
  else node->parent[0]=0;
  if(name) strcpy(node->name,name);
  else node->name[0]=0;
  node->interfaces=interfaces;
  node->ls=ls;
  return node;
}

char* new_string_node(char* msg){
  char* text=(char*)malloc(sizeof(char)*(strlen(msg)+1));
  strcpy(text,msg);
  return text;
}
