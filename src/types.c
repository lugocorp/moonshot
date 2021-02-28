#include "./moonshot.h"
#include <stdlib.h>
#include <string.h>

// TODO implement subtype graph for classes, interfaces and typedefs
// TODO implement function parameter types (also function types in general)

// Type nodes
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
int typed_match(AstNode* l,AstNode* r){ // TODO implement function types
  // FUNC
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
