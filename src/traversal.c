#include "./internal.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define ERROR(cond,msg,...) if(cond){add_error(-1,msg,__VA_ARGS__);return;}
static FILE* _output;
static int validate;
AstNode* float_type;
AstNode* bool_type;
AstNode* int_type;
AstNode* any_type;

// Traversal interface
AstNode* float_type_const(){
  return float_type;
}
AstNode* bool_type_const(){
  return bool_type;
}
AstNode* int_type_const(){
  return int_type;
}
AstNode* any_type_const(){
  return any_type;
}
void init_traverse(){
  float_type=new_node(AST_TYPE_BASIC,PRIMITIVE_FLOAT);
  bool_type=new_node(AST_TYPE_BASIC,PRIMITIVE_BOOL);
  int_type=new_node(AST_TYPE_BASIC,PRIMITIVE_INT);
  any_type=new_node(AST_TYPE_ANY,NULL);
}
void traverse(AstNode* root,int valid){
  validate=valid;
  if(validate){
    preempt_scopes();
    init_types();
    init_scopes();
    push_scope();
  }
  process_stmt(root);
}
void dealloc_traverse(){
  pop_scope();
  dealloc_scopes();
  dealloc_types();
  free(any_type);
  free(int_type);
  free(bool_type);
  free(float_type);
}

// Output
void set_output(FILE* output){
  _output=output;
}
static void write(const char* msg,...){
  if(validate) return;
  va_list args;
  int n=strlen(msg);
  va_start(args,msg);
  for(int a=0;a<n;a++){
    if(a<n-1 && msg[a]=='%'){
      if(msg[a+1]=='s'){
        fprintf(_output,"%s",va_arg(args,char*));
        a++;
        continue;
      }
    }
    fprintf(_output,"%c",msg[a]);
  }
  va_end(args);
}

// Node to function switch
void process_node(AstNode* node){
  switch(node->type){
    case AST_STMT: process_stmt(node); return;
    case AST_PRIMITIVE: process_primitive(node); return;
    case AST_INTERFACE: process_interface(node); return;
    case AST_FUNCTION: process_function(node); return;
    case AST_TYPEDEF: process_typedef(node); return;
    case AST_REQUIRE: process_require(node); return;
    case AST_DEFINE: process_define(node); return;
    case AST_REPEAT: process_repeat(node); return;
    case AST_LTUPLE: process_ltuple(node); return;
    case AST_RETURN: process_return(node); return;
    case AST_BINARY: process_binary(node); return;
    case AST_FORNUM: process_fornum(node); return;
    case AST_CLASS: process_class(node); return;
    case AST_BREAK: process_break(node); return;
    case AST_FORIN: process_forin(node); return;
    case AST_PAREN: process_paren(node); return;
    case AST_UNARY: process_unary(node); return;
    case AST_TUPLE: process_tuple(node); return;
    case AST_TABLE: process_table(node); return;
    case AST_LOCAL: process_local(node); return;
    case AST_WHILE: process_while(node); return;
    case AST_FIELD: process_field(node); return;
    case AST_LABEL: process_label(node); return;
    case AST_GOTO: process_goto(node); return;
    case AST_CALL: process_call(node); return;
    case AST_SET: process_set(node); return;
    case AST_SUB: process_sub(node); return;
    case AST_IF: process_if(node); return;
    case AST_DO: process_do(node); return;
    case AST_ID: process_id(node); return;
    default: add_error(-1,"invalid Moonshot AST detected (node ID %i)\n",node->type);
  }
}

// Extended grammar
void process_define(AstNode* node){
  BinaryNode* data=(BinaryNode*)(node->data);
  if(validate){
    ERROR(!compound_type_exists(data->l),"reference to nonexistent type %t",data->l);
    if(data->r){
      AstNode* tr=get_type(data->r);
      ERROR(!typed_match(data->l,tr),"expression of type %t cannot be assigned to variable of type %t",tr,data->l);
    }
    ERROR(!add_scoped_var(data),"variable %s was already declared in this scope",data->text);
  }
  write("local %s=",data->text);
  if(data->r) process_node(data->r);
  else write("nil");
  write("\n");
}
void process_typedef(AstNode* node){
  StringAstNode* data=(StringAstNode*)(node->data);
  if(validate){
    ERROR(type_exists(data->text),"type %s is already declared",data->text);
    ERROR(!compound_type_exists(data->node),"type %t does not exist",data->node);
    ERROR(!add_type_equivalence(data->text,data->node,RL_EQUALS),"co-dependent typedef %s detected",data->text);
    register_type(data->text);
  }
  /*char* type1=stringify_type(data->node);
  write("-- typedef %s -> %s\n",data->text,type1);
  free(type1);*/
}
void process_interface(AstNode* node){
  InterfaceNode* data=(InterfaceNode*)(node->data);
  if(validate){
    if(data->parent){
      ERROR(!interface_exists(data->parent),"parent interface %s does not exist",data->parent);
      ERROR(!add_child_type(data->name,data->parent,RL_EXTENDS),"co-dependent interface %s detected",data->name);
    }
    ERROR(type_exists(data->name),"type %s is already declared",data->name);
    register_type(data->name);
    register_interface(data);
  }
}
void process_class(AstNode* node){
  ClassNode* data=(ClassNode*)(node->data);
  if(validate){
    push_scope();
    if(data->parent){
      ERROR(!class_exists(data->parent),"parent class %s does not exist",data->parent);
      ERROR(!add_child_type(data->name,data->parent,RL_EXTENDS),"co-dependent class %s detected",data->name);
    }
    for(int a=0;a<data->interfaces->n;a++){
      char* interface=(char*)get_from_list(data->interfaces,a);
      ERROR(!interface_exists(interface),"interface %s does not exist",interface);
      add_child_type(data->name,interface,RL_IMPLEMENTS);
    }
    ERROR(type_exists(data->name),"type %s is already declared",data->name);
    int num_cons=num_constructors(data);
    ERROR(num_cons>1,"class %s has %i constructors, should have only 1",data->name,num_cons);
    List* missing=get_missing_class_methods(data);
    if(missing->n){
      for(int a=0;a<missing->n;a++){
        FunctionNode* f=(FunctionNode*)get_from_list(missing,a);
        add_error(-1,"Class %s does not implement method %s",data->name,(char*)(f->name->data));
      }
      dealloc_list(missing);
      return;
    }
    for(int a=0;a<data->ls->n;a++){
      process_node((AstNode*)get_from_list(data->ls,a));
    }
    dealloc_list(missing);
    register_type(data->name);
    register_class(data);
    pop_scope();
  }
  List* all_fields=get_all_class_fields(data);
  Map* fields=collapse_ancestor_class_fields(all_fields);
  ERROR(validate && !fields,"class %s has colliding names",data->name);
  write("function %s(",data->name);
  FunctionNode* constructor=get_constructor(data);
  if(constructor){
    for(int a=0;a<constructor->args->n;a++){
      StringAstNode* e=(StringAstNode*)get_from_list(constructor->args,a);
      if(a) write(",");
      write("%s",e->text);
    }
  }
  write(")\n");
  write("local obj={}\n");
  FunctionNode* fdata=get_constructor(data);
  if(fdata){
    for(int a=0;a<fdata->body->n;a++){
      AstNode* e=(AstNode*)get_from_list(fdata->body,a);
      process_node(e);
      if(e->type==AST_FUNCTION) write("\n");
      if(e->type==AST_CALL) write("\n");
      if(e->type==AST_REQUIRE) write("\n");
    }
  }
  if(fields){
    for(int a=0;a<fields->n;a++){
      AstNode* child=(AstNode*)iterate_from_map(fields,a);
      if(child->type==AST_FUNCTION){
        fdata=(FunctionNode*)(child->data);
        if(fdata->is_constructor) continue;
        write("obj.%s=function(",(char*)(fdata->name->data));
        if(fdata->args){
          for(int a=0;a<fdata->args->n;a++){
            if(a) write(",");
            char* arg=((StringAstNode*)get_from_list(fdata->args,a))->text;
            write("%s",arg);
          }
        }
        write(")\n");
        for(int a=0;a<fdata->body->n;a++){
          AstNode* e=(AstNode*)get_from_list(fdata->body,a);
          process_node(e);
          if(e->type==AST_FUNCTION) write("\n");
          if(e->type==AST_CALL) write("\n");
          if(e->type==AST_REQUIRE) write("\n");
        }
        write("end\n");
      }else if(child->type==AST_DEFINE){
        BinaryNode* cdata=(BinaryNode*)(child->data);
        if(!cdata->r) write("obj.%s=nil",cdata->text);
        else{
          write("obj.%s=",cdata->text);
          process_node(cdata->r);
        }
        write("\n");
      }else{
        add_error(-1,"invalid child node in class %s",data->name);
        break;
      }
    }
    dealloc_map(fields);
  }
  write("return obj\n");
  write("end\n");
}

// Statement group
void process_stmt(AstNode* node){
  List* ls=(List*)(node->data);
  for(int a=0;a<ls->n;a++){
    AstNode* e=(AstNode*)get_from_list(ls,a);
    process_node(e);
    if(e->type==AST_CALL) write("\n");
    if(e->type==AST_FUNCTION) write("\n");
    if(e->type==AST_REQUIRE) write("\n");
  }
}
void process_do(AstNode* node){
  List* ls=(List*)(node->data);
  write("do\n");
  push_scope();
  for(int a=0;a<ls->n;a++){
    AstNode* e=(AstNode*)get_from_list(ls,a);
    process_node(e);
    if(e->type==AST_CALL) write("\n");
    if(e->type==AST_FUNCTION) write("\n");
    if(e->type==AST_REQUIRE) write("\n");
  }
  write("end\n");
  pop_scope();
}

// Statement
void process_call(AstNode* node){
  AstAstNode* data=(AstAstNode*)(node->data);
  if(validate){
    char* name=NULL;
    AstNode* functype=NULL;
    AstNode* dummy_constructor_type=NULL;
    if(data->l->type==AST_ID){
      name=(char*)(data->l->data);
      FunctionNode* func=function_exists(name);
      if(func){
        AstNode* funcnode=new_node(AST_FUNCTION,func);
        functype=get_type(funcnode);
        free(funcnode);
      }else{
        ClassNode* clas=class_exists(name);
        if(clas){
          FunctionNode* constructor=get_constructor(clas);
          if(constructor){
            AstNode* funcnode=new_node(AST_FUNCTION,constructor);
            functype=get_type(funcnode);
            free(funcnode);
          }else{
            dummy_constructor_type=new_node(AST_TYPE_FUNC,new_ast_list_node(new_node(AST_TYPE_BASIC,name),new_default_list()));
            functype=dummy_constructor_type;
          }
        }
      }
    }else if(data->l->type==AST_FIELD){
      name=((StringAstNode*)(data->l->data))->text;
      functype=get_type(data->l);
    }
    if(functype){
      List* funcargs=functype->data?((AstListNode*)(functype->data))->list:NULL;
      if(data->r){
        List* args=((AstListNode*)(data->r->data))->list;
        ERROR(!funcargs,"too many arguments for function %s",name);
        ERROR(funcargs->n!=args->n,"invalid number of arguments for function %s",name);
        for(int a=0;a<args->n;a++){
          AstNode* type1=get_type((AstNode*)get_from_list(args,a));
          AstNode* type2=(AstNode*)get_from_list(funcargs,a);
          ERROR(!typed_match(type1,type2),"invalid argument provided for function %s",name);
        }
      }else{
        ERROR(funcargs && funcargs->n,"not enough arguments for function %s",name);
      }
      if(dummy_constructor_type){
        dealloc_ast_type(dummy_constructor_type);
      }
    }
  }
  process_node(data->l);
  write("(");
  if(data->r) process_node(data->r);
  write(")");
}
void process_set(AstNode* node){
  AstAstNode* data=(AstAstNode*)(node->data);
  if(validate){
    AstNode* tl=get_type(data->l);
    AstNode* tr=get_type(data->r);
    if(tr->type==AST_TYPE_TUPLE){
      List* ls=(List*)(tr->data);
      if(ls->n==1) tr=(AstNode*)get_from_list(ls,0);
    }
    ERROR(!typed_match(tl,tr),"expression of type %t cannot be assigned to variable of type %t",tr,tl);
  }
  process_node(data->l);
  write("=");
  process_node(data->r);
  write("\n");
}
void process_return(AstNode* node){
  if(validate){
    FunctionNode* func=get_function_scope();
    if(func){
      AstNode* type1=func->type;
      AstNode* type2=node->data?get_type(node->data):any_type_const();
      if(type2->type==AST_TYPE_TUPLE){
        List* ls=(List*)(type2->data);
        if(ls->n==1){
          type2=(AstNode*)get_from_list(ls,0);
        }
      }
      ERROR(!typed_match(type1,type2),"function of type %t cannot return type %t",type1,type2);
    }
  }
  write("return");
  if(node->data){
    write(" ");
    process_node((AstNode*)(node->data));
  }
  write("\n");
}
void process_ltuple(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  for(int a=0;a<data->list->n;a++){
    if(a) write(",");
    process_id((AstNode*)get_from_list(data->list,a));
  }
}
void process_field(AstNode* node){
  StringAstNode* data=(StringAstNode*)(node->data);
  process_node(data->node);
  write(".%s",data->text);
}
void process_sub(AstNode* node){
  AstAstNode* data=(AstAstNode*)(node->data);
  process_node(data->l);
  write("[");
  process_node(data->r);
  write("]");
}
void process_id(AstNode* node){
  write("%s",(char*)(node->data));
}
void process_local(AstNode* node){
  StringAstNode* data=(StringAstNode*)(node->data);
  write("local %s",data->text);
  if(data->node){
    write("=");
    process_node(data->node);
  }
  write("\n");
}
void process_require(AstNode* node){
  AstNode* data=(AstNode*)(node->data);
  write("require ");
  process_node(data);
}

// Control
void process_function(AstNode* node){
  FunctionNode* data=(FunctionNode*)(node->data);
  write("function");
  if(data->name){
    if(data->type) register_function(data);
    write(" ");
    process_node(data->name);
  }
  write("(");
  for(int a=0;a<data->args->n;a++){
    if(a) write(",");
    StringAstNode* arg=(StringAstNode*)get_from_list(data->args,a);
    write("%s",arg->text);
  }
  write(")\n");
  if(data->body){
    push_scope();
    push_function(data);
    int num_returns=0;
    for(int a=0;a<data->body->n;a++){
      AstNode* child=(AstNode*)get_from_list(data->body,a);
      if(child->type==AST_RETURN) num_returns++;
      process_node(child);
      if(child->type==AST_CALL) write("\n");
      if(child->type==AST_FUNCTION) write("\n");
      if(child->type==AST_REQUIRE) write("\n");
    }
    if(data->is_constructor){
      ERROR(num_returns,"constructors cannot have return statements",NULL);
    }else if(!is_primitive(data->type,PRIMITIVE_NIL) && data->type->type!=AST_TYPE_ANY){
      ERROR(!num_returns,"function of type %t cannot return nil",data->type);
    }
    pop_function(data);
    write("end");
    pop_scope();
  }
}
void process_repeat(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  write("repeat\n");
  push_scope();
  for(int a=0;a<data->list->n;a++){
    AstNode* e=(AstNode*)get_from_list(data->list,a);
    process_node(e);
    if(e->type==AST_CALL) write("\n");
    if(e->type==AST_FUNCTION) write("\n");
    if(e->type==AST_REQUIRE) write("\n");
  }
  pop_scope();
  write("until ");
  process_node(data->node);
  write("\n");
}
void process_while(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  write("while ");
  process_node(data->node);
  write(" do\n");
  push_scope();
  for(int a=0;a<data->list->n;a++){
    AstNode* e=(AstNode*)get_from_list(data->list,a);
    process_node(e);
    if(e->type==AST_CALL) write("\n");
    if(e->type==AST_FUNCTION) write("\n");
    if(e->type==AST_REQUIRE) write("\n");
  }
  pop_scope();
  write("end\n");
}
void process_if(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  write("if ");
  process_node(data->node);
  write(" then\n");
  push_scope();
  for(int a=0;a<data->list->n;a++){
    AstNode* e=(AstNode*)get_from_list(data->list,a);
    process_node(e);
    if(e->type==AST_CALL) write("\n");
    if(e->type==AST_FUNCTION) write("\n");
    if(e->type==AST_REQUIRE) write("\n");
  }
  pop_scope();
  write("end\n");
}
void process_fornum(AstNode* node){
  FornumNode* data=(FornumNode*)(node->data);
  write("for %s=",data->name);
  process_node(data->num1);
  write(",");
  process_node(data->num2);
  if(data->num3){
    write(",");
    process_node(data->num3);
  }
  push_scope();
  write(" do\n");
  for(int a=0;a<data->body->n;a++){
    AstNode* e=(AstNode*)get_from_list(data->body,a);
    process_node(e);
    if(e->type==AST_CALL) write("\n");
    if(e->type==AST_FUNCTION) write("\n");
    if(e->type==AST_REQUIRE) write("\n");
  }
  write("end\n");
  pop_scope();
}
void process_forin(AstNode* node){
  ForinNode* data=(ForinNode*)(node->data);
  write("for ");
  process_node(data->lhs);
  write(" in ");
  process_node(data->tuple);
  write(" do\n");
  push_scope();
  for(int a=0;a<data->body->n;a++){
    AstNode* e=(AstNode*)get_from_list(data->body,a);
    process_node(e);
    if(e->type==AST_CALL) write("\n");
    if(e->type==AST_FUNCTION) write("\n");
    if(e->type==AST_REQUIRE) write("\n");
  }
  write("end\n");
  pop_scope();
}
void process_break(AstNode* node){
  write("break\n");
}
void process_label(AstNode* node){
  write("::%s::\n",(char*)(node->data));
}
void process_goto(AstNode* node){
  write("goto %s\n",(char*)(node->data));
}

// Primitives
void process_primitive(AstNode* node){
  write("%s",((StringAstNode*)(node->data))->text);
}
void process_table(AstNode* node){
  TableNode* data=(TableNode*)(node->data);
  write("{");
  for(int a=0;a<data->keys->n;a++){
    if(a) write(",");
    write("%s=",(char*)get_from_list(data->keys,a));
    process_node((AstNode*)get_from_list(data->vals,a));
  }
  write("}");
}
void process_tuple(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  List* ls=data->list;
  for(int a=0;a<ls->n;a++){
    if(a) write(",");
    process_node((AstNode*)get_from_list(ls,a));
  }
}

// Expressions
void process_unary(AstNode* node){
  BinaryNode* data=(BinaryNode*)(node->data);
  if(strcmp(data->text,"trust")) write("%s ",data->text);
  process_node(data->l);
}
void process_binary(AstNode* node){
  BinaryNode* data=(BinaryNode*)(node->data);
  process_node(data->l);
  write(" %s ",data->text);
  process_node(data->r);
}
void process_paren(AstNode* node){
  write("(");
  process_node((AstNode*)(node->data));
  write(")");
}
