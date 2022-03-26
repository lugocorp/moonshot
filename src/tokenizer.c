#include "./internal.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define TOKEN_BUFFER_LENGTH 256 // Max length for a token string
#define KEY_TOKEN(s, t) else if (!strcmp(buffer, s)) tk->type = t;
#define SPECIAL_TOKEN(s, l, t)                         \
  else if (n - a >= l && !strncmp(buffer + a, s, l))   \
  {                                                    \
    tk->text = (char *)malloc(sizeof(char) * (l + 1)); \
    sprintf(tk->text, s);                              \
    tk->type = t;                                      \
    a += l;                                            \
  }
static int class_alphanumeric = 0; // Represents the alphanumeric token class
static int class_whitespace = 1;   // Represents the whitespace token class
static int class_special = 2;      // Represents the special token class
static int multiline_comment = 0;  // Flag for tokenizing within a multiline comment
static int comment = 0;            // Flag for tokenizing within a single line comment

/*
  Get the character class for a char
  Helps detect boundaries for tokens
*/
static int get_char_class(char c)
{
  if (c == '_' || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9'))
    return class_alphanumeric;
  if (c == ' ' || c == '\t' || c == '\n')
    return class_whitespace;
  return class_special;
}

/*
  Deallocate a token
*/
void dealloc_token(Token *tk)
{
  free(tk->text);
  free(tk);
}

/*
  Generates tokens from a buffer of similarly-classes characters
*/
static void discover_tokens(List *ls, int line, char *buffer, int n, int char_class)
{
  buffer[n] = 0;
  if (char_class != class_special)
  {

    // Comment tokenization
    if (comment && char_class == class_whitespace)
    {
      for (int a = 0; a < n; a++)
      {
        if (buffer[a] == '\n')
          comment = 0;
      }
      return;
    }
    if (multiline_comment || comment)
      return;

    // Whitespace and alphanumeric tokenization
    Token *tk = (Token *)malloc(sizeof(Token));
    tk->text = (char *)malloc(sizeof(char) * (n + 1));
    strcpy(tk->text, buffer);
    add_to_list(ls, tk);
    tk->line = line;
    if (char_class == class_whitespace)
      tk->type = TK_SPACE;
    else
    {
      if (!strcmp(buffer, "and"))
        tk->type = TK_BINARY;
      KEY_TOKEN("constructor", TK_CONSTRUCTOR)
      KEY_TOKEN("implements", TK_IMPLEMENTS)
      KEY_TOKEN("interface", TK_INTERFACE)
      KEY_TOKEN("function", TK_FUNCTION)
      KEY_TOKEN("extends", TK_EXTENDS)
      KEY_TOKEN("require", TK_REQUIRE)
      KEY_TOKEN("typedef", TK_TYPEDEF)
      KEY_TOKEN("elseif", TK_ELSEIF)
      KEY_TOKEN("repeat", TK_REPEAT)
      KEY_TOKEN("return", TK_RETURN)
      KEY_TOKEN("local", TK_LOCAL)
      KEY_TOKEN("break", TK_BREAK)
      KEY_TOKEN("false", TK_FALSE)
      KEY_TOKEN("class", TK_CLASS)
      KEY_TOKEN("where", TK_WHERE)
      KEY_TOKEN("trust", TK_UNARY)
      KEY_TOKEN("super", TK_SUPER)
      KEY_TOKEN("until", TK_UNTIL)
      KEY_TOKEN("while", TK_WHILE)
      KEY_TOKEN("final", TK_FINAL)
      KEY_TOKEN("then", TK_THEN)
      KEY_TOKEN("true", TK_TRUE)
      KEY_TOKEN("goto", TK_GOTO)
      KEY_TOKEN("not", TK_UNARY)
      KEY_TOKEN("or", TK_BINARY)
      KEY_TOKEN("as", TK_BINARY)
      KEY_TOKEN("else", TK_ELSE)
      KEY_TOKEN("new", TK_NEW)
      KEY_TOKEN("var", TK_VAR)
      KEY_TOKEN("end", TK_END)
      KEY_TOKEN("for", TK_FOR)
      KEY_TOKEN("nil", TK_NIL)
      KEY_TOKEN("do", TK_DO)
      KEY_TOKEN("if", TK_IF)
      KEY_TOKEN("in", TK_IN)
      else
      {
        tk->type = TK_INT;
        for (int a = 0; a < n; a++)
        {
          if (buffer[a] < '0' || buffer[a] > '9')
          {
            tk->type = TK_NAME;
            break;
          }
        }
      }
    }
  }
  else
  { // Special class tokenization
    int a = 0;
    while (a < n)
    {
      if (multiline_comment)
      {
        if (n - a >= 2 && !strncmp(buffer + a, "]]", 2))
        {
          multiline_comment = 0;
          a++;
        }
        a++;
        continue;
      }
      if (comment)
        break;
      if (n - a >= 4 && !strncmp(buffer + a, "--[[", 4))
      {
        multiline_comment = 1;
        a += 4;
        continue;
      }
      if (n - a >= 2 && !strncmp(buffer + a, "--", 2))
      {
        comment = 1;
        break;
      }
      Token *tk = (Token *)malloc(sizeof(Token));
      tk->text = NULL;
      tk->line = line;
      if (n - a >= 3 && !strncmp(buffer + a, "...", 3))
      {
        tk->text = (char *)malloc(sizeof(char) * 4);
        sprintf(tk->text, "...");
        tk->type = TK_DOTS;
        a += 3;
      }
      SPECIAL_TOKEN("..", 2, TK_BINARY)
      SPECIAL_TOKEN("<=", 2, TK_BINARY)
      SPECIAL_TOKEN(">=", 2, TK_BINARY)
      SPECIAL_TOKEN("==", 2, TK_BINARY)
      SPECIAL_TOKEN("~=", 2, TK_BINARY)
      SPECIAL_TOKEN("::", 2, TK_DBCOLON)
      SPECIAL_TOKEN("<", 1, TK_BINARY)
      SPECIAL_TOKEN(">", 1, TK_BINARY)
      SPECIAL_TOKEN("+", 1, TK_BINARY)
      SPECIAL_TOKEN("^", 1, TK_BINARY)
      SPECIAL_TOKEN("*", 1, TK_BINARY)
      SPECIAL_TOKEN("/", 1, TK_BINARY)
      SPECIAL_TOKEN("#", 1, TK_UNARY)
      SPECIAL_TOKEN("'", 1, TK_QUOTE)
      SPECIAL_TOKEN("\"", 1, TK_QUOTE)
      SPECIAL_TOKEN("(", 1, TK_PAREN)
      SPECIAL_TOKEN(")", 1, TK_PAREN)
      SPECIAL_TOKEN("{", 1, TK_CURLY)
      SPECIAL_TOKEN("}", 1, TK_CURLY)
      SPECIAL_TOKEN("[", 1, TK_SQUARE)
      SPECIAL_TOKEN("]", 1, TK_SQUARE)
      else
      {
        tk->text = (char *)malloc(sizeof(char) * 2);
        tk->text[0] = buffer[a];
        tk->type = TK_MISC;
        tk->text[1] = 0;
        a++;
      }
      add_to_list(ls, tk);
    }
  }
}

/*
  Read through some Lua code and tokenize it along the way
  Returns a list of Tokens
*/
List *tokenize(FILE *f)
{
  int line = 1;
  if (!f)
    return NULL;
  int current_class = -1;
  List *ls = new_list(100);
  char buffer[TOKEN_BUFFER_LENGTH];
  int i = 0;
  while (1)
  {
    char c = fgetc(f);
    if (feof(f))
    {
      if (i)
        discover_tokens(ls, line, buffer, i, current_class);
      break;
    }
    int char_class = get_char_class(c);
    if (current_class != -1 && char_class != current_class)
    {
      discover_tokens(ls, line, buffer, i, current_class);
      i = 0;
    }
    current_class = char_class;
    if (c == '\n')
      line++;
    if (i == TOKEN_BUFFER_LENGTH)
    {
      for (int a = 0; a < ls->n; a++)
        dealloc_token((Token *)get_from_list(ls, a));
      dealloc_list(ls);
      return NULL;
    }
    buffer[i++] = c;
  }
  return ls;
}
