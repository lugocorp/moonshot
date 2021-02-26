#include "./enums.h"
#include "./types.h"
#include <stdio.h>

// Preemptive function declarations
static void* process_stmt(AstNode* node);
static void* process_type(AstNode* node);
static void* process_define(AstNode* node);
static void* process_typedef(AstNode* node);
static void* process_primitive(AstNode* node);
static void* process_interface(AstNode* node);
static void* process_class(AstNode* node);
static void* process_function(AstNode* node);
static void* process_repeat(AstNode* node);
static void* process_ltuple(AstNode* node);
static void* process_return(AstNode* node);
static void* process_binary(AstNode* node);
static void* process_fornum(AstNode* node);
static void* process_break(AstNode* node);
static void* process_paren(AstNode* node);
static void* process_forin(AstNode* node);
static void* process_unary(AstNode* node);
static void* process_tuple(AstNode* node);
static void* process_table(AstNode* node);
static void* process_local(AstNode* node);
static void* process_while(AstNode* node);
static void* process_field(AstNode* node);
static void* process_label(AstNode* node);
static void* process_goto(AstNode* node);
static void* process_call(AstNode* node);
static void* process_set(AstNode* node);
static void* process_sub(AstNode* node);
static void* process_if(AstNode* node);
static void* process_do(AstNode* node);
static void* process_id(AstNode* node);

// Public interface
void traverse(AstNode* root){
  process_stmt(root);
}

// Node to function switch
void* process_node(AstNode* node){
  int type=node->type;
  switch(node->type){
    case AST_STMT: return process_stmt(node);
    case AST_PRIMITIVE: return process_primitive(node);
    case AST_INTERFACE: return process_interface(node);
    case AST_FUNCTION: return process_function(node);
    case AST_TYPEDEF: return process_typedef(node);
    case AST_DEFINE: return process_define(node);
    case AST_REPEAT: return process_repeat(node);
    case AST_LTUPLE: return process_ltuple(node);
    case AST_RETURN: return process_return(node);
    case AST_BINARY: return process_binary(node);
    case AST_FORNUM: return process_fornum(node);
    case AST_CLASS: return process_class(node);
    case AST_BREAK: return process_break(node);
    case AST_FORIN: return process_forin(node);
    case AST_PAREN: return process_paren(node);
    case AST_UNARY: return process_unary(node);
    case AST_TUPLE: return process_tuple(node);
    case AST_TABLE: return process_table(node);
    case AST_LOCAL: return process_local(node);
    case AST_WHILE: return process_while(node);
    case AST_FIELD: return process_field(node);
    case AST_LABEL: return process_label(node);
    case AST_GOTO: return process_goto(node);
    case AST_CALL: return process_call(node);
    case AST_SET: return process_set(node);
    case AST_SUB: return process_sub(node);
    case AST_IF: return process_if(node);
    case AST_DO: return process_do(node);
    case AST_ID: return process_id(node);
    default: printf("Uh oh, you shouldn't be here (%i)\n",node->type);
  }
  return NULL;
}

// Extended grammar
static void* process_type(AstNode* node){
  if(node->type==AST_TYPE_ANY){
    printf("var");
  }else if(node->type==AST_TYPE_BASIC){
    char* name=(char*)(node->data);
    printf("%s",name);
  }else if(node->type==AST_TYPE_TUPLE){
    List* ls=(List*)(node->data);
    printf("(");
    for(int a=0;a<ls->n;a++){
      if(a) printf(",");
      process_type((AstNode*)get_from_list(ls,a));
    }
    printf(")");
  }else if(node->type==AST_TYPE_FUNC){
    AstListNode* data=(AstListNode*)(node->data);
    process_type(data->node);
    printf("(");
    for(int a=0;a<data->list->n;a++){
      if(a) printf(",");
      process_type((AstNode*)get_from_list(data->list,a));
    }
    printf(")");
  }
  return NULL;
}
static void* process_define(AstNode* node){
  BinaryNode* data=(BinaryNode*)(node->data);
  process_type(data->l);
  printf(" %s",data->text);
  if(data->r){
    printf("=");
    process_node(data->r);
  }
  printf("\n");
  return NULL;
}
static void* process_typedef(AstNode* node){
  StringAstNode* data=(StringAstNode*)(node->data);
  printf("typedef %s -> ",data->text);
  process_type(data->node);
  printf("\n");
  return NULL;
}
static void* process_interface(AstNode* node){
  InterfaceNode* data=(InterfaceNode*)(node->data);
  printf("interface %s ",data->name);
  if(data->parent[0]) printf("extends %s ",data->parent);
  printf("where\n");
  for(int a=0;a<data->ls->n;a++){
    process_node((AstNode*)get_from_list(data->ls,a));
  }
  printf("end\n");
  return NULL;
}
static void* process_class(AstNode* node){
  ClassNode* data=(ClassNode*)(node->data);
  printf("class %s ",data->name);
  if(data->parent[0]) printf("extends %s ",data->parent);
  if(data->interfaces->n){
    printf("implements ");
    for(int a=0;a<data->interfaces->n;a++){
      if(a) printf(",");
      printf("%s",(char*)get_from_list(data->interfaces,a));
    }
    printf(" ");
  }
  printf("where\n");
  for(int a=0;a<data->ls->n;a++){
    process_node((AstNode*)get_from_list(data->ls,a));
  }
  printf("end\n");
  return NULL;
}

// Statement group
static void* process_stmt(AstNode* node){
  List* ls=(List*)(node->data);
  for(int a=0;a<ls->n;a++){
    process_node((AstNode*)get_from_list(ls,a));
  }
  return NULL;
}
static void* process_do(AstNode* node){
  List* ls=(List*)(node->data);
  printf("do\n");
  for(int a=0;a<ls->n;a++){
    process_node((AstNode*)get_from_list(ls,a));
  }
  printf("end\n");
  return NULL;
}

// Statement
static void* process_call(AstNode* node){
  AstAstNode* data=(AstAstNode*)(node->data);
  process_node(data->l);
  printf("(");
  if(data->r) process_node(data->r);
  printf(")\n");
  return NULL;
}
static void* process_set(AstNode* node){
  AstAstNode* data=(AstAstNode*)(node->data);
  process_node(data->l);
  printf("=");
  process_node(data->r);
  printf("\n");
  return NULL;
}
static void* process_return(AstNode* node){
  printf("return ");
  process_node((AstNode*)(node->data));
  printf("\n");
  return NULL;
}
static void* process_ltuple(AstNode* node){
  List* ls=(List*)(node->data);
  for(int a=0;a<ls->n;a++){
    if(a) printf(",");
    printf("%s",(char*)get_from_list(ls,a));
  }
  return NULL;
}
static void* process_field(AstNode* node){
  StringAstNode* data=(StringAstNode*)(node->data);
  process_node(data->node);
  printf(".%s",data->text);
  return NULL;
}
static void* process_sub(AstNode* node){
  AstAstNode* data=(AstAstNode*)(node->data);
  process_node(data->l);
  printf("[");
  process_node(data->r);
  printf("]");
  return NULL;
}
static void* process_id(AstNode* node){
  printf("%s",(char*)(node->data));
  return NULL;
}
static void* process_local(AstNode* node){
  StringAstNode* data=(StringAstNode*)(node->data);
  printf("local %s",data->text);
  if(data->node){
    printf("=");
    process_node(data->node);
  }
  printf("\n");
  return NULL;
}

// Control
static void* process_function(AstNode* node){
  FunctionNode* data=(FunctionNode*)(node->data);
  if(data->type->type!=AST_TYPE_ANY){
    process_type(data->type);
    printf(" ");
  }
  printf("function");
  if(data->name[0]) printf(" %s",data->name);
  printf("(");
  for(int a=0;a<data->args->n;a++){
    if(a) printf(",");
    printf("%s",(char*)get_from_list(data->args,a));
  }
  printf(")\n");
  if(data->body){
    for(int a=0;a<data->body->n;a++){
      process_node((AstNode*)get_from_list(data->body,a));
    }
    printf("end\n");
  }
  return NULL;
}
static void* process_repeat(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  printf("repeat\n");
  for(int a=0;a<data->list->n;a++){
    process_node((AstNode*)get_from_list(data->list,a));
  }
  printf("until ");
  process_node(data->node);
  printf("\n");
  return NULL;
}
static void* process_while(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  printf("while ");
  process_node(data->node);
  printf(" do\n");
  for(int a=0;a<data->list->n;a++){
    process_node((AstNode*)get_from_list(data->list,a));
  }
  printf("end\n");
  return NULL;
}
static void* process_if(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  printf("if ");
  process_node(data->node);
  printf(" then\n");
  for(int a=0;a<data->list->n;a++){
    process_node((AstNode*)get_from_list(data->list,a));
  }
  printf("end\n");
  return NULL;
}
static void* process_fornum(AstNode* node){
  FornumNode* data=(FornumNode*)(node->data);
  printf("for %s=",data->name);
  process_node(data->num1);
  printf(",");
  process_node(data->num2);
  if(data->num3){
    printf(",");
    process_node(data->num3);
  }
  printf(" do\n");
  for(int a=0;a<data->body->n;a++){
    process_node((AstNode*)get_from_list(data->body,a));
  }
  printf("end\n");
  return NULL;
}
static void* process_forin(AstNode* node){
  ForinNode* data=(ForinNode*)(node->data);
  printf("for ");
  process_node(data->lhs);
  printf(" in ");
  process_node(data->tuple);
  printf(" do\n");
  for(int a=0;a<data->body->n;a++){
    process_node((AstNode*)get_from_list(data->body,a));
  }
  printf("end\n");
  return NULL;
}
static void* process_break(AstNode* node){
  printf("break\n");
  return NULL;
}
static void* process_label(AstNode* node){
  printf("::%s::\n",(char*)(node->data));
  return NULL;
}
static void* process_goto(AstNode* node){
  printf("goto %s\n",(char*)(node->data));
  return NULL;
}

// Primitives
static void* process_primitive(AstNode* node){
  ValueNode* data=(ValueNode*)(node->data);
  printf("%s",data->text);
  return NULL;
}
static void* process_table(AstNode* node){
  TableNode* data=(TableNode*)(node->data);
  printf("{");
  for(int a=0;a<data->keys->n;a++){
    if(a) printf(",");
    printf("%s=",(char*)get_from_list(data->keys,a));
    process_node((AstNode*)get_from_list(data->vals,a));
  }
  printf("}");
  return NULL;
}
static void* process_tuple(AstNode* node){
  List* ls=(List*)(node->data);
  for(int a=0;a<ls->n;a++){
    if(a) printf(",");
    process_node((AstNode*)get_from_list(ls,a));
  }
  return NULL;
}

// Expressions
static void* process_unary(AstNode* node){
  StringAstNode* data=(StringAstNode*)(node->data);
  printf("(%s ",data->text);
  process_node(data->node);
  printf(")");
  return NULL;
}
static void* process_binary(AstNode* node){
  BinaryNode* data=(BinaryNode*)(node->data);
  printf("(");
  process_node(data->l);
  printf(" %s ",data->text);
  process_node(data->r);
  printf(")");
  return NULL;
}
static void* process_paren(AstNode* node){
  printf("(");
  process_node((AstNode*)(node->data));
  printf(")");
  return NULL;
}
