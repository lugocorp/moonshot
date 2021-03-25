#include "./internal.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define ERROR(cond,msg,...) if(cond){add_error(-1,msg,__VA_ARGS__);return;}
static char* instance_str; // The variable used for the produced object in constructors
static FILE* _output; // The configured output as desired by the developer
static int validate; // The traversal phase you're running
static int line_written; // Zero if there's no content on the current output line yet
static int num_indents; // Number of tabs on the output line
static AstNode* float_type; // AstNode constant representing the FLOAT type
static AstNode* bool_type; // AstNode constant representing the BOOL type
static AstNode* int_type; // AstNode constant representing the INT type
static AstNode* any_type; // AstNode constant representing the ANY type

// Constant primitive type AstNodes access
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

/*
  Set up resources used by traversal
*/
void init_traverse(){
  instance_str=(char*)malloc(sizeof(char)*6);
  float_type=new_node(AST_TYPE_BASIC,PRIMITIVE_FLOAT);
  bool_type=new_node(AST_TYPE_BASIC,PRIMITIVE_BOOL);
  int_type=new_node(AST_TYPE_BASIC,PRIMITIVE_INT);
  any_type=new_node(AST_TYPE_ANY,NULL);
  sprintf(instance_str,"__obj");
  line_written=0;
  num_indents=0;
  preempt_scopes();
  init_types();
  init_scopes();
  push_scope();
}

/*
  Traverse through a parsed Moonshot AST
  This function is called twice in the Moonshot compile function
  if valid is 1, traversal will check for errors instead of writing output
  if valid is 0, traversal will output and ignore error checks
*/
void traverse(AstNode* root,int valid){
  validate=valid;
  process_stmt(root);
}

/*
  Deallocate resources used by the traversal module
*/
void dealloc_traverse(){
  free(instance_str);
  pop_scope();
  dealloc_scopes();
  dealloc_types();
  free(any_type);
  free(int_type);
  free(bool_type);
  free(float_type);
}

/*
  Sets the output to the stream desired by the developer
*/
void set_output(FILE* output){
  _output=output;
}

/*
  Increment the output indentation
*/
static void indent(int a){
  num_indents+=a;
}

/*
  Writes a message to the configured output
*/
static void write(const char* msg,...){
  if(validate) return;
  va_list args;
  int n=strlen(msg);
  va_start(args,msg);
  for(int a=0;a<n;a++){
    if(msg[a]=='\n'){
      line_written=0;
    }else if(!line_written){
      line_written=1;
      for(int a=0;a<num_indents;a++){
        fprintf(_output,"\t");
      }
    }
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

/*
  Prints a newline character after nodes that could either be a value or statement
*/
static void conditional_newline(AstNode* node){
  if(node->type==AST_FUNCTION) write("\n");
  if(node->type==AST_CALL) write("\n");
  if(node->type==AST_REQUIRE) write("\n");
}
static void process_list(List* ls){
  for(int a=0;a<ls->n;a++){
    AstNode* e=(AstNode*)get_from_list(ls,a);
    process_node(e);
    conditional_newline(e);
  }
}

/*
  Switch for calling a specific traversal method by AstNode type
*/
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
    case AST_ELSEIF: process_elseif(node); return;
    case AST_CLASS: process_class(node); return;
    case AST_SUPER: process_super(node); return;
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
    case AST_ELSE: process_else(node); return;
    case AST_SET: process_set(node); return;
    case AST_SUB: process_sub(node); return;
    case AST_IF: process_if(node); return;
    case AST_DO: process_do(node); return;
    case AST_ID: process_id(node); return;
    default: add_error(-1,"invalid Moonshot AST detected (node ID %i)\n",node->type);
  }
}

/*
  Traverses through entity nodes
*/
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
  push_class_scope(data);
  if(validate){
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
  }
  List* all_fields=get_all_class_fields(data);
  Map* fields=collapse_ancestor_class_fields(all_fields);
  dealloc_list(all_fields);
  ERROR(validate && !fields,"class %s has colliding names",data->name);
  write("function %s(",data->name);
  FunctionNode* fdata=get_constructor(data);
  if(fdata){
    for(int a=0;a<fdata->args->n;a++){
      StringAstNode* e=(StringAstNode*)get_from_list(fdata->args,a);
      if(a) write(",");
      write("%s",e->text);
    }
  }
  write(")\n");
  indent(1);
  write("local %s={}\n",instance_str);
  if(fields){
    for(int a=0;a<fields->n;a++){
      AstNode* child=(AstNode*)iterate_from_map(fields,a);
      if(child->type==AST_DEFINE){
        BinaryNode* cdata=(BinaryNode*)(child->data);
        add_scoped_var(new_string_ast_node(cdata->text,cdata->l));
        if(cdata->r){
          write("%s.%s=",instance_str,cdata->text);
          process_node(cdata->r);
        }else{
          write("%s.%s=nil",instance_str,cdata->text);
        }
        write("\n");
      }
    }
  }
  if(fdata){
    push_function_scope(fdata);
    process_list(fdata->body);
    pop_scope();
  }
  if(fields){
    for(int a=0;a<fields->n;a++){
      AstNode* child=(AstNode*)iterate_from_map(fields,a);
      if(child->type==AST_FUNCTION){
        fdata=(FunctionNode*)(child->data);
        if(fdata->is_constructor) continue;
        push_function_scope(fdata);
        char* funcname=(char*)(fdata->name->data);
        //add_scoped_var(new_string_ast_node(funcname,get_type(child)));
        write("%s.%s=function(",instance_str,funcname);
        if(fdata->args){
          for(int a=0;a<fdata->args->n;a++){
            if(a) write(",");
            char* arg=((StringAstNode*)get_from_list(fdata->args,a))->text;
            write("%s",arg);
          }
        }
        write(")\n");
        indent(1);
        process_list(fdata->body);
        indent(-1);
        write("end\n");
        pop_scope();
      }else if(child->type!=AST_DEFINE){
        add_error(-1,"invalid child node in class %s",data->name);
        break;
      }
    }
    dealloc_map(fields);
  }
  write("return %s\n",instance_str);
  indent(-1);
  write("end\n");
  pop_scope();
}

/*
  Traverses through node groups
*/
void process_stmt(AstNode* node){
  process_list((List*)(node->data));
}
void process_do(AstNode* node){
  write("do\n");
  indent(1);
  push_scope();
  process_list((List*)(node->data));
  indent(-1);
  write("end\n");
  pop_scope();
}

/*
  Returns 1 if the provided arguments can be accepted by the function definition
  target is whatever entity you want represented in any potential error messages
  func is the AST_FUNCTION node you're calling
  args_node is the AST_TUPLE node that represents your arguments
*/
static int validate_function_parameters(char* target,AstNode* func,AstNode* args_node){
  if(!func){
    if(args_node){
      List* args=((AstListNode*)(args_node->data))->list;
      return args->n==0;
    }
    return 1;
  }
  AstNode* functype=get_type(func);
  List* funcargs=functype->data?((AstListNode*)(functype->data))->list:NULL;
  if(args_node){
    List* args=((AstListNode*)(args_node->data))->list;
    if(!funcargs){
      add_error(-1,"too many arguments for %s",target);
      return 0;
    }
    int max=funcargs->n;
    if(is_variadic_function(funcargs)){
      if(args->n<funcargs->n-1){
        add_error(-1,"not enough arguments for %s",target);
        return 0;
      }
      max=funcargs->n-1;
    }else if(args->n!=funcargs->n){
      add_error(-1,"invalid number of arguments for %s",target);
      return 0;
    }
    for(int a=0;a<max;a++){
      AstNode* type1=get_type((AstNode*)get_from_list(args,a));
      AstNode* type2=(AstNode*)get_from_list(funcargs,a);
      if(!typed_match(type1,type2)){
        add_error(-1,"invalid argument provided for %s",target);
        return 0;
      }
    }
  }else if(funcargs && funcargs->n && !(funcargs->n==1 && is_variadic_function(funcargs))){
    add_error(-1,"not enough arguments for %s",target);
    return 0;
  }
  return 1;
}

/*
  Traverses through function call nodes
*/
void process_call(AstNode* node){
  AstAstNode* data=(AstAstNode*)(node->data);
  if(validate){
    char* name=NULL;
    AstNode* functype=NULL;
    AstNode* funcnode=NULL;
    FunctionNode* func=NULL;
    AstNode* dummy_constructor_type=NULL;
    if(data->l->type==AST_ID){
      name=(char*)(data->l->data);
      func=function_exists(name);
      if(func){
        funcnode=new_node(AST_FUNCTION,func);
        functype=get_type(funcnode);
      }else{
        ClassNode* clas=class_exists(name);
        if(clas){
          FunctionNode* constructor=get_constructor(clas);
          if(constructor){
            funcnode=new_node(AST_FUNCTION,constructor);
            functype=get_type(funcnode);
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
      char* target=(char*)malloc(sizeof(char)*(strlen(name)+10));
      sprintf(target,"function %s",name);
      validate_function_parameters(target,funcnode,data->r);
      if(dummy_constructor_type){
        dealloc_ast_type(dummy_constructor_type);
      }
      if(funcnode) free(funcnode);
      free(target);
    }
  }
  process_node(data->l);
  write("(");
  if(data->r) process_node(data->r);
  write(")");
}
void process_super(AstNode* node){
  AstNode* data=(AstNode*)(node->data);
  ClassNode* clas=get_class_scope();
  FunctionNode* func=get_method_scope();
  if(validate){
    ERROR(!clas,"cannot use super methods outside of a class",NULL);
    ERROR(!func,"must use super keyword within a class method",NULL);
    ERROR(!clas->parent,"cannot use super methods because %s is not a child class",clas->name);
  }
  ClassNode* parent=class_exists(clas->parent);
  FunctionNode* method=get_parent_method(parent,func);
  if(validate){
    // I'm assuming func->name is of type AST_ID
    ERROR(!method && func->is_constructor,"constructor in class %s does not override a super constructor",clas->name);
    ERROR(!method && !func->is_constructor,"method %s in class %s does not override a super method",(char*)(func->name->data),clas->name);
    char* target=(char*)malloc(sizeof(char)*(strlen(clas->name)+22));
    sprintf(target,"constructor of class %s",clas->name);
    AstNode* fnode=new_node(AST_FUNCTION,method);
    validate_function_parameters(target,fnode,data);
    free(target);
    free(fnode);
  }
  push_class_scope(parent);
  push_function_scope(method);
  write("(function(");
  for(int a=0;a<method->args->n;a++){
    StringAstNode* e=(StringAstNode*)get_from_list(method->args,a);
    if(a) write(",");
    write("%s",e->text);
  }
  write(")\n");
  indent(1);
  for(int a=0;a<method->body->n;a++){
    AstNode* child=(AstNode*)get_from_list(method->body,a);
    process_node(child);
    conditional_newline(child);
  }
  indent(-1);
  write("end)(");
  if(data){
    List* args=((AstListNode*)(data->data))->list;
    for(int a=0;a<args->n;a++){
      if(a) write(",");
      process_node((AstNode*)get_from_list(args,a));
    }
  }
  write(")\n");
  pop_scope();
  pop_scope();
}

/*
  Traverses through miscellaneous value nodes
*/
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

/*
  Traverses through identifier nodes
*/
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
  char* var=(char*)(node->data);
  if(get_class_scope() && !strcmp(var,"this")){
    write("%s",instance_str);
  }else{
    if(field_defined_in_class(var)){
      write("%s.",instance_str);
    }
    write("%s",var);
  }
}

/*
  Traverses through nodes that define new things
*/
void process_require(AstNode* node){
  AstNode* data=(AstNode*)(node->data); // data is AST_PRIMITIVE with type string
  StringAstNode* primitive=(StringAstNode*)(data->data);
  char* filename=primitive->text;
  if(validate){
    require_file(filename);
  }
  write("require ");
  int l=strlen(filename);
  if(l>=6 && !strncmp(filename+l-6,".moon",5)){
    char* filename1=(char*)malloc(sizeof(char)*l);
    strncpy(filename1,filename,l-5);
    filename1[l-5]=0;
    strcat(filename1,"lua");
    filename1[l-2]=filename1[0];
    filename1[l-1]=0;
    write("%s",filename1);
    free(filename1);
  }else{
    write("%s",filename);
  }
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
void process_define(AstNode* node){
  BinaryNode* data=(BinaryNode*)(node->data);
  if(validate){
    //ERROR(!compound_type_exists(data->l),"reference to nonexistent type %t",data->l);
    make_type_promise(data->l);
    if(data->r){
      AstNode* tr=get_type(data->r);
      ERROR(!typed_match(data->l,tr),"expression of type %t cannot be assigned to variable of type %t",tr,data->l);
    }
    StringAstNode* data1=new_string_ast_node(data->text,data->l);
    if(!add_scoped_var(data1)){
      add_error(-1,"variable %s was already declared in this scope",data->text);
      free(data1);
    }
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
    //ERROR(!compound_type_exists(data->node),"type %t does not exist",data->node);
    make_type_promise(data->node);
    ERROR(!add_type_equivalence(data->text,data->node,RL_EQUALS),"co-dependent typedef %s detected",data->text);
    register_type(data->text);
  }
}

/*
  Traverses through a function node
*/
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
    StringAstNode* e=(StringAstNode*)get_from_list(data->args,a);
    //ERROR(!compound_type_exists(e->node),"reference to nonexistent type %t",e->node);
    make_type_promise(e->node);
    write("%s",e->text);
  }
  write(")\n");
  indent(1);
  if(data->body){
    push_function_scope(data);
    int num_returns=0;
    for(int a=0;a<data->body->n;a++){
      AstNode* child=(AstNode*)get_from_list(data->body,a);
      if(child->type==AST_RETURN) num_returns++;
      process_node(child);
      conditional_newline(child);
    }
    if(data->is_constructor){
      ERROR(num_returns,"constructors cannot have return statements",NULL);
    }else if(!is_primitive(data->type,PRIMITIVE_NIL) && data->type->type!=AST_TYPE_ANY){
      ERROR(!num_returns,"function of type %t cannot return nil",data->type);
    }
    indent(-1);
    write("end");
    pop_scope();
  }
}

/*
  Traverses through conditional nodes
*/
void process_repeat(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  write("repeat\n");
  indent(1);
  push_scope();
  process_list(data->list);
  pop_scope();
  indent(-1);
  write("until ");
  process_node(data->node);
  write("\n");
}
void process_while(AstNode* node){
  AstListNode* data=(AstListNode*)(node->data);
  write("while ");
  process_node(data->node);
  write(" do\n");
  indent(1);
  push_scope();
  process_list(data->list);
  pop_scope();
  indent(-1);
  write("end\n");
}
void process_if(AstNode* node){
  IfNode* data=(IfNode*)(node->data);
  write("if ");
  process_node(data->expr);
  write(" then\n");
  indent(1);
  push_scope();
  process_list(data->body);
  pop_scope();
  indent(-1);
  if(data->next) process_node(data->next);
  else write("end\n");
}
void process_elseif(AstNode* node){
  IfNode* data=(IfNode*)(node->data);
  write("elseif ");
  process_node(data->expr);
  write(" then\n");
  indent(1);
  push_scope();
  process_list(data->body);
  pop_scope();
  indent(-1);
  if(data->next) process_node(data->next);
  else write("end\n");
}
void process_else(AstNode* node){
  write("else\n");
  indent(1);
  push_scope();
  process_list((List*)(node->data));
  pop_scope();
  indent(-1);
  write("end\n");
}

/*
  Traverses through for loop nodes
*/
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
  indent(1);
  process_list(data->body);
  indent(-1);
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
  indent(1);
  push_scope();
  process_list(data->body);
  indent(-1);
  write("end\n");
  pop_scope();
}

/*
  Traverses through simple control statement nodes
*/
void process_break(AstNode* node){
  write("break\n");
}
void process_label(AstNode* node){
  write("::%s::\n",(char*)(node->data));
}
void process_goto(AstNode* node){
  write("goto %s\n",(char*)(node->data));
}

/*
  Traverses through primitive value nodes
*/
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

/*
  Traverses through expression nodes
*/
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
