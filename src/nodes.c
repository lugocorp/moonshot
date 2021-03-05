#include "./internal.h"
#include <stdlib.h>
#include <string.h>

// Deallocation
void dealloc_ast_type(AstNode* node){
  if(node->type==AST_TYPE_FUNC){
    AstListNode* data=(AstListNode*)(node->data);
    if(data->node) dealloc_ast_type(data->node);
    for(int a=0;a<data->list->n;a++){
      AstNode* e=(AstNode*)get_from_list(data->list,a);
      if(e) dealloc_ast_type(e);
    }
    dealloc_list(data->list);
    free(data);
  }
  if(node->type==AST_TYPE_TUPLE){
    List* ls=(List*)(node->data);
    for(int a=0;a<ls->n;a++){
      AstNode* e=(AstNode*)get_from_list(ls,a);
      dealloc_ast_type(e);
    }
    dealloc_list(ls);
  }
  free(node);
}
void dealloc_ast_node(AstNode* node){
  if(node->type==AST_TYPE_ANY || node->type==AST_TYPE_FUNC || node->type==AST_TYPE_BASIC || node->type==AST_TYPE_TUPLE){
    dealloc_ast_type(node);
    return;
  }
  if(node->type==AST_STMT || node->type==AST_DO){
    List* ls=(List*)(node->data);
    for(int a=0;a<ls->n;a++){
      AstNode* e=(AstNode*)get_from_list(ls,a);
      dealloc_ast_node(e);
    }
    dealloc_list(ls);
  }
  else if(node->type==AST_LTUPLE){
    dealloc_list((List*)(node->data));
  }
  else if(node->type==AST_RETURN || node->type==AST_PAREN || node->type==AST_REQUIRE){
    if(node->data) dealloc_ast_node((AstNode*)(node->data));
  }
  else if(node->type==AST_PRIMITIVE || node->type==AST_FIELD || node->type==AST_LOCAL || node->type==AST_TYPEDEF){
    StringAstNode* data=(StringAstNode*)(node->data);
    dealloc_ast_node(data->node);
    free(data);
  }
  else if(node->type==AST_INTERFACE){
    InterfaceNode* data=(InterfaceNode*)(node->data);
    dealloc_ast_type(data->type);
    for(int a=0;a<data->ls->n;a++){
      AstNode* e=get_from_list(data->ls,a);
      dealloc_ast_node(e);
    }
    dealloc_list(data->ls);
    free(data);
  }
  else if(node->type==AST_CLASS){
    ClassNode* data=(ClassNode*)(node->data);
    dealloc_list(data->interfaces);
    dealloc_ast_type(data->type);
    for(int a=0;a<data->ls->n;a++){
      AstNode* e=get_from_list(data->ls,a);
      dealloc_ast_node(e);
    }
    dealloc_list(data->ls);
    free(data);
  }
  else if(node->type==AST_FUNCTION){
    FunctionNode* data=(FunctionNode*)(node->data);
    if(data->functype) free(data->functype);//dealloc_ast_type(data->functype);
    if(data->type) dealloc_ast_type(data->type);
    if(data->name) dealloc_ast_node(data->name);
    if(data->body){
      for(int a=0;a<data->body->n;a++){
        AstNode* e=(AstNode*)get_from_list(data->body,a);
        dealloc_ast_node(e);
      }
      dealloc_list(data->body);
    }
    for(int a=0;a<data->args->n;a++){
      StringAstNode* e=(StringAstNode*)get_from_list(data->args,a);
      if(e->node) dealloc_ast_node(e->node);
      free(e);
    }
    dealloc_list(data->args);
    free(data);
  }
  else if(node->type==AST_BINARY || node->type==AST_UNARY || node->type==AST_DEFINE){
    BinaryNode* data=(BinaryNode*)(node->data);
    if(data->l) dealloc_ast_node(data->l);
    if(data->r) dealloc_ast_node(data->r);
    free(data);
  }
  else if(node->type==AST_REPEAT || node->type==AST_WHILE || node->type==AST_IF || node->type==AST_TUPLE){
    AstListNode* data=(AstListNode*)(node->data);
    if(data->node) dealloc_ast_node(data->node);
    for(int a=0;a<data->list->n;a++){
      AstNode* e=get_from_list(data->list,a);
      dealloc_ast_node(e);
    }
    dealloc_list(data->list);
    free(data);
  }
  else if(node->type==AST_CALL || node->type==AST_SET || node->type==AST_SUB){
    AstAstNode* data=(AstAstNode*)(node->data);
    if(data->r) dealloc_ast_node(data->r);
    dealloc_ast_node(data->l);
    free(data);
  }
  else if(node->type==AST_TABLE){
    TableNode* data=(TableNode*)(node->data);
    for(int a=0;a<data->vals->n;a++){
      AstNode* e=(AstNode*)get_from_list(data->vals,a);
      dealloc_ast_node(e);
    }
    dealloc_list(data->vals);
    dealloc_list(data->keys);
    free(data);
  }
  else if(node->type==AST_FORNUM){
    FornumNode* data=(FornumNode*)(node->data);
    if(data->num3) dealloc_ast_node(data->num3);
    dealloc_ast_node(data->num1);
    dealloc_ast_node(data->num2);
    for(int a=0;a<data->body->n;a++){
      AstNode* e=(AstNode*)get_from_list(data->body,a);
      dealloc_ast_node(e);
    }
    dealloc_list(data->body);
    free(data);
  }
  else if(node->type==AST_FORIN){
    ForinNode* data=(ForinNode*)(node->data);
    dealloc_ast_node(data->tuple);
    dealloc_ast_node(data->lhs);
    for(int a=0;a<data->body->n;a++){
      AstNode* e=(AstNode*)get_from_list(data->body,a);
      dealloc_ast_node(e);
    }
    dealloc_list(data->body);
    free(data);
  }
  free(node);
}

// Node constructors
AstNode* new_node(int type,void* data){
  AstNode* node=(AstNode*)malloc(sizeof(AstNode));
  node->type=type;
  node->data=data;
  return node;
}
FunctionNode* new_function_node(AstNode* name,AstNode* type,List* args,List* body){
  FunctionNode* node=(FunctionNode*)malloc(sizeof(FunctionNode));
  node->functype=NULL;
  node->name=name;
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
  node->name=name;
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
  node->text=text;
  node->r=r;
  node->l=l;
  return node;
}
InterfaceNode* new_interface_node(char* name,char* parent,List* ls){
  InterfaceNode* node=(InterfaceNode*)malloc(sizeof(InterfaceNode));
  node->type=new_node(AST_TYPE_BASIC,name);
  node->parent=parent;
  node->name=name;
  node->ls=ls;
  return node;
}
ClassNode* new_class_node(char* name,char* parent,List* interfaces,List* ls){
  ClassNode* node=(ClassNode*)malloc(sizeof(ClassNode));
  node->type=new_node(AST_TYPE_BASIC,name);
  node->interfaces=interfaces;
  node->parent=parent;
  node->name=name;
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
