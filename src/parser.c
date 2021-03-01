#include "./internal.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
// #define DEBUG(...) printf(__VA_ARGS__)
static List* tokens;
static int _i;

// Errors
static AstNode* error(Token* tk,const char* msg){
  char* error_msg=(char*)malloc(sizeof(char)*256);
  if(tk) sprintf(error_msg,"ERROR %s on line %i",msg,tk->line);
  else sprintf(error_msg,"ERROR %s",msg);
  add_error(error_msg);
  return NULL;
}

// Parsing interface
AstNode* parse(List* ls){
  _i=0;
  tokens=ls;
  AstNode* root=parse_stmt();
  if(root){
    Token* tk;
    while(_i<tokens->n){
      if((tk=(Token*)get_from_list(tokens,_i++))->type!=TK_SPACE){
        error(tk,"unparsed tokens");
        // TODO deallocate root somehow
        return NULL;
      }
    }
  }
  return root;
}

// Token consumption functions
static Token* consume(){
  while(_i<tokens->n && ((Token*)get_from_list(tokens,_i))->type==TK_SPACE) _i++;
  return (_i<tokens->n)?((Token*)get_from_list(tokens,_i++)):NULL;
}
static Token* check(){
  int a=_i;
  while(a<tokens->n && ((Token*)get_from_list(tokens,a))->type==TK_SPACE) a++;
  return (a<tokens->n)?((Token*)get_from_list(tokens,a)):NULL;
}
static Token* consume_next(){
  if(_i<tokens->n) return (Token*)get_from_list(tokens,_i++);
  return NULL;
}
static Token* check_next(){
  if(_i<tokens->n) return (Token*)get_from_list(tokens,_i);
  return NULL;
}
static Token* check_ahead(int n){
  int a=_i;
  while(n){
    while(a<tokens->n && ((Token*)get_from_list(tokens,a))->type==TK_SPACE) a++;
    if(n>1 && a<tokens->n) a++;
    n--;
  }
  return (a<tokens->n)?((Token*)get_from_list(tokens,a)):NULL;
}
static int expect(Token* tk,int type){
  return tk && tk->type==type;
}
static int specific(Token* tk,int type,const char* val){
  return tk && tk->type==type && !strcmp(tk->text,val);
}
static int precedence(char* op){
  if(!strcmp(op,"^")) return 6;
  if(!strcmp(op,"*") || !strcmp(op,"/")) return 5;
  if(!strcmp(op,"+") || !strcmp(op,"-")) return 4;
  if(!strcmp(op,"..")) return 3;
  if(!strcmp(op,"<=") || !strcmp(op,">=") || !strcmp(op,"<") || !strcmp(op,">") || !strcmp(op,"==") || !strcmp(op,"~=")) return 2;
  if(!strcmp(op,"and")) return 1;
  return 0;
}

// Moonshot-specific parse functions
AstNode* parse_interface(){
  char* parent=NULL;
  Token* tk=consume();
  List* ls=new_default_list();
  if(!expect(tk,TK_INTERFACE)) return error(tk,"invalid interface");
  tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid interface");
  char* name=tk->text;
  tk=check();
  if(expect(tk,TK_EXTENDS)){
    consume();
    tk=consume();
    if(!expect(tk,TK_NAME)) return error(tk,"invalid interface");
    parent=tk->text;
  }
  tk=consume();
  if(!expect(tk,TK_WHERE)) return error(tk,"invalid interface");
  tk=check();
  while(tk && !expect(tk,TK_END)){
    AstNode* type=NULL;
    if(!expect(tk,TK_FUNCTION)){
      type=parse_type();
      if(!type) return NULL;
    }
    AstNode* func=parse_function(type,0);
    if(!func) return NULL;
    add_to_list(ls,func);
    tk=check();
  }
  tk=consume();
  if(!expect(tk,TK_END)) return error(tk,"invalid interface");
  return new_node(AST_INTERFACE,new_interface_node(name,parent,ls));
}
AstNode* parse_class(){
  char* parent=NULL;
  Token* tk=consume();
  List* ls=new_default_list();
  List* interfaces=new_default_list();
  if(!expect(tk,TK_CLASS)) return error(tk,"invalid class");
  tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid class");
  char* name=tk->text;
  tk=check();
  if(expect(tk,TK_EXTENDS)){
    consume();
    tk=consume();
    if(!expect(tk,TK_NAME)) return error(tk,"invalid class");
    parent=tk->text;
    tk=check();
  }
  if(expect(tk,TK_IMPLEMENTS)){
    consume();
    tk=consume();
    if(!expect(tk,TK_NAME)) return error(tk,"invalid class");
    add_to_list(interfaces,tk->text);
    tk=check();
    while(specific(tk,TK_MISC,",")){
      consume();
      tk=consume();
      if(!expect(tk,TK_NAME)) return error(tk,"invalid class");
      add_to_list(interfaces,tk->text);
      tk=check();
    }
  }
  tk=consume();
  if(!expect(tk,TK_WHERE)) return error(tk,"invalid class");
  tk=check();
  while(tk && !expect(tk,TK_END)){
    AstNode* node=NULL;
    if(expect(tk,TK_FUNCTION)){
      node=parse_function(NULL,1);
    }else{
      AstNode* type=parse_type();
      if(!type) return NULL;
      tk=check();
      if(expect(tk,TK_FUNCTION)){
        node=parse_function(type,1);
      }else{
        node=parse_define(type);
      }
    }
    if(!node) return NULL;
    add_to_list(ls,node);
    tk=check();
  }
  tk=consume();
  if(!expect(tk,TK_END)) return error(tk,"invalid class");
  return new_node(AST_CLASS,new_class_node(name,parent,interfaces,ls));
}
AstNode* parse_typedef(){
  Token* tk=consume();
  if(!expect(tk,TK_TYPEDEF)) return error(tk,"invalid typedef");
  tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid typedef");
  char* name=tk->text;
  AstNode* node=parse_type();
  if(!node) return NULL;
  return new_node(AST_TYPEDEF,new_string_ast_node(name,node));
}
AstNode* parse_define(AstNode* type){
  AstNode* expr=NULL;
  Token* tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid definition");
  char* name=tk->text;
  tk=check();
  if(specific(tk,TK_MISC,"=")){
    consume();
    expr=parse_expr();
    if(!expr) return NULL;
  }
  return new_node(AST_DEFINE,new_binary_node(name,type,expr));
}
AstNode* parse_basic_type(){
  Token* tk=check();
  AstNode* node=NULL;
  if(expect(tk,TK_VAR)){
    consume();
    node=new_node(AST_TYPE_ANY,NULL);
  }else if(expect(tk,TK_NAME)){
    consume();
    node=new_node(AST_TYPE_BASIC,tk->text);
  }else if(specific(tk,TK_PAREN,"(")){
    node=parse_type();
    if(!node) return NULL;
  }else{
    return error(tk,"invalid type");
  }
  tk=check();
  while(specific(tk,TK_PAREN,"(")){
    consume();
    tk=check();
    List* ls=new_default_list();
    while(tk && !specific(tk,TK_PAREN,")")){
      AstNode* arg=parse_type();
      if(!arg) return error(tk,"invalid function type");
      add_to_list(ls,arg);
      tk=check();
      if(specific(tk,TK_MISC,",")){
        consume();
      }
    }
    node=new_node(AST_TYPE_FUNC,new_ast_list_node(node,ls));
    tk=consume();
    if(!specific(tk,TK_PAREN,")")) return error(tk,"invalid function type");
    tk=check();
  }
  return node;
}
AstNode* parse_type(){
  Token* tk=check();
  if(specific(tk,TK_PAREN,"(")){
    List* ls=new_default_list();
    consume();
    AstNode* e=parse_type();
    if(!e) return NULL;
    add_to_list(ls,e);
    tk=check();
    while(specific(tk,TK_MISC,",")){
      consume();
      e=parse_type();
      if(!e) return NULL;
      add_to_list(ls,e);
      tk=check();
    }
    tk=consume();
    if(!specific(tk,TK_PAREN,")")) return error(tk,"invalid tuple type");
    return new_node(AST_TYPE_TUPLE,ls);
  }
  return parse_basic_type();
}

// Statement group parse functions
AstNode* parse_stmt(){
  Token* tk;
  AstNode* node;
  List* ls=new_default_list();
  while(1){
    tk=check();
    if(!tk) break;
    if(expect(tk,TK_FUNCTION)) node=parse_function(NULL,1);
    else if(expect(tk,TK_IF)) node=parse_if();
    else if(expect(tk,TK_CLASS)) node=parse_class();
    else if(expect(tk,TK_INTERFACE)) node=parse_interface();
    else if(expect(tk,TK_TYPEDEF)) node=parse_typedef();
    else if(expect(tk,TK_RETURN)) node=parse_return();
    else if(expect(tk,TK_DBCOLON)) node=parse_label();
    else if(expect(tk,TK_LOCAL)) node=parse_local();
    else if(expect(tk,TK_BREAK)) node=parse_break();
    else if(expect(tk,TK_REPEAT)) node=parse_repeat();
    else if(expect(tk,TK_WHILE)) node=parse_while();
    else if(expect(tk,TK_GOTO)) node=parse_goto();
    else if(expect(tk,TK_DO)) node=parse_do();
    else if(specific(tk,TK_PAREN,"(")){
      AstNode* type=parse_type();
      if(type){
        tk=check();
        if(expect(tk,TK_NAME)){
          node=parse_define(type);
        }else if(expect(tk,TK_FUNCTION)){
          node=parse_function(type,1);
        }else{
          node=error(tk,"invalid statement");
        }
      }else node=error(tk,"invalid statement");
    }else if(expect(tk,TK_NAME) || expect(tk,TK_VAR)){
      int tmp=_i;
      AstNode* lhs=parse_lhs();
      tk=check();
      _i=tmp;
      if(specific(tk,TK_PAREN,"(")){
        node=parse_call();
      }else if(specific(tk,TK_MISC,"=")){
        node=parse_set();
      }else if(expect(tk,TK_NAME) || expect(tk,TK_FUNCTION) || specific(tk,TK_PAREN,"(")){
        AstNode* type=parse_type();
        tk=check();
        if(expect(tk,TK_NAME)){
          node=parse_define(type);
        }else if(expect(tk,TK_FUNCTION)){
          node=parse_function(type,1);
        }else{
          node=error(tk,"invalid statement");
        }
      }else{
        node=error(tk,"invalid statement");
      }
    }else if(expect(tk,TK_FOR)){
      tk=check_ahead(3);
      if(specific(tk,TK_MISC,",") || expect(tk,TK_IN)) node=parse_forin();
      else if(specific(tk,TK_MISC,"=")) node=parse_fornum();
      else return error(tk,"invalid loop");
    }else break;
    if(node) add_to_list(ls,node);
    else return NULL;
  }
  return new_node(AST_STMT,ls);
}
AstNode* parse_call(){
  AstNode* args=NULL;
  AstNode* lhs=parse_lhs();
  if(!lhs) return NULL;
  Token* tk=consume_next();
  if(!specific(tk,TK_PAREN,"(")) return error(tk,"invalid function call");
  tk=check();
  if(tk && !specific(tk,TK_PAREN,")")){
    args=parse_tuple();
    if(!args) return NULL;
  }
  tk=consume();
  if(!specific(tk,TK_PAREN,")")) return error(tk,"invalid function call");
  // DEBUG("call node\n");
  return new_node(AST_CALL,new_ast_ast_node(lhs,args));
}
AstNode* parse_set(){
  AstNode* expr=NULL;
  AstNode* lhs=parse_lhs();
  if(!lhs) return NULL;
  Token* tk=consume();
  if(!specific(tk,TK_MISC,"=")) return error(tk,"invalid statement");
  expr=parse_expr();
  if(!expr) return NULL;
  // DEBUG("set node\n");
  return new_node(AST_SET,new_ast_ast_node(lhs,expr));
}
AstNode* parse_do(){
  Token* tk=consume();
  if(!expect(tk,TK_DO)) return error(tk,"invalid do block");
  AstNode* node=parse_stmt();
  if(!node) return NULL;
  tk=consume();
  if(!expect(tk,TK_END)) return error(tk,"invalid do block");
  // DEBUG("do stmt\n");
  return new_node(AST_DO,(List*)(node->data));
}

// Statement parse functions
AstNode* parse_return(){
  Token* tk=consume();
  if(!expect(tk,TK_RETURN)) return error(tk,"invalid return statement");
  AstNode* node=parse_expr();
  if(!node) return NULL;
  return new_node(AST_RETURN,node);
}
AstNode* parse_lhs(){
  Token* tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid left-hand side of statement");
  // DEBUG("lhs: %s\n",tk->text);
  char* first=tk->text;
  tk=check();
  if(specific(tk,TK_MISC,",")){
    List* ls=new_default_list();
    add_to_list(ls,first);
    while(specific(tk,TK_MISC,",")){
      consume();
      tk=consume();
      if(!expect(tk,TK_NAME)) return error(tk,"invalid left-hand tuple");
      // DEBUG(",%s\n",tk->text);
      add_to_list(ls,tk->text);
      tk=check();
    }
    return new_node(AST_LTUPLE,ls);
  }
  tk=check_next();
  AstNode* tmp=NULL;
  AstNode* node=new_node(AST_ID,first);
  int attempt=(tk)?1:0;
  while(attempt){
    if(specific(tk,TK_SQUARE,"[")){
      consume();
      // DEBUG("square bracket\n");
      AstNode* r=parse_expr();
      if(!r) return NULL;
      tmp=node;
      node=new_node(AST_SUB,new_ast_ast_node(tmp,r));
      tk=consume();
      if(!specific(tk,TK_SQUARE,"]")) return error(tk,"invalid property");
    }
    if(specific(tk,TK_MISC,".")){
      consume();
      tk=consume();
      // DEBUG(".%s\n",tk->text);
      if(!expect(tk,TK_NAME)) return error(tk,"invalid field");
      tmp=node;
      node=new_node(AST_FIELD,new_string_ast_node(tk->text,tmp));
    }
    tk=check_next();
    attempt=specific(tk,TK_MISC,".") || specific(tk,TK_SQUARE,"[");
  }
  return node;
}
AstNode* parse_local(){
  char name[256];
  AstNode* node=NULL;
  Token* tk=consume();
  if(!expect(tk,TK_LOCAL)) return error(tk,"invalid local variable declaration");
  tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid lcoal variable declaration");
  strcpy(name,tk->text);
  tk=check();
  if(specific(tk,TK_MISC,"=")){
    consume();
    node=parse_expr();
    if(!node) return NULL;
  }
  // DEBUG("local\n");
  return new_node(AST_LOCAL,new_string_ast_node(name,node));
}

// Control parse functions
AstNode* parse_function(AstNode* type,int include_body){
  char name[256];
  Token* tk=consume();
  List* args=new_default_list();
  if(!type) type=new_node(AST_TYPE_ANY,NULL);
  if(!expect(tk,TK_FUNCTION)) return error(tk,"invalid function");
  tk=check();
  if(expect(tk,TK_NAME)){
    tk=consume();
    strcpy(name,tk->text);
  }else{
    name[0]=0;
  }
  tk=consume();
  if(!specific(tk,TK_PAREN,"(")) return error(tk,"invalid function");
  tk=consume();
  while(tk && !specific(tk,TK_PAREN,")")){
    if(!expect(tk,TK_NAME)) return error(tk,"invalid function argument");
    add_to_list(args,tk->text);
    tk=consume();
    if(specific(tk,TK_MISC,",")){
      tk=consume();
    }
  }
  if(!tk) return error(tk,"invalid function");
  List* ls=NULL;
  if(include_body){
    AstNode* node=parse_stmt();
    if(!node) return NULL;
    tk=consume();
    if(!expect(tk,TK_END)) return error(tk,"invalid function");
    ls=(List*)(node->data);
  }
  // DEBUG("Function %s (%i args) (%i stmts)\n",name,args->n,ls?ls->n:0);
  return new_node(AST_FUNCTION,new_function_node(name,type,args,ls));
}
AstNode* parse_repeat(){
  Token* tk=consume();
  if(!expect(tk,TK_REPEAT)) return error(tk,"invalid repeat statement");
  AstNode* body=parse_stmt();
  if(!body) return NULL;
  tk=consume();
  if(!expect(tk,TK_UNTIL)) return error(tk,"invalid repeat statement");
  AstNode* expr=parse_expr();
  if(!expr) return NULL;
  // DEBUG("repeat\n");
  return new_node(AST_REPEAT,new_ast_list_node(expr,(List*)(body->data)));
}
AstNode* parse_while(){
  Token* tk=consume();
  if(!expect(tk,TK_WHILE)) return error(tk,"invalid while statement");
  AstNode* expr=parse_expr();
  if(!expr) return NULL;
  tk=consume();
  if(!expect(tk,TK_DO)) return error(tk,"invalid while statement");
  AstNode* body=parse_stmt();
  if(!body) return NULL;
  tk=consume();
  if(!expect(tk,TK_END)) return error(tk,"invalid while statement");
  // DEBUG("while\n");
  return new_node(AST_WHILE,new_ast_list_node(expr,(List*)(body->data)));
}
AstNode* parse_if(){
  Token* tk=consume();
  if(!expect(tk,TK_IF)) return error(tk,"invalid if statement");
  AstNode* expr=parse_expr();
  if(!expr) return NULL;
  tk=consume();
  if(!expect(tk,TK_THEN)) return error(tk,"invalid if statement");
  AstNode* body=parse_stmt();
  if(!body) return NULL;
  tk=consume();
  if(!expect(tk,TK_END)) return error(tk,"invalid if statement");
  // DEBUG("if\n");
  return new_node(AST_IF,new_ast_list_node(expr,(List*)(body->data)));
}
AstNode* parse_fornum(){
  char name[256];
  Token* tk=consume();
  AstNode *num1,*num2,*num3=NULL;
  if(!expect(tk,TK_FOR)) return error(tk,"invalid for loop");
  tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid for loop");
  strcpy(name,tk->text);
  tk=consume();
  if(!specific(tk,TK_MISC,"=")) return error(tk,"invalid for loop");
  num1=parse_number();
  if(!num1) return NULL;
  tk=consume();
  if(!specific(tk,TK_MISC,",")) return error(tk,"invalid for loop");
  num2=parse_number();
  if(!num2) return NULL;
  tk=check();
  if(specific(tk,TK_MISC,",")){
    consume();
    num3=parse_number();
    if(!num3) return NULL;
  }
  tk=consume();
  if(!expect(tk,TK_DO)) return error(tk,"invalid for loop");
  AstNode* body=parse_stmt();
  if(!body) return NULL;
  tk=consume();
  if(!expect(tk,TK_END)) return error(tk,"invalid for loop");
  // DEBUG("for num\n");
  return new_node(AST_FORNUM,new_fornum_node(name,num1,num2,num3,(List*)(body->data)));
}
AstNode* parse_forin(){
  Token* tk=consume();
  List* lhs=new_default_list();
  if(!expect(tk,TK_FOR)) return error(tk,"invalid for loop");
  tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid for loop");
  add_to_list(lhs,tk->text);
  tk=check();
  while(specific(tk,TK_MISC,",")){
    consume();
    tk=consume();
    if(!expect(tk,TK_NAME)) return error(tk,"invalid for loop");
    add_to_list(lhs,tk->text);
    tk=check();
  }
  tk=consume();
  if(!expect(tk,TK_IN)) return error(tk,"invalid for loop");
  AstNode* tuple=parse_tuple();
  if(!tuple) return NULL;
  tk=consume();
  if(!expect(tk,TK_DO)) return error(tk,"invalid for loop");
  AstNode* body=parse_stmt();
  if(!body) return NULL;
  tk=consume();
  if(!expect(tk,TK_END)) return error(tk,"invalid for loop");
  AstNode* lhs_node=new_node(AST_LTUPLE,lhs);
  // DEBUG("for in\n");
  return new_node(AST_FORIN,new_forin_node(lhs_node,tuple,(List*)(body->data)));
}
AstNode* parse_break(){
  Token* tk=consume();
  if(!expect(tk,TK_BREAK)) return error(tk,"invalid break");
  return new_node(AST_BREAK,NULL);
}
AstNode* parse_label(){
  char text[256];
  Token* tk=consume();
  if(!expect(tk,TK_DBCOLON)) return error(tk,"invalid label");
  tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid label");
  strcpy(text,tk->text);
  tk=consume();
  if(!expect(tk,TK_DBCOLON)) return error(tk,"invalid label");
  // DEBUG("label %s\n",text);
  return new_node(AST_LABEL,new_string_node(text));
}
AstNode* parse_goto(){
  char text[256];
  Token* tk=consume();
  if(!expect(tk,TK_GOTO)) return error(tk,"invalid goto statement");
  tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid goto statement");
  strcpy(text,tk->text);
  // DEBUG("goto %s\n",text);
  return new_node(AST_GOTO,new_string_node(text));
}

// Primitive types parse functions
AstNode* parse_string(){
  char text[256];
  Token* tk=consume();
  if(!expect(tk,TK_QUOTE)) return error(tk,"invalid string");
  Token* begin=tk;
  int line=tk->line;
  strcpy(text,begin->text);
  tk=consume_next();
  while(tk && !specific(tk,TK_QUOTE,begin->text)){
    strcat(text,tk->text);
    tk=consume_next();
  }
  if(tk) strcat(text,begin->text);
  else return error(begin,"unclosed string");
  // DEBUG("string %s\n",text);
  return new_node(AST_PRIMITIVE,new_primitive_node(text,PRIMITIVE_STRING));
}
AstNode* parse_number(){
  char text[256];
  Token* tk=consume();
  if(!expect(tk,TK_INT)) return error(tk,"invalid number");
  strcpy(text,tk->text);
  tk=check_next();
  if(specific(tk,TK_MISC,".")){
    consume();
    tk=consume();
    if(!expect(tk,TK_INT)) return error(tk,"invalid number");
    strcat(text,".");
    strcat(text,tk->text);
    return new_node(AST_PRIMITIVE,new_primitive_node(text,PRIMITIVE_FLOAT));
  }
  // DEBUG("%s %s\n",type,text);
  return new_node(AST_PRIMITIVE,new_primitive_node(text,PRIMITIVE_INT));
}
AstNode* parse_boolean(){
  char text[6];
  Token* tk=consume();
  if(!expect(tk,TK_TRUE) && !expect(tk,TK_FALSE)) return error(tk,"invalid boolean");
  strcpy(text,tk->text);
  // DEBUG("boolean %s\n",text);
  return new_node(AST_PRIMITIVE,new_primitive_node(text,PRIMITIVE_BOOL));
}
AstNode* parse_nil(){
  Token* tk=consume();
  if(!expect(tk,TK_NIL)) return error(tk,"invalid nil");
  // DEBUG("nil\n");
  return new_node(AST_PRIMITIVE,new_primitive_node("nil",PRIMITIVE_NIL));
}
AstNode* parse_table(){
  Token* tk=consume();
  List* keys=new_default_list();
  List* vals=new_default_list();
  if(!specific(tk,TK_CURLY,"{")) return error(tk,"invalid table");
  tk=consume();
  while(tk && !specific(tk,TK_CURLY,"}")){
    if(!expect(tk,TK_NAME)) return error(tk,"invalid table");
    add_to_list(keys,tk->text);
    // DEBUG("key: %s\n",tk->text);
    tk=consume();
    if(!specific(tk,TK_MISC,"=")) return error(tk,"invalid table");
    AstNode* node=parse_expr();
    if(!node) return NULL;
    add_to_list(vals,node);
    tk=consume();
    if(!specific(tk,TK_CURLY,"}")){
      if(!specific(tk,TK_MISC,",")) return error(tk,"invalid table");
      tk=consume();
    }
  }
  if(!tk) return error(tk,"unclosed table");
  // DEBUG("table\n");
  return new_node(AST_TABLE,new_table_node(keys,vals));
}

// Expression parse functions
AstNode* parse_tuple(){
  List* types=new_default_list();
  List* ls=new_default_list();
  AstNode* node=parse_expr();
  if(!node) return NULL;
  add_to_list(ls,node);
  add_to_list(types,get_type(node));
  Token* tk=check();
  while(specific(tk,TK_MISC,",")){
    consume();
    node=parse_expr();
    if(!node) return NULL;
    add_to_list(ls,node);
    add_to_list(types,get_type(node));
    tk=check();
  }
  // DEBUG("tuple\n");
  AstNode* type_node=new_node(AST_TYPE_TUPLE,types);
  return new_node(AST_TUPLE,new_ast_list_node(type_node,ls));
}
AstNode* parse_expr(){
  Token* tk=check();
  AstNode* node=NULL;
  if(!tk) return error(tk,"incomplete expression");
  if(tk->type==TK_NIL){
    node=parse_nil();
  }else if(expect(tk,TK_TRUE) || expect(tk,TK_FALSE)){
    node=parse_boolean();
  }else if(expect(tk,TK_INT)){
    node=parse_number();
  }else if(expect(tk,TK_QUOTE)){
    node=parse_string();
  }else if(tk->type==TK_FUNCTION){
    node=parse_function(NULL,1);
  }else if(specific(tk,TK_CURLY,"{")){
    node=parse_table();
  }else if(specific(tk,TK_PAREN,"(")){
    tk=consume();
    node=parse_tuple();
    if(!node) return NULL;
    tk=consume();
    if(!specific(tk,TK_PAREN,")")) return error(tk,"unclosed expression");
    node=new_node(AST_PAREN,node);
  }else if(tk->type==TK_NAME){
    AstNode* lhs=parse_lhs();
    tk=check_next();
    if(specific(tk,TK_PAREN,"(")){
      consume();
      tk=check();
      if(!specific(tk,TK_PAREN,")")) node=parse_tuple();
      tk=consume();
      if(!specific(tk,TK_PAREN,")")) return error(tk,"invalid function invocation");
      // DEBUG("function call\n");
      node=new_node(AST_CALL,new_ast_ast_node(lhs,node));
    }else{
      node=lhs;
    }
  }else if(expect(tk,TK_UNARY) || specific(tk,TK_MISC,"-")){
    char* text=consume()->text;
    node=parse_expr();
    if(!node) return NULL;
    if(node->type==AST_BINARY){
      BinaryNode* data=(BinaryNode*)(node->data);
      if(strcmp(data->text,"^")){
        while(data->l->type==AST_BINARY) data=(BinaryNode*)(data->l->data);
        data->l=new_node(AST_UNARY,new_unary_node(text,data->l));
      }else{
        node=new_node(AST_UNARY,new_unary_node(text,node));
      }
    }else{
      node=new_node(AST_UNARY,new_unary_node(text,node));
    }
  }
  tk=check();
  if(expect(tk,TK_BINARY) || specific(tk,TK_MISC,"-")){
    char* op=tk->text;
    consume();
    AstNode* r=parse_expr();
    if(!r) return NULL;
    if(r->type==AST_BINARY){
      BinaryNode* br=(BinaryNode*)(r->data);
      int lp=precedence(op);
      int rp=precedence(br->text);
      if(lp>rp){
        AstNode* l=new_node(AST_BINARY,new_binary_node(op,node,br->l));
        node=new_node(AST_BINARY,new_binary_node(br->text,l,br->r));
      }else{
        node=new_node(AST_BINARY,new_binary_node(op,node,r));
      }
    }else{
      node=new_node(AST_BINARY,new_binary_node(op,node,r));
    }
  }
  if(!node) error(tk,"unexpected expression");
  return node;
}
