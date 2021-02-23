#include "./tokens.h"
#include "./types.h"
#include <stdio.h>

// Preemptive function declarations
static void* process_stmt(AstNode* node);
static void* process_function(AstNode* node);
static void* process_repeat(AstNode* node);
static void* process_while(AstNode* node);
static void* process_if(AstNode* node);
static void* process_do(AstNode* node);

// Public interface
void traverse(AstNode* root){
  process_stmt(root);
}

// Node to function switch
void* process_node(AstNode* node){
  int type=node->type;
  switch(node->type){
    case AST_STMT: return (void*)process_stmt(node);
    case AST_FUNCTION: return (void*)process_function(node);
    case AST_REPEAT: return (void*)process_repeat(node);
    case AST_WHILE: return (void*)process_while(node);
    case AST_IF: return (void*)process_if(node);
    case AST_DO: return (void*)process_do(node);
    default: printf("Uh oh, you shouldn't be here\n");
  }
  return NULL;
}

// Control
static void* process_stmt(AstNode* node){
  List* ls=(List*)(node->data);
  printf("stmt node\n");
  for(int a=0;a<ls->n;a++){
    process_node((AstNode*)get_from_list(ls,a));
  }
  printf("stmt end\n");
  return NULL;
}
static void* process_function(AstNode* node){
  FunctionNode* data=(FunctionNode*)(node->data);
  printf("function node\n");
  printf("function end\n");
  return NULL;
}
static void* process_repeat(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  printf("repeat node\n");
  printf("repeat end\n");
  return NULL;
}
static void* process_while(AstNode* node){
  AstListNode* ls=(AstListNode*)(node->data);
  printf("while node\n");
  printf("while end\n");
  return NULL;
}
static void* process_if(AstNode* node){
  AstListNode* ls=(AstListNode*)(node->data);
  printf("if node\n");
  printf("if end\n");
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
