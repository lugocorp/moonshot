#include "./tokens.h"
#include "./types.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define KEY_TOKEN(s,t) else if(!strcmp(buffer,s)) tk->type=t;
#define SPECIAL_TOKEN(s,l,t) else if(n-a>=l && !strncmp(buffer+a,s,l)){ \
    tk->text=(char*)malloc(sizeof(char)*(l+1)); \
    sprintf(tk->text,s); \
    tk->type=t; \
    a+=l; \
  }

// Character classes
static int class_alphanumeric=0;
static int class_whitespace=1;
static int class_special=2;

// Preemptive function definitions
static void discover_tokens(List* ls,int line,char* buffer,int n,int char_class);

/*
  Deallocate a Token
*/
void dealloc_token(Token* tk){
  if(tk->text) free(tk->text);
  free(tk);
}

/*
  Get the character class for a char
  Helps detect boundaries for tokens
*/
static int get_char_class(char c){
  if(c=='_' || ('a'<=c && c<='z') || ('A'<=c && c<='Z') || ('0'<=c && c<='9')) return class_alphanumeric;
  if(c==' ' || c=='\t' || c=='\n') return class_whitespace;
  return class_special;
}

/*
  Stream some Lua code and tokenize it along the way
*/
List* tokenize(FILE* f){
  int line=1;
  if(!f) return NULL;
  int current_class=-1;
  List* ls=new_list(100);
  char buffer[100]; // TODO replace fixed length array with dynamic length array
  int i=0;
  while(1){
    char c=fgetc(f);
    if(feof(f)){
      if(i) discover_tokens(ls,line,buffer,i,current_class);
      break;
    }
    fflush(stdout);
    int char_class=get_char_class(c);
    if(current_class!=-1 && char_class!=current_class){
      discover_tokens(ls,line,buffer,i,current_class);
      i=0;
    }
    current_class=char_class;
    if(c=='\n') line++;
    buffer[i++]=c;
  }
  return ls;
}

/*
  Generate tokens from a group of similar characters
  Returns 0 if there's no error
*/
static void discover_tokens(List* ls,int line,char* buffer,int n,int char_class){
  buffer[n]=0;
  if(char_class!=class_special){
    Token* tk=(Token*)malloc(sizeof(Token));
    tk->text=(char*)malloc(sizeof(char)*(n+1));
    strcpy(tk->text,buffer);
    add_to_list(ls,(void*)tk);
    tk->line=line;
    if(char_class==class_whitespace) tk->type=TK_SPACE;
    else{
      if(!strcmp(buffer,"and")) tk->type=TK_AND;
      KEY_TOKEN("break",TK_BREAK)
      KEY_TOKEN("do",TK_DO)
      KEY_TOKEN("else",TK_ELSE)
      KEY_TOKEN("elseif",TK_ELSEIF)
      KEY_TOKEN("end",TK_END)
      KEY_TOKEN("false",TK_FALSE)
      KEY_TOKEN("for",TK_FOR)
      KEY_TOKEN("function",TK_FUNCTION)
      KEY_TOKEN("goto",TK_GOTO)
      KEY_TOKEN("if",TK_IF)
      KEY_TOKEN("in",TK_IN)
      KEY_TOKEN("local",TK_LOCAL)
      KEY_TOKEN("nil",TK_NIL)
      KEY_TOKEN("not",TK_NOT)
      KEY_TOKEN("or",TK_OR)
      KEY_TOKEN("repeat",TK_REPEAT)
      KEY_TOKEN("return",TK_RETURN)
      KEY_TOKEN("then",TK_THEN)
      KEY_TOKEN("true",TK_TRUE)
      KEY_TOKEN("until",TK_UNTIL)
      KEY_TOKEN("while",TK_WHILE)
      else{
        tk->type=TK_INT;
        for(int a=0;a<n;a++){
          if(buffer[a]<'0' || buffer[a]>'9'){
            tk->type=TK_NAME;
            break;
          }
        }
      }
    }
  }else{
    int a=0;
    while(a<n){
      Token* tk=(Token*)malloc(sizeof(Token));
      tk->text=NULL;
      tk->line=line;
      if(n-a>=3 && !strncmp(buffer+a,"...",3)){
        tk->text=(char*)malloc(sizeof(char)*4);
        sprintf(tk->text,"...");
        tk->type=TK_DOTS;
        a+=3;
      }
      SPECIAL_TOKEN("--[[",4,TK_COMMENT)
      SPECIAL_TOKEN("..",2,TK_DOTS)
      SPECIAL_TOKEN("<=",2,TK_LE)
      SPECIAL_TOKEN(">=",2,TK_GE)
      SPECIAL_TOKEN("==",2,TK_EQ)
      SPECIAL_TOKEN("!=",2,TK_NE)
      SPECIAL_TOKEN("::",2,TK_DBCOLON)
      SPECIAL_TOKEN("--",2,TK_COMMENT)
      SPECIAL_TOKEN("<",1,TK_SHL)
      SPECIAL_TOKEN(">",1,TK_SHR)
      SPECIAL_TOKEN("'",1,TK_QUOTE)
      SPECIAL_TOKEN("\"",1,TK_QUOTE)
      SPECIAL_TOKEN("(",1,TK_PAREN)
      SPECIAL_TOKEN(")",1,TK_PAREN)
      SPECIAL_TOKEN("{",1,TK_CURLY)
      SPECIAL_TOKEN("}",1,TK_CURLY)
      SPECIAL_TOKEN("[",1,TK_SQUARE)
      SPECIAL_TOKEN("]",1,TK_SQUARE)
      else{
        tk->text=(char*)malloc(sizeof(char)*2);
        tk->text[0]=buffer[a];
        tk->type=TK_MISC;
        tk->text[1]=0;
        a++;
      }
      add_to_list(ls,(void*)tk);
    }
  }
}
