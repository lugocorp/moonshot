#include "./tokens.h"
#include "./types.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define DEBUG(...) printf(__VA_ARGS__)
static char error_msg[256];
static List* tokens;
static int _i;

// Preemptive function declarations
static AstNode* parse_stmt();
static AstNode* parse_call_or_set();
static AstNode* parse_function();
static AstNode* parse_repeat();
static AstNode* parse_string();
static AstNode* parse_number();
static AstNode* parse_return();
static AstNode* parse_fornum();
static AstNode* parse_tuple();
static AstNode* parse_while();
static AstNode* parse_local();
static AstNode* parse_table();
static AstNode* parse_forin();
static AstNode* parse_label();
static AstNode* parse_goto();
static AstNode* parse_expr();
static AstNode* parse_lhs();
static AstNode* parse_if();
static AstNode* parse_do();

// Errors
static AstNode* line_error(int line,const char* msg){
  sprintf(error_msg,"ERROR %s on line %i",msg,line);
  return NULL;
}
static AstNode* error(Token* tk,const char* msg){
  if(tk) return line_error(tk->line,msg);
  sprintf(error_msg,"ERROR %s",msg);
  return NULL;
}

// Parsing interface
AstNode* parse(List* ls){
  _i=0;
  tokens=ls;
  error_msg[0]=0;
  AstNode* root=parse_stmt();
  if(root){
    while(_i<tokens->n){
      if(((Token*)get_from_list(tokens,_i++))->type!=TK_SPACE){
        error(NULL,"unparsed tokens");
        // deallocate root somehow
        return NULL;
      }
    }
  }
  return root;
}
char* get_parse_error(){
  return error_msg;
}

// Node constructors
static AstNode* new_node(int type,void* data){
  AstNode* node=(AstNode*)malloc(sizeof(AstNode));
  node->type=type;
  node->data=data;
  return node;
}
static ValueNode* new_value_node(char* type,char* text){
  ValueNode* node=(ValueNode*)malloc(sizeof(ValueNode));
  strcpy(node->text,text);
  strcpy(node->type,type);
  return node;
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

// Statement group parse functions
static AstNode* parse_stmt(){
  Token* tk;
  AstNode* node;
  while(1){
    tk=check();
    if(!tk) break;
    //if(expect(tk,TK_FUNCTION)) node=parse_function();
    //else if(expect(tk,TK_IF)) node=parse_if();
    /*else */if(expect(tk,TK_NAME)) node=parse_call_or_set();
    else if(expect(tk,TK_RETURN)) node=parse_return();
    else if(expect(tk,TK_DBCOLON)) node=parse_label();
    else if(expect(tk,TK_LOCAL)) node=parse_local();
    else if(expect(tk,TK_REPEAT)) node=parse_repeat();
    else if(expect(tk,TK_WHILE)) node=parse_while();
    else if(expect(tk,TK_GOTO)) node=parse_goto();
    else if(expect(tk,TK_DO)) node=parse_do();
    /*else if(expect(tk,TK_FOR)){
      int line=tk->line;
      tk=check_ahead(3);
      if((tk->type==TK_MISC && !strcmp(tk->text,",")) || tk->type==TK_IN) error=parse_forin();
      else if(tk->type==TK_MISC && !strcmp(tk->text,"=")) error=parse_fornum();
      else{
        printf("ERROR invalid for loop on line %i\n",line);
        error=1;
      }
    }*/
    else break;
    if(!node) return NULL;
  }
  return new_node(AST_PRIMITIVE,node);
}
static AstNode* parse_do(){
  Token* tk=consume();
  if(!expect(tk,TK_DO)) return error(tk,"invalid do block");
  AstNode* node=parse_stmt();
  if(!node) return NULL;
  tk=consume();
  if(!expect(tk,TK_END)) return error(tk,"invalid do block");
  DEBUG("do stmt\n");
  return new_node(AST_PRIMITIVE,node);
}

// Statement parse functions
static AstNode* parse_call_or_set(){
  AstNode* node=parse_lhs();
  if(!node) return NULL;
  Token* tk=consume();
  if(specific(tk,TK_PAREN,"(")){
    tk=check();
    if(tk && !specific(tk,TK_PAREN,")")){
      node=parse_tuple();
      if(!node) return NULL;
    }
    tk=consume();
    if(!specific(tk,TK_PAREN,")")) return error(tk,"invalid function invocation");
    DEBUG("call node\n");
  }else if(specific(tk,TK_MISC,"=")){
    node=parse_expr();
    if(!node) return NULL;
    DEBUG("set node\n");
  }else{
    return error(tk,"unexpected lhs");
  }
  return node;
}
static AstNode* parse_return(){
  Token* tk=consume();
  if(!expect(tk,TK_RETURN)) return error(tk,"invalid return statement");
  AstNode* node=parse_expr();
  if(!node) return NULL;
  return new_node(AST_PRIMITIVE,NULL);
}
static AstNode* parse_lhs(){ // TODO finalize output
  Token* tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid left-hand side of statement");
  DEBUG("lhs: %s\n",tk->text);
  tk=check();
  if(specific(tk,TK_MISC,",")){
    while(specific(tk,TK_MISC,",")){
      consume();
      tk=consume();
      if(!expect(tk,TK_NAME)) return error(tk,"invalid left-hand tuple");
      DEBUG(",%s\n",tk->text);
      tk=check();
    }
  }else{
    int attempt=(tk)?1:0;
    while(attempt){
      if(specific(tk,TK_SQUARE,"[")){
        consume();
        DEBUG("square bracket\n");
        AstNode* node=parse_expr();
        if(!node) return NULL;
        tk=consume();
        if(!specific(tk,TK_SQUARE,"]")) return error(tk,"invalid property");
      }
      if(specific(tk,TK_MISC,".")){
        consume();
        tk=consume();
        DEBUG(".%s\n",tk->text);
        if(!expect(tk,TK_NAME)) return error(tk,"invalid field");
      }
      tk=check();
      attempt=specific(tk,TK_MISC,".") || specific(tk,TK_SQUARE,"[");
    }
  };
  return new_node(AST_PRIMITIVE,NULL);
}
static AstNode* parse_local(){
  Token* tk=consume();
  if(!expect(tk,TK_LOCAL)) return error(tk,"invalid local variable declaration");
  AstNode* node=parse_lhs();
  if(!node) return NULL;
  tk=check();
  if(specific(tk,TK_MISC,"=")){
    consume();
    node=parse_expr();
    if(!node) return NULL;
  }
  DEBUG("local\n");
  return new_node(AST_PRIMITIVE,NULL);
}

// Control parse functions
/*static int parse_function(){
  printf("function\n");
  Token* tk;
  EXPECT(TK_FUNCTION,"invalid function");
  EXPECT(TK_NAME,"invalid function");
  SPECIFIC(TK_PAREN,"(","invalid function");
  tk=check();
  while(tk && (tk->type!=TK_PAREN || strcmp(tk->text,")"))){
    SUBCALL(parse_expr());
    tk=check();
    if(tk && (tk->type!=TK_PAREN || strcmp(tk->text,")"))){
      SPECIFIC(TK_MISC,",","invalid function");
      tk=check();
    }
  }
  SPECIFIC(TK_PAREN,")","invalid function");
  SUBCALL(parse_stmt());
  EXPECT(TK_END,"invalid function");
  return 0;
}*/
static AstNode* parse_repeat(){
  Token* tk=consume();
  if(!expect(tk,TK_REPEAT)) return error(tk,"invalid repeat statement");
  AstNode* node=parse_stmt();
  if(!node) return NULL;
  tk=consume();
  if(!expect(tk,TK_UNTIL)) return error(tk,"invalid repeat statement");
  node=parse_expr();
  if(!node) return NULL;
  DEBUG("repeat\n");
  return new_node(AST_PRIMITIVE,NULL);
}
static AstNode* parse_while(){
  Token* tk=consume();
  if(!expect(tk,TK_WHILE)) return error(tk,"invalid while statement");
  AstNode* node=parse_expr();
  if(!node) return NULL;
  tk=consume();
  if(!expect(tk,TK_DO)) return error(tk,"invalid while statement");
  node=parse_stmt();
  if(!node) return NULL;
  tk=consume();
  if(!expect(tk,TK_END)) return error(tk,"invalid while statement");
  DEBUG("while\n");
  return new_node(AST_PRIMITIVE,NULL);
}
/*static int parse_if(){
  printf("if\n");
  Token* tk;
  EXPECT(TK_IF,"invalid if statement");
  SUBCALL(parse_expr());
  EXPECT(TK_THEN,"invalid if statement");
  SUBCALL(parse_stmt());
  EXPECT(TK_END,"invalid if statement");
  return 0;
}
static int parse_fornum(){
  printf("for number\n");
  Token* tk;
  EXPECT(TK_FOR,"invalid for loop");
  EXPECT(TK_NAME,"invalid for loop");
  SPECIFIC(TK_MISC,"=","invalid for loop");
  SUBCALL(parse_number());
  SPECIFIC(TK_MISC,",","invalid for loop");
  SUBCALL(parse_number());
  tk=check();
  if(tk && tk->type==TK_MISC && !strcmp(tk->text,",")){
    consume();
    SUBCALL(parse_number());
  }
  EXPECT(TK_DO,"invalid for loop");
  SUBCALL(parse_stmt());
  EXPECT(TK_END,"invalid for loop");
  return 0;
}
static int parse_forin(){
  printf("for in\n");
  Token* tk;
  EXPECT(TK_FOR,"invalid for loop");
  EXPECT(TK_NAME,"invalid for loop");
  tk=check();
  while(tk && tk->type==TK_MISC && !strcmp(tk->text,",")){
    consume();
    EXPECT(TK_NAME,"invalid for loop");
    tk=check();
  }
  EXPECT(TK_IN,"invalid for loop");
  SUBCALL(parse_expr());
  tk=check();
  while(tk && tk->type==TK_MISC && !strcmp(tk->text,",")){
    consume();
    SUBCALL(parse_expr());
    tk=check();
  }
  EXPECT(TK_DO,"invalid for loop");
  SUBCALL(parse_stmt());
  EXPECT(TK_END,"invalid for loop");
  return 0;
}*/
static AstNode* parse_label(){
  char text[256];
  Token* tk=consume();
  if(!expect(tk,TK_DBCOLON)) return error(tk,"invalid label");
  tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid label");
  strcpy(text,tk->text);
  tk=consume();
  if(!expect(tk,TK_DBCOLON)) return error(tk,"invalid label");
  DEBUG("label %s\n",text);
  return new_node(AST_PRIMITIVE,NULL);
}
static AstNode* parse_goto(){
  char text[256];
  Token* tk=consume();
  if(!expect(tk,TK_GOTO)) return error(tk,"invalid goto statement");
  tk=consume();
  if(!expect(tk,TK_NAME)) return error(tk,"invalid goto statement");
  strcpy(text,tk->text);
  DEBUG("goto %s\n",text);
  return new_node(AST_PRIMITIVE,NULL);
}

// Primitive types parse functions
static AstNode* parse_string(){
  char text[256];
  Token* tk=consume();
  if(!expect(tk,TK_QUOTE)) return error(tk,"invalid string");
  char* quote=tk->text;
  int line=tk->line;
  strcpy(text,quote);
  tk=consume_next();
  while(tk && !specific(tk,TK_QUOTE,quote)){
    strcat(text,tk->text);
    tk=consume_next();
  }
  if(tk) strcat(text,quote);
  else return line_error(line,"unclosed string");
  DEBUG("string %s\n",text);
  return new_node(AST_PRIMITIVE,new_value_node("string",text));
}
static AstNode* parse_number(){
  char type[256];
  char text[256];
  Token* tk=consume();
  if(!expect(tk,TK_INT)) return error(tk,"invalid number");
  strcpy(text,tk->text);
  sprintf(type,"int");
  tk=check_next();
  if(specific(tk,TK_MISC,".")){
    sprintf(type,"float");
    consume();
    tk=consume();
    if(!expect(tk,TK_INT)) return error(tk,"invalid number");
    strcat(text,".");
    strcat(text,tk->text);
  }
  DEBUG("%s %s\n",type,text);
  return new_node(AST_PRIMITIVE,new_value_node(type,text));
}
static AstNode* parse_boolean(){
  char text[6];
  Token* tk=consume();
  if(!expect(tk,TK_TRUE) && !expect(tk,TK_FALSE)) return error(tk,"invalid boolean");
  strcpy(text,tk->text);
  DEBUG("boolean %s\n",text);
  return new_node(AST_PRIMITIVE,new_value_node("boolean",text));
}
static AstNode* parse_nil(){
  char text[4];
  Token* tk=consume();
  if(!expect(tk,TK_NIL)) return error(tk,"invalid nil");
  DEBUG("nil\n");
  return new_node(AST_PRIMITIVE,new_value_node("boolean","nil"));
}
static AstNode* parse_table(){ // TODO finalize table output
  Token* tk=consume();
  if(!specific(tk,TK_CURLY,"{")) return error(tk,"invalid table");
  tk=consume();
  while(tk && !specific(tk,TK_CURLY,"}")){
    if(!expect(tk,TK_NAME)) return error(tk,"invalid table");
    DEBUG("key: %s\n",tk->text);
    tk=consume();
    if(!specific(tk,TK_MISC,"=")) return error(tk,"invalid table");
    AstNode* node=parse_expr();
    if(!node) return NULL;
    tk=consume();
    if(!specific(tk,TK_CURLY,"}")){
      if(!specific(tk,TK_MISC,",")) return error(tk,"invalid table");
      tk=consume();
    }
  }
  if(!tk) return error(tk,"unclosed table");
  DEBUG("table\n");
  return new_node(AST_TABLE,NULL);
}

// Expression parse functions
static AstNode* parse_tuple(){ // TODO finalize output
  AstNode* node=parse_expr();
  if(!node) return NULL;
  Token* tk=check();
  while(specific(tk,TK_MISC,",")){
    consume();
    node=parse_expr();
    if(!node) return NULL;
    tk=check();
  }
  DEBUG("tuple\n");
  return new_node(AST_PRIMITIVE,NULL);
}
static AstNode* parse_expr(){
  AstNode* node;
  Token* tk=check();
  if(!tk) return error(tk,"incomplete expression");
  if(tk->type==TK_NIL) return parse_nil();
  else if(expect(tk,TK_TRUE) || expect(tk,TK_FALSE)) return parse_boolean();
  else if(expect(tk,TK_INT)){
    return parse_number();
  }else if(expect(tk,TK_QUOTE)){
    return parse_string();
  }/*else if(tk->type==TK_FUNCTION){
    SUBCALL(parse_function());
  }*/else if(specific(tk,TK_CURLY,"{")){
    return parse_table();
  }else if(specific(tk,TK_PAREN,"(")){
    tk=consume();
    node=parse_expr();
    if(!node) return NULL;
    tk=consume();
    if(!specific(tk,TK_PAREN,")")) return error(tk,"unclosed expression");
    return node;
  }else if(tk->type==TK_NAME){
    AstNode* node=parse_lhs();
    tk=check();
    if(specific(tk,TK_PAREN,"(")){
      consume();
      tk=check();
      if(!specific(tk,TK_PAREN,")")) node=parse_tuple();
      tk=consume();
      if(!specific(tk,TK_PAREN,")")) return error(tk,"invalid function invocation");
    }
    DEBUG("function call\n");
    return node;
  }
  return error(tk,"unexpected expression");
}
