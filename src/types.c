#include "./internal.h"
#include <stdlib.h>
#include <string.h>
static List* types_registry;
static List* types_graph;

// TODO implement function parameter types (also function types in general)

// Init/dealloc
void init_types(){
  types_graph=new_default_list();
  types_registry=new_default_list();
  register_primitive(PRIMITIVE_STRING);
  register_primitive(PRIMITIVE_FLOAT);
  register_primitive(PRIMITIVE_BOOL);
  register_primitive(PRIMITIVE_INT);
  register_primitive(PRIMITIVE_NIL);
}
void dealloc_types(){
  dealloc_list(types_registry);
  dealloc_list(types_graph);
}

// Type matching for AST traversal
static int is_primitive(AstNode* node,const char* type){
  return node->type==AST_TYPE_BASIC && !strcmp((char*)(node->data),type);
}
AstNode* get_type(AstNode* node){ // TODO implement the commented branches
  int type=node->type;
  switch(node->type){
    //case AST_FUNCTION: return process_function(node);
    //case AST_CALL: return process_call(node);
    case AST_PRIMITIVE: return ((StringAstNode*)(node->data))->node;
    case AST_PAREN: return get_type((AstNode*)(node->data));
    case AST_UNARY: return ((BinaryNode*)(node->data))->r;
    case AST_TUPLE: return ((AstListNode*)(node->data))->node;
    case AST_SUB: return new_node(AST_TYPE_ANY,NULL);
    case AST_FIELD:{
      // Check if get_type((StringAtNode*)node->data->ast) is a class or interface
      return new_node(AST_TYPE_ANY,NULL);
    }
    case AST_BINARY:{
      BinaryNode* data=(BinaryNode*)(node->data);
      AstNode* tl=get_type(data->l);
      AstNode* tr=get_type(data->r);
      if(!strcmp(data->text,"..")){
        if(is_primitive(tl,PRIMITIVE_STRING) && is_primitive(tr,PRIMITIVE_STRING)) return tl;
        return new_node(AST_TYPE_ANY,NULL);
      }
      if(!strcmp(data->text,"/")){
        return new_node(AST_TYPE_BASIC,new_string_node(PRIMITIVE_FLOAT));
      }
      if(!strcmp(data->text,"+") || !strcmp(data->text,"-") || !strcmp(data->text,"*")){
        if(is_primitive(tl,PRIMITIVE_FLOAT)) return tl;
        if(is_primitive(tr,PRIMITIVE_FLOAT)) return tr;
        return new_node(AST_TYPE_BASIC,new_string_node(PRIMITIVE_INT));
      }
      return new_node(AST_TYPE_BASIC,new_string_node(PRIMITIVE_BOOL));
    }
    case AST_ID:{
      char* name=(char*)(node->data);
      BinaryNode* var=get_scoped_var(name);
      return var?(var->l):NULL;
    }
    default: return NULL;
  }
}
static int typed_match_no_equivalence(AstNode* l,AstNode* r){ // TODO implement function types
  if(l->type==AST_TYPE_ANY) return 1;
  if(!r) return 0;
  if(is_primitive(r,PRIMITIVE_NIL)) return 1;
  if(is_primitive(l,PRIMITIVE_FLOAT) && is_primitive(r,PRIMITIVE_INT)) return 1;
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
int typed_match(AstNode* l,AstNode* r){
  if(!l) return 1;
  if(r && l->type==AST_TYPE_BASIC && types_equivalent((char*)(l->data),r)) return 1;
  return typed_match_no_equivalence(l,r);
}

// Type registry
void register_type(char* name){
  add_to_list(types_registry,name);
}
void register_primitive(const char* name){
  char* type=(char*)malloc(sizeof(char)*(strlen(name)+1));
  strcpy(type,name);
  add_to_list(types_registry,type);
}
int type_exists(char* name){
  for(int a=0;a<types_registry->n;a++){
    if(!strcmp((char*)get_from_list(types_registry,a),name)) return 1;
  }
  return 0;
}
int compound_type_exists(AstNode* node){
  switch(node->type){
    case AST_TYPE_ANY: return 1;
    case AST_TYPE_BASIC: return type_exists((char*)(node->data));
    case AST_TYPE_TUPLE:{
      List* ls=(List*)(node->data);
      for(int a=0;a<ls->n;a++){
        if(!compound_type_exists((AstNode*)get_from_list(ls,a))) return 0;
      }
      return 1;
    }
    case AST_TYPE_FUNC:{
      // TODO implement function type
    }
    default: return 0;
  }
}

// Type equivalence graph
void add_type_equivalence(char* name,AstNode* type){
  // TODO prevent graph cycles
  add_to_list(types_graph,new_string_ast_node(name,type));
}
List* get_equivalent_types(char* name){
  List* ls=new_default_list();
  for(int a=0;a<types_graph->n;a++){
    StringAstNode* node=(StringAstNode*)get_from_list(types_graph,a);
    if(!strcmp(node->text,name)) add_to_list(ls,node->node);
  }
  return ls;
}
int types_equivalent(char* name,AstNode* type){
  if(type->type==AST_TYPE_BASIC && !strcmp(name,(char*)(type->data))) return 1;
  List* ls=get_equivalent_types(name);
  int a=0;
  while(a<ls->n){
    AstNode* node=get_from_list(ls,a);
    if(typed_match_no_equivalence(node,type)){
      dealloc_list(ls);
      return 1;
    }
    if(node->type==AST_TYPE_BASIC){
      List* ls1=get_equivalent_types((char*)(node->data));
      for(int b=0;b<ls1->n;b++) add_to_list(ls,get_from_list(ls1,b));
      dealloc_list(ls1);
    }
    a++;
  }
  dealloc_list(ls);
  return 0;
}

// Type stringification
static void stringify_type_internal(List* ls,AstNode* node){
  if(!node || node->type==AST_TYPE_ANY){
    add_to_list(ls,"var");
  }else if(node->type==AST_TYPE_BASIC){
    add_to_list(ls,node->data);
  }else if(node->type==AST_TYPE_TUPLE){
    List* tls=(List*)(node->data);
    add_to_list(ls,"(");
    for(int a=0;a<tls->n;a++){
      if(a) add_to_list(ls,",");
      stringify_type_internal(ls,(AstNode*)get_from_list(tls,a));
    }
    add_to_list(ls,")");
  }else if(node->type==AST_TYPE_FUNC){
    AstListNode* data=(AstListNode*)(node->data);
    if(ls->n==0 || strcmp((char*)get_from_list(ls,ls->n-1),"*")){
      add_to_list(ls,"*");
    }
    stringify_type_internal(ls,data->node);
    add_to_list(ls,"(");
    for(int a=0;a<data->list->n;a++){
      if(a) add_to_list(ls,",");
      stringify_type_internal(ls,(AstNode*)get_from_list(data->list,a));
    }
    add_to_list(ls,")");
  }
}
char* stringify_type(AstNode* node){
  int len=1;
  List* ls=new_default_list();
  stringify_type_internal(ls,node);
  for(int a=0;a<ls->n;a++) len+=strlen((char*)get_from_list(ls,a));
  char* type=(char*)malloc(sizeof(char)+len);
  type[0]=0;
  for(int a=0;a<ls->n;a++) strcat(type,(char*)get_from_list(ls,a));
  dealloc_list(ls);
  return type;
}
