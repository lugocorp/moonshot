#include "./tokens.h"
#include "./types.h"
#include <string.h>
#include <stdio.h>
#define EXPECT(t,msg) tk=consume(); \
  if(!tk || tk->type!=t){ \
    printf("ERROR %s on line %i",msg,tk->line); \
    return 1; \
  }
#define SPECIFIC(t,str,msg) tk=consume(); \
  if(!tk || tk->type!=t || strcmp(tk->text,str)){ \
    printf("ERROR %s on line %i",msg,tk->line); \
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

// Preemptive function declarations
int parse_stmt();
static int parse_call_or_set();
static int parse_function();
static int parse_string();
static int parse_number();
static int parse_block();
static int parse_expr();
static int parse_lhs();
static int parse_if();

// AST parse node functions
int parse_stmt(){
  printf("Stmt\n");
  int error=0;
  while(!error){
    Token* tk=check();
    if(!tk) break;
    if(tk->type==TK_FUNCTION) error=parse_function();
    else if(tk->type==TK_IF) error=parse_if();
    else if(tk->type==TK_NAME) error=parse_call_or_set();
    else break;
  }
  return error;
}
static int parse_block(){
  Token* tk;
  printf("Block\n");
  if(parse_stmt()) return 1;
  /*Token* tk=consume();
  if(tk->type!=TK_END) return 1;*/
  EXPECT(TK_END,"unclosed block");
  return 0;
}
static int parse_function(){
  printf("Function\n");
  Token* tk=consume();
  if(tk->type!=TK_FUNCTION) return 1;
  tk=consume();
  if(tk->type!=TK_NAME) return 1;
  tk=consume();
  if(tk->type!=TK_PAREN || strcmp(tk->text,"(")) return 1;
  tk=consume();
  if(tk->type!=TK_PAREN || strcmp(tk->text,")")) return 1;
  if(parse_block()) return 1;
  return 0;
}
static int parse_if(){
  printf("If\n");
  Token* tk=consume();
  if(tk->type!=TK_IF) return 1;
  if(parse_expr()) return 1;
  tk=consume();
  if(tk->type!=TK_THEN) return 1;
  if(parse_block()) return 1;
  return 0;
}
static int parse_expr(){
  printf("Expr\n");
  if(parse_lhs()) return 1;
  Token* tk=consume();
  if(tk->type!=TK_EQ) return 1;
  if(parse_number()) return 1;
  return 0;
}
static int parse_lhs(){
  printf("LHS\n");
  Token* tk=consume();
  if(tk->type!=TK_NAME) return 1;
  return 0;
}
static int parse_call_or_set(){
  if(parse_lhs()) return 1;
  Token* tk=consume();
  if(tk->type==TK_PAREN && !strcmp(tk->text,"(")){
    printf("Call\n");
    if(parse_string()) return 1;
    tk=consume();
    if(tk->type!=TK_PAREN || strcmp(tk->text,")")) return 1;
  }else if(tk->type==TK_MISC && !strcmp(tk->text,"=")){
    printf("Set\n");
    if(parse_expr()) return 1;
  }else return 1;
  return 0;
}
static int parse_string(){
  printf("String\n");
  Token* tk=consume();
  if(tk->type!=TK_QUOTE) return 1;
  char* quote=tk->text;
  tk=consume();
  if(!tk) return 1;
  while(tk->type!=TK_QUOTE || strcmp(tk->text,quote)){
    tk=consume();
    if(!tk) return 1;
  }
  return 0;
}
static int parse_number(){
  printf("Number\n");
  Token* tk=consume();
  if(tk->type!=TK_INT) return 1;
  tk=check();
  if(tk->type==TK_MISC && !strcmp(tk->text,".")){
    consume();
    tk=consume();
    if(tk->type!=TK_INT) return 1;
  }
  return 0;
}
