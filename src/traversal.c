#include "./tokens.h"
#include "./types.h"
#include <stdio.h>

// Preemptive function declarations
static void* process_stmt(AstNode* node);
static void* process_primitive(AstNode* node);
static void* process_function(AstNode* node);
static void* process_repeat(AstNode* node);
static void* process_ltuple(AstNode* node);
static void* process_return(AstNode* node);
static void* process_binary(AstNode* node);
static void* process_fornum(AstNode* node);
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
    case AST_FUNCTION: return process_function(node);
    case AST_REPEAT: return process_repeat(node);
    case AST_LTUPLE: return process_ltuple(node);
    case AST_RETURN: return process_return(node);
    case AST_BINARY: return process_binary(node);
    case AST_FORNUM: return process_fornum(node);
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

// Statement group
static void* process_stmt(AstNode* node){
  List* ls=(List*)(node->data);
  printf("stmt node\n");
  for(int a=0;a<ls->n;a++){
    process_node((AstNode*)get_from_list(ls,a));
  }
  printf("stmt end\n");
  return NULL;
}
static void* process_do(AstNode* node){
  List* ls=(List*)(node->data);
  printf("do node\n");
  for(int a=0;a<ls->n;a++){
    process_node((AstNode*)get_from_list(ls,a));
  }
  printf("do end\n");
  return NULL;
}

// Statement
static void* process_call(AstNode* node){
  AstAstNode* data=(AstAstNode*)(node->data);
  printf("invocation node left\n");
  process_node(data->l);
  printf("invocation end left\n");
  printf("invocation node right\n");
  if(data->r) process_node(data->r);
  printf("invocation end right\n");
  return NULL;
}
static void* process_set(AstNode* node){
  AstAstNode* data=(AstAstNode*)(node->data);
  printf("set node left\n");
  process_node(data->l);
  printf("set end left\n");
  printf("set node right\n");
  process_node(data->r);
  printf("set end right\n");
  return NULL;
}
static void* process_return(AstNode* node){
  printf("return node\n");
  process_node((AstNode*)(node->data));
  printf("return end\n");
  return NULL;
}
static void* process_ltuple(AstNode* node){
  List* ls=(List*)(node->data);
  printf("ltuple node\n");
  for(int a=0;a<ls->n;a++){
    printf("%s\n",(char*)get_from_list(ls,a));
  }
  printf("ltuple end\n");
  return NULL;
}
static void* process_field(AstNode* node){
  StringAstNode* data=(StringAstNode*)(node->data);
  printf("field node %s\n",data->text);
  process_node(data->node);
  printf("field end\n");
  return NULL;
}
static void* process_sub(AstNode* node){
  AstAstNode* data=(AstAstNode*)(node->data);
  printf("sub node left\n");
  process_node(data->l);
  printf("sub end left\n");
  printf("sub node right\n");
  process_node(data->r);
  printf("sub end right\n");
  return NULL;
}
static void* process_id(AstNode* node){
  printf("id %s\n",(char*)(node->data));
  return NULL;
}
static void* process_local(AstNode* node){
  StringAstNode* data=(StringAstNode*)(node->data);
  printf("local node %s\n",data->text);
  process_node(data->node);
  printf("local node end\n");
  return NULL;
}

// Control
static void* process_function(AstNode* node){
  FunctionNode* data=(FunctionNode*)(node->data);
  printf("function node\n");
  printf("function end\n");
  return NULL;
}
static void* process_repeat(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  printf("repeat node body\n");
  for(int a=0;a<data->list->n;a++){
    process_node((AstNode*)get_from_list(data->list,a));
  }
  printf("repeat end body\n");
  printf("repeat node expr\n");
  process_node(data->node);
  printf("repeat end expr\n");
  return NULL;
}
static void* process_while(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  printf("while node expr\n");
  process_node(data->node);
  printf("while end expr\n");
  printf("while node body\n");
  for(int a=0;a<data->list->n;a++){
    process_node((AstNode*)get_from_list(data->list,a));
  }
  printf("if end body\n");
  return NULL;
}
static void* process_if(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  printf("if node expr\n");
  process_node(data->node);
  printf("if end expr\n");
  printf("if node body\n");
  for(int a=0;a<data->list->n;a++){
    process_node((AstNode*)get_from_list(data->list,a));
  }
  printf("if end body\n");
  return NULL;
}
static void* process_fornum(AstNode* node){
  FornumNode* data=(FornumNode*)(node->data);
  printf("fornum %s\n",data->name);
  process_node(data->num1);
  process_node(data->num2);
  if(data->num3) process_node(data->num3);
  printf("fornum body\n");
  for(int a=0;a<data->body->n;a++){
    process_node((AstNode*)get_from_list(data->body,a));
  }
  printf("fornum end\n");
  return NULL;
}
static void* process_forin(AstNode* node){
  ForinNode* data=(ForinNode*)(node->data);
  printf("forin node lhs\n");
  process_node(data->lhs);
  printf("forin end lhs\n");
  printf("forin node lhs\n");
  process_node(data->tuple);
  printf("forin end lhs\n");
  printf("forin body\n");
  for(int a=0;a<data->body->n;a++){
    process_node((AstNode*)get_from_list(data->body,a));
  }
  printf("forin end\n");
  return NULL;
}
static void* process_label(AstNode* node){
  printf("label %s\n",(char*)(node->data));
  return NULL;
}
static void* process_goto(AstNode* node){
  printf("goto %s\n",(char*)(node->data));
  return NULL;
}

// Primitives
static void* process_primitive(AstNode* node){
  ValueNode* data=(ValueNode*)(node->data);
  printf("%s %s\n",data->type,data->text);
}
static void* process_table(AstNode* node){
  TableNode* data=(TableNode*)(node->data);
  printf("table node\n");
  for(int a=0;a<data->keys->n;a++){
    printf("key node %s\n",(char*)get_from_list(data->keys,a));
    process_node((AstNode*)get_from_list(data->vals,a));
    printf("key end\n");
  }
  printf("table end\n");
}
static void* process_tuple(AstNode* node){
  List* ls=(List*)(node->data);
  printf("tuple node\n");
  for(int a=0;a<ls->n;a++){
    process_node((AstNode*)get_from_list(ls,a));
  }
  printf("tuple end\n");
}

// Expressions
static void* process_unary(AstNode* node){
  StringAstNode* data=(StringAstNode*)(node->data);
  printf("unary node %s\n",data->text);
  process_node(data->node);
  printf("unary end\n");
  return NULL;
}
static void* process_binary(AstNode* node){
  BinaryNode* data=(BinaryNode*)(node->data);
  printf("binary node %s\n",data->text);
  printf("binary node left\n");
  process_node(data->l);
  printf("binary end left\n");
  printf("binary node right\n");
  process_node(data->r);
  printf("binary end right\n");
  printf("binary end\n");
  return NULL;
}
static void* process_paren(AstNode* node){
  process_node((AstNode*)(node->data));
}
