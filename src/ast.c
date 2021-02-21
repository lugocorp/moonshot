#include "./tokens.h"
#include "./types.h"
#include <string.h>
#include <stdio.h>
#define SUBCALL(call) if(call) return 1;
#define EXPECT(t,msg) tk=consume(); \
  if(!tk){ \
    printf("ERROR %s\n",(msg)); \
    return 1; \
  }else if(tk->type!=(t)){ \
    printf("ERROR %s on line %i\n",(msg),tk->line); \
    return 1; \
  }
#define SPECIFIC(t,str,msg) tk=consume(); \
  if(!tk){ \
    printf("ERROR %s\n",(msg)); \
    return 1; \
  }else if(tk->type!=(t) || strcmp(tk->text,(str))){ \
    printf("ERROR %s on line %i\n",(msg),tk->line); \
    return 1; \
  }

// Token string setup
static int _i;
static List* tokens;
void set_token_string(List* ls){
  tokens=ls;
  _i=0;
}

// Parse helper functions
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
    n--;
  }
  return (a<tokens->n)?((Token*)get_from_list(tokens,a)):NULL;
}

// Preemptive function declarations
int parse_stmt();
static int parse_call_or_set();
static int parse_function();
static int parse_repeat();
static int parse_string();
static int parse_number();
static int parse_return();
static int parse_fornum();
static int parse_tuple();
static int parse_while();
static int parse_local();
static int parse_table();
static int parse_forin();
static int parse_block();
static int parse_label();
static int parse_goto();
static int parse_expr();
static int parse_lhs();
static int parse_if();
static int parse_do();

// Statement group parse functions
int parse_stmt(){
  printf("stmt\n");
  Token* tk;
  int error=0;
  while(!error){
    tk=check();
    if(!tk) break;
    if(tk->type==TK_FUNCTION) error=parse_function();
    else if(tk->type==TK_IF) error=parse_if();
    else if(tk->type==TK_NAME) error=parse_call_or_set();
    else if(tk->type==TK_RETURN) error=parse_return();
    else if(tk->type==TK_DBCOLON) error=parse_label();
    else if(tk->type==TK_REPEAT) error=parse_repeat();
    else if(tk->type==TK_WHILE) error=parse_while();
    else if(tk->type==TK_GOTO) error=parse_goto();
    else if(tk->type==TK_FOR){
      int line=tk->line;
      tk=check_ahead(3);
      if((tk->type==TK_MISC && !strcmp(tk->text,",")) || tk->type==TK_IN) error=parse_forin();
      else if(tk->type==TK_MISC && !strcmp(tk->text,"=")) error=parse_fornum();
      else{
        printf("ERROR invalid for loop on line %i\n",line);
        error=1;
      }
    }
    else break;
  }
  return error;
}
static int parse_block(){
  printf("block\n");
  Token* tk;
  SUBCALL(parse_stmt());
  EXPECT(TK_END,"unclosed block");
  return 0;
}
static int parse_do(){
  printf("do\n");
  Token* tk;
  EXPECT(TK_DO,"invalid do block");
  SUBCALL(parse_block());
  return 0;
}

// Statement parse functions
static int parse_call_or_set(){
  SUBCALL(parse_lhs());
  Token* tk=consume();
  if(tk && tk->type==TK_PAREN && !strcmp(tk->text,"(")){
    printf("call\n");
    tk=check();
    if(tk && (tk->type!=TK_PAREN || strcmp(tk->text,")"))) SUBCALL(parse_tuple());
    SPECIFIC(TK_PAREN,")","invalid function invocation");
  }else if(tk && tk->type==TK_MISC && !strcmp(tk->text,"=")){
    printf("set\n");
    SUBCALL(parse_expr());
  }else{
    printf("ERROR unexpected lhs\n");
    return 1;
  }
  return 0;
}
static int parse_return(){
  printf("return\n");
  Token* tk;
  EXPECT(TK_RETURN,"invalid return statement");
  SUBCALL(parse_expr());
  return 0;
}
static int parse_lhs(){
  printf("lhs\n");
  Token* tk;
  EXPECT(TK_NAME,"invalid left-hand side of statement");
  tk=check();
  if(tk && tk->type==TK_MISC && !strcmp(tk->text,",")){
    while(tk && tk->type==TK_MISC && !strcmp(tk->text,",")){
      consume();
      EXPECT(TK_NAME,"invalid left-hand tuple");
      tk=check();
    }
  }else{
    tk=check();
    int attempt=(tk)?1:0;
    while(attempt){
      if(tk->type==TK_SQUARE && !strcmp(tk->text,"[")){
        consume();
        SUBCALL(parse_expr());
        SPECIFIC(TK_SQUARE,"]","invalid property");
      }
      if(tk->type==TK_MISC && !strcmp(tk->text,".")){
        consume();
        EXPECT(TK_NAME,"invalid field");
      }
      tk=check();
      attempt=(tk && ((tk->type==TK_MISC && !strcmp(tk->text,".")) || (tk->type==TK_SQUARE && !strcmp(tk->text,"["))));
    }
  };
  return 0;
}
static int parse_local(){
  printf("local\n");
  Token* tk;
  EXPECT(TK_LOCAL,"invalid local variable declaration");
  SUBCALL(parse_lhs());
  tk=check();
  if(tk && tk->type==TK_MISC && !strcmp(tk->text,"=")){
    consume();
    SUBCALL(parse_expr());
  }
  return 0;
}

// Control parse functions
static int parse_function(){
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
  SUBCALL(parse_block());
  return 0;
}
static int parse_repeat(){
  printf("repeat\n");
  Token* tk;
  EXPECT(TK_REPEAT,"invalid repeat statement");
  SUBCALL(parse_stmt());
  EXPECT(TK_UNTIL,"invalid repeat statement");
  SUBCALL(parse_expr());
  return 0;
}
static int parse_while(){
  printf("while\n");
  Token* tk;
  EXPECT(TK_WHILE,"invalid while statement");
  SUBCALL(parse_expr());
  SUBCALL(parse_do());
  return 0;
}
static int parse_if(){
  printf("if\n");
  Token* tk;
  EXPECT(TK_IF,"invalid if statement");
  SUBCALL(parse_expr());
  EXPECT(TK_THEN,"invalid if statement");
  SUBCALL(parse_block());
  return 0;
}
static int parse_fornum(){
  printf("for number\n");
  Token* tk;
  EXPECT(TK_FOR,"invalid for loop");
  EXPECT(TK_NAME,"invalid for loop");
  SPECIFIC(TK_MISC,"=","invalid for loop");
  SUBCALL(parse_expr());
  SPECIFIC(TK_MISC,",","invalid for loop");
  SUBCALL(parse_expr());
  tk=check();
  if(tk && tk->type==TK_MISC && !strcmp(tk->text,",")){
    consume();
    SUBCALL(parse_expr());
  }
  SUBCALL(parse_do());
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
  SUBCALL(parse_do());
  return 0;
}
static int parse_label(){
  printf("label\n");
  Token* tk;
  EXPECT(TK_DBCOLON,"invalid label");
  EXPECT(TK_NAME,"invalid label");
  EXPECT(TK_DBCOLON,"invalid label");
  return 0;
}
static int parse_goto(){
  printf("goto\n");
  Token* tk;
  EXPECT(TK_GOTO,"invalid goto statement");
  EXPECT(TK_NAME,"invalid goto statement");
  return 0;
}

// Primitive types parse functions
static int parse_string(){
  printf("string\n");
  Token* tk;
  EXPECT(TK_QUOTE,"invalid string");
  char* quote=tk->text;
  tk=consume_next();
  while(tk && (tk->type!=TK_QUOTE || strcmp(tk->text,quote))) tk=consume_next();
  return tk==NULL;
}
static int parse_number(){
  printf("number\n");
  Token* tk;
  EXPECT(TK_INT,"invalid number");
  tk=check_next();
  if(tk->type==TK_MISC && !strcmp(tk->text,".")){
    consume();
    EXPECT(TK_INT,"invalid number");
  }
  return 0;
}
static int parse_table(){
  printf("table\n");
  Token* tk;
  SPECIFIC(TK_CURLY,"{","invalid table");
  tk=check();
  while(tk && (tk->type!=TK_CURLY || strcmp(tk->text,"}"))){
    EXPECT(TK_NAME,"invalid table");
    SPECIFIC(TK_MISC,"=","invalid table");
    SUBCALL(parse_expr());
    tk=check();
    if(tk && (tk->type!=TK_CURLY || strcmp(tk->text,"}"))){
      SPECIFIC(TK_MISC,",","invalid table");
      tk=check();
    }
  }
  SPECIFIC(TK_CURLY,"}","invalid table");
  return 0;
}

// Expression parse functions
static int parse_tuple(){
  printf("tuple\n");
  Token* tk;
  SUBCALL(parse_expr());
  tk=check();
  while(tk && tk->type==TK_MISC && !strcmp(tk->text,",")){
    consume();
    SUBCALL(parse_expr());
    tk=check();
  }
  return 0;
}
static int parse_expr(){
  printf("expression\n");
  Token* tk=check();
  if(!tk) return 1;
  if(tk->type==TK_NIL) consume();
  else if(tk->type==TK_TRUE) consume();
  else if(tk->type==TK_FALSE) consume();
  else if(tk->type==TK_INT){
    SUBCALL(parse_number());
  }else if(tk->type==TK_QUOTE){
    SUBCALL(parse_string());
  }else if(tk->type==TK_FUNCTION){
    SUBCALL(parse_function());
  }else if(tk->type==TK_PAREN && !strcmp(tk->text,"{")){
    SUBCALL(parse_table());
  }else if(tk->type==TK_PAREN && !strcmp(tk->text,"(")){
    SPECIFIC(TK_PAREN,"(","invalid expression");
    SUBCALL(parse_expr());
    SPECIFIC(TK_PAREN,")","invalid expression");
  }else if(tk->type==TK_NAME){
    SUBCALL(parse_lhs());
    tk=check();
    if(tk && tk->type==TK_PAREN && !strcmp(tk->text,"(")){
      SPECIFIC(TK_PAREN,"(","invalid function invocation");
      tk=check();
      if(tk && (tk->type!=TK_PAREN || strcmp(tk->text,")"))) SUBCALL(parse_tuple());
      SPECIFIC(TK_PAREN,")","invalid function invocation");
    }
  }else return 1;
  return 0;
}
