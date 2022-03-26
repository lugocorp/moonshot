#define MOONSHOT_PARSING
#include "./internal.h"
#undef MOONSHOT_PARSING
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define UNARY_PRECEDENCE 6 // Precedence level for unary operators
static List *tokens;       // List of Tokens
static int _i;             // Index of the Token that's next to be consumed

/*
  Wrapper for adding a compilation error
  Pulls the line number from a Token
*/
static AstNode *error(Token *tk, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  add_error_internal(tk ? tk->line : -1, msg, args);
  va_end(args);
  return NULL;
}

// Error wrappers that also deallocate data
#define FREE_LIST(ret, ls) \
  {                        \
    dealloc_list(ls);      \
    return ret;            \
  }
#define FREE_AST_NODE(ret, node) \
  {                              \
    dealloc_ast_node(node);      \
    return ret;                  \
  }
#define FREE_AST_NODE_LIST(ret, ls)                      \
  {                                                      \
    for (int a = 0; a < ls->n; a++)                      \
      dealloc_ast_node((AstNode *)get_from_list(ls, a)); \
    dealloc_list(ls);                                    \
    return ret;                                          \
  }
#define FREE_STRING_AST_NODE_LIST(ret, ls)                         \
  {                                                                \
    for (int a = 0; a < ls->n; a++)                                \
    {                                                              \
      StringAstNode *node = (StringAstNode *)get_from_list(ls, a); \
      dealloc_ast_node(node->node);                                \
      free(node);                                                  \
    }                                                              \
    dealloc_list(ls);                                              \
    return ret;                                                    \
  }
#define FREE_AST_NODE_AND_LIST(ret, node, ls) \
  {                                           \
    dealloc_ast_node(node);                   \
    dealloc_list(ls);                         \
    return ret;                               \
  }
#define FREE_2_LISTS(ret, ls1, ls2) \
  {                                 \
    dealloc_list(ls1);              \
    dealloc_list(ls2);              \
    return ret;                     \
  }
#define FREE_2_AST_NODES(ret, node1, node2) \
  {                                         \
    dealloc_ast_node(node1);                \
    dealloc_ast_node(node2);                \
    return ret;                             \
  }

/*
  The top-level parser interface function
  Takes in a Tokens list and returns an AST representation of your Moonshot source code
*/
AstNode *parse(List *ls)
{
  _i = 0;
  tokens = ls;
  AstNode *root = parse_stmt();
  if (root)
  {
    Token *tk;
    while (_i < tokens->n)
    {
      if ((tk = (Token *)get_from_list(tokens, _i++))->type != TK_SPACE)
      {
        error(tk, "unparsed tokens", NULL);
        dealloc_ast_node(root);
        return NULL;
      }
    }
  }
  return root;
}

/*
  Consumes the next non-whitespace Token and returns it
*/
static Token *consume()
{
  while (_i < tokens->n && ((Token *)get_from_list(tokens, _i))->type == TK_SPACE)
    _i++;
  return (_i < tokens->n) ? ((Token *)get_from_list(tokens, _i++)) : NULL;
}

/*
  Looks ahead at the next non-whitespace Token and returns it
*/
static Token *check()
{
  int a = _i;
  while (a < tokens->n && ((Token *)get_from_list(tokens, a))->type == TK_SPACE)
    a++;
  return (a < tokens->n) ? ((Token *)get_from_list(tokens, a)) : NULL;
}

/*
  Consumes the next Token and returns it
*/
static Token *consume_next()
{
  if (_i < tokens->n)
    return (Token *)get_from_list(tokens, _i++);
  return NULL;
}

/*
  Looks ahead at the next Token and returns it
*/
static Token *check_next()
{
  if (_i < tokens->n)
    return (Token *)get_from_list(tokens, _i);
  return NULL;
}

/*
  Looks ahead the nth next non-whitespace Token and returns it
*/
static Token *check_ahead(int n)
{
  int a = _i;
  while (n)
  {
    while (a < tokens->n && ((Token *)get_from_list(tokens, a))->type == TK_SPACE)
      a++;
    if (n > 1 && a < tokens->n)
      a++;
    n--;
  }
  return (a < tokens->n) ? ((Token *)get_from_list(tokens, a)) : NULL;
}

/*
  Returns 1 if the Token is of type type
*/
static int expect(Token *tk, int type)
{
  return tk && tk->type == type;
}

/*
  Returns 1 if the Token is of type type and has text val
*/
static int specific(Token *tk, int type, const char *val)
{
  return tk && tk->type == type && !strcmp(tk->text, val);
}

/*
  Returns the precedence level of a binary operator
*/
static int precedence(char *op)
{
  if (!strcmp(op, "as"))
    return 8;
  if (!strcmp(op, "^"))
    return 7;
  if (!strcmp(op, "*") || !strcmp(op, "/"))
    return 5;
  if (!strcmp(op, "+") || !strcmp(op, "-"))
    return 4;
  if (!strcmp(op, ".."))
    return 3;
  if (!strcmp(op, "<=") || !strcmp(op, ">=") || !strcmp(op, "<") || !strcmp(op, ">") || !strcmp(op, "==") || !strcmp(op, "~="))
    return 2;
  if (!strcmp(op, "and"))
    return 1;
  return 0;
}

// Statement block parsers
AstNode *parse_stmt()
{
  int line = -1;
  Token *tk;
  AstNode *node;
  List *ls = new_default_list();
  while (1)
  {
    tk = check();
    if (!tk)
      break;
    if (line < 0)
      line = tk->line;
    if (expect(tk, TK_FUNCTION))
      node = parse_function(NULL, 1);
    else if (expect(tk, TK_IF))
      node = parse_if();
    else if (expect(tk, TK_SUPER))
      node = parse_super();
    else if (expect(tk, TK_CLASS))
      node = parse_class();
    else if (expect(tk, TK_INTERFACE))
      node = parse_interface();
    else if (expect(tk, TK_TYPEDEF))
      node = parse_typedef();
    else if (expect(tk, TK_REQUIRE))
      node = parse_require();
    else if (expect(tk, TK_RETURN))
      node = parse_return();
    else if (expect(tk, TK_DBCOLON))
      node = parse_label();
    else if (expect(tk, TK_LOCAL))
      node = parse_local();
    else if (expect(tk, TK_BREAK))
      node = parse_break();
    else if (expect(tk, TK_REPEAT))
      node = parse_repeat();
    else if (expect(tk, TK_WHILE))
      node = parse_while();
    else if (expect(tk, TK_GOTO))
      node = parse_goto();
    else if (expect(tk, TK_DO))
      node = parse_do();
    else if (specific(tk, TK_BINARY, "*"))
      node = parse_function_or_define();
    else if (expect(tk, TK_CONSTRUCTOR))
      node = error(tk, "invalid constructor without a class", NULL);
    else if (specific(tk, TK_PAREN, "("))
    {
      AstNode *type = parse_type();
      if (type)
        node = parse_function(type, 1);
      else
        node = NULL;
    }
    else if (expect(tk, TK_FOR))
    {
      tk = check_ahead(3);
      if (specific(tk, TK_MISC, ",") || expect(tk, TK_IN))
        node = parse_forin();
      else if (specific(tk, TK_MISC, "="))
        node = parse_fornum();
      else
        node = error(tk, "invalid loop", NULL);
    }
    else if (expect(tk, TK_NAME) || expect(tk, TK_VAR))
    {
      tk = check_ahead(2);
      if (specific(tk, TK_PAREN, "(") || specific(tk, TK_SQUARE, "[") || specific(tk, TK_MISC, "=") || specific(tk, TK_MISC, ".") || specific(tk, TK_MISC, ","))
        node = parse_set_or_call();
      else if (expect(tk, TK_VAR) || expect(tk, TK_NAME))
        node = parse_function_or_define();
      else
        node = error(tk, "invalid statement", NULL);
    }
    else
    {
      break;
    }
    if (node)
      add_to_list(ls, node);
    else
      FREE_AST_NODE_LIST(NULL, ls);
  }
  return new_node(AST_STMT, line, ls);
}
AstNode *parse_do()
{
  Token *tk = consume();
  if (!expect(tk, TK_DO))
    return error(tk, "invalid do block", NULL);
  int line = tk->line;
  AstNode *node = parse_stmt();
  if (!node)
    return NULL;
  tk = consume();
  if (!expect(tk, TK_END))
    return error(tk, "unclosed do block", NULL);
  return new_node(AST_DO, line, (List *)(node->data));
}

// Entity parsers (classes and interfaces)
AstNode *parse_interface()
{
  char *parent = NULL;
  Token *tk = consume();
  if (!expect(tk, TK_INTERFACE))
    return error(tk, "invalid interface", NULL);
  int line = tk->line;
  tk = consume();
  if (!expect(tk, TK_NAME))
    return error(tk, "invalid name for interface", NULL);
  char *name = tk->text;
  tk = check();
  if (expect(tk, TK_EXTENDS))
  {
    consume();
    tk = consume();
    if (!expect(tk, TK_NAME))
      return error(tk, "invalid parent for interface %s", name);
    parent = tk->text;
  }
  tk = consume();
  if (!expect(tk, TK_WHERE))
    return error(tk, "invalid interface %s", name);
  tk = check();
  List *ls = new_default_list();
  while (tk && !expect(tk, TK_END))
  {
    AstNode *type = NULL;
    if (!expect(tk, TK_FUNCTION))
    {
      type = parse_type();
      if (!type)
        FREE_AST_NODE_LIST(NULL, ls);
    }
    AstNode *func = parse_function(type, 0);
    if (!func)
    {
      dealloc_ast_type(type);
      FREE_AST_NODE_LIST(NULL, ls);
    }
    add_to_list(ls, func);
    tk = check();
  }
  tk = consume();
  if (!expect(tk, TK_END))
    FREE_AST_NODE_LIST(error(tk, "invalid interface %s", NULL), ls);
  return new_node(AST_INTERFACE, line, new_interface_node(name, parent, ls));
}
AstNode *parse_class()
{
  char *parent = NULL;
  Token *tk = consume();
  if (!expect(tk, TK_CLASS))
    return error(tk, "invalid class", NULL);
  int line = tk->line;
  tk = consume();
  if (!expect(tk, TK_NAME))
    return error(tk, "invalid name for class", NULL);
  char *name = tk->text;
  tk = check();
  if (expect(tk, TK_EXTENDS))
  {
    consume();
    tk = consume();
    if (!expect(tk, TK_NAME))
      return error(tk, "invalid parent for class %s", name);
    parent = tk->text;
    tk = check();
  }
  List *interfaces = new_default_list();
  if (expect(tk, TK_IMPLEMENTS))
  {
    consume();
    tk = consume();
    if (!expect(tk, TK_NAME))
      FREE_LIST(error(tk, "invalid interface for class %s", name), interfaces);
    add_to_list(interfaces, tk->text);
    tk = check();
    while (specific(tk, TK_MISC, ","))
    {
      consume();
      tk = consume();
      if (!expect(tk, TK_NAME))
        FREE_LIST(error(tk, "invalid interface for class %s", name), interfaces);
      add_to_list(interfaces, tk->text);
      tk = check();
    }
  }
  tk = consume();
  if (!expect(tk, TK_WHERE))
    FREE_LIST(error(tk, "invalid class %s", name), interfaces);
  tk = check();
  List *ls = new_default_list();
  while (tk && !expect(tk, TK_END))
  {
    AstNode *node;
    if (expect(tk, TK_CONSTRUCTOR))
      node = parse_constructor(name);
    else if (expect(tk, TK_FUNCTION))
      node = parse_function(NULL, 1);
    else
      node = parse_function_or_define();
    if (!node)
    {
      for (int a = 0; a < ls->n; a++)
        dealloc_ast_node((AstNode *)get_from_list(ls, a));
      FREE_2_LISTS(NULL, interfaces, ls);
    }
    add_to_list(ls, node);
    tk = check();
  }
  tk = consume();
  if (!expect(tk, TK_END))
  {
    for (int a = 0; a < ls->n; a++)
      dealloc_ast_node((AstNode *)get_from_list(ls, a));
    FREE_2_LISTS(error(tk, "invalid class %s", name), interfaces, ls);
  }
  return new_node(AST_CLASS, line, new_class_node(name, parent, interfaces, ls));
}

// Type parsers
AstNode *parse_typedef()
{
  Token *tk = consume();
  if (!expect(tk, TK_TYPEDEF))
    return error(tk, "invalid typedef", NULL);
  int line = tk->line;
  tk = consume();
  if (!expect(tk, TK_NAME))
    return error(tk, "invalid name for typedef", NULL);
  char *name = tk->text;
  AstNode *node = parse_type();
  if (!node)
    return NULL;
  return new_node(AST_TYPEDEF, line, new_string_ast_node(name, node));
}
static AstNode *parse_basic_type()
{
  Token *tk = check();
  if (expect(tk, TK_VAR))
  {
    consume();
    return new_node(AST_TYPE_ANY, tk->line, NULL);
  }
  else if (expect(tk, TK_DOTS))
  {
    consume();
    return new_node(AST_TYPE_VARARG, tk->line, NULL);
  }
  else if (expect(tk, TK_NAME))
  {
    consume();
    return new_node(AST_TYPE_BASIC, tk->line, tk->text);
  }
  else if (specific(tk, TK_BINARY, "*"))
  {
    int line = tk->line;
    consume();
    AstNode *node = parse_type();
    if (!node)
      return NULL;
    if (node->type == AST_TYPE_VARARG)
    {
      return error(tk, "invalid variadic member in function type", NULL);
    }
    tk = check();
    while (specific(tk, TK_PAREN, "("))
    {
      consume();
      tk = check();
      List *ls = new_default_list();
      while (tk && !specific(tk, TK_PAREN, ")"))
      {
        AstNode *arg = parse_type();
        if (!arg)
        {
          for (int a = 0; a < ls->n; a++)
            dealloc_ast_type((AstNode *)get_from_list(ls, a));
          FREE_AST_NODE_AND_LIST(error(tk, "invalid function type", NULL), node, ls);
        }
        add_to_list(ls, arg);
        tk = check();
        if (specific(tk, TK_MISC, ","))
          consume();
      }
      node = new_node(AST_TYPE_FUNC, line, new_ast_list_node(node, ls));
      tk = consume();
      if (!specific(tk, TK_PAREN, ")"))
      {
        FREE_AST_NODE(error(tk, "unclosed function type", NULL), node);
      }
      tk = check();
    }
    return node;
  }
  return error(tk, "invalid type", NULL);
}
AstNode *parse_type()
{
  Token *tk = check();
  if (specific(tk, TK_PAREN, "("))
  {
    int line = tk->line;
    int commas = 0;
    consume();
    AstNode *e = parse_basic_type();
    if (!e)
      return NULL;
    if (e->type == AST_TYPE_VARARG)
    {
      dealloc_ast_type(e);
      return error(tk, "invalid variadic member in tuple type", NULL);
    }
    List *ls = new_default_list();
    add_to_list(ls, e);
    tk = check();
    while (specific(tk, TK_MISC, ","))
    {
      consume();
      e = parse_basic_type();
      if (!e)
        FREE_AST_NODE_LIST(NULL, ls);
      if (e->type == AST_TYPE_VARARG)
      {
        FREE_AST_NODE_LIST(error(tk, "invalid variadic member in tuple type", NULL), ls);
      }
      add_to_list(ls, e);
      tk = check();
      commas++;
    }
    tk = consume();
    if (!specific(tk, TK_PAREN, ")"))
      FREE_AST_NODE_LIST(error(tk, "unclosed tuple type", NULL), ls);
    if (!commas)
      FREE_AST_NODE_LIST(error(tk, "too few elements in tuple type", NULL), ls);
    return new_node(AST_TYPE_TUPLE, line, ls);
  }
  return parse_basic_type();
}

// Variable parse functions
AstNode *parse_define(AstNode *type)
{
  AstNode *expr = NULL;
  Token *tk = consume();
  if (!expect(tk, TK_NAME))
    return error(tk, "invalid name for definition", NULL);
  int line = tk->line;
  char *name = tk->text;
  tk = check();
  if (specific(tk, TK_MISC, "="))
  {
    consume();
    expr = parse_expr();
    if (!expr)
      return NULL;
  }
  return new_node(AST_DEFINE, line, new_binary_node(name, type, expr));
}
AstNode *parse_set_or_call()
{
  AstNode *lhs = parse_potential_tuple_lhs();
  if (!lhs)
    return NULL;
  Token *tk = check();
  if (expect(tk, TK_PAREN))
  {
    if (lhs->type == AST_LTUPLE)
      FREE_AST_NODE(error(tk, "invalid function call", NULL), lhs);
    return parse_call(lhs);
  }
  tk = consume();
  if (!specific(tk, TK_MISC, "="))
    FREE_AST_NODE(error(tk, "invalid set statement", NULL), lhs);
  AstNode *expr = parse_tuple();
  if (!expr)
    FREE_AST_NODE(NULL, lhs);
  return new_node(AST_SET, lhs->line, new_ast_ast_node(lhs, expr));
}
AstNode *parse_function_or_define()
{
  AstNode *type = parse_type();
  if (!type)
    return NULL;
  Token *tk = check();
  if (!expect(tk, TK_NAME))
    FREE_AST_NODE(error(tk, "invalid statement", NULL), type);
  tk = check_ahead(2);
  if (specific(tk, TK_PAREN, "("))
    return parse_function(type, 1);
  return parse_define(type);
}
AstNode *parse_potential_tuple_lhs()
{
  AstNode *node = parse_lhs();
  Token *tk = check();
  if (specific(tk, TK_MISC, ","))
  {
    int line = tk->line;
    if (node->type != AST_ID)
      FREE_AST_NODE(error(tk, "Invalid left-hand entity in tuple", NULL), node);
    List *ls = new_default_list();
    add_to_list(ls, node);
    while (specific(tk, TK_MISC, ","))
    {
      consume();
      tk = consume();
      if (!expect(tk, TK_NAME))
        FREE_AST_NODE_LIST(error(tk, "invalid left-hand tuple", NULL), ls);
      add_to_list(ls, new_node(AST_ID, line, tk->text));
      tk = check();
    }
    return new_node(AST_LTUPLE, line, new_ast_list_node(NULL, ls));
  }
  return node;
}
AstNode *parse_lhs()
{
  Token *tk = consume();
  if (!expect(tk, TK_NAME))
    return error(tk, "invalid left-hand side of statement", NULL);
  int line = tk->line;
  AstNode *node = new_node(AST_ID, line, tk->text);
  tk = check_next();
  while (specific(tk, TK_MISC, ".") || specific(tk, TK_SQUARE, "["))
  {
    if (specific(tk, TK_SQUARE, "["))
    {
      consume();
      AstNode *r = parse_expr();
      if (!r)
        FREE_AST_NODE(NULL, node);
      node = new_node(AST_SUB, line, new_ast_ast_node(node, r));
      tk = consume();
      if (!specific(tk, TK_SQUARE, "]"))
        FREE_AST_NODE(error(tk, "invalid property", NULL), node);
    }
    if (specific(tk, TK_MISC, "."))
    {
      consume();
      tk = consume();
      if (!expect(tk, TK_NAME))
        FREE_AST_NODE(error(tk, "invalid field", NULL), node);
      node = new_node(AST_FIELD, line, new_string_ast_node(tk->text, node));
    }
    tk = check_next();
  }
  return node;
}
AstNode *parse_local()
{
  AstNode *node = NULL;
  Token *tk = consume();
  if (!expect(tk, TK_LOCAL))
    return error(tk, "invalid local variable declaration", NULL);
  int line = tk->line;
  tk = consume();
  if (!expect(tk, TK_NAME))
    return error(tk, "invalid name for local variable", NULL);
  char *name = tk->text;
  tk = check();
  if (specific(tk, TK_MISC, "="))
  {
    consume();
    node = parse_expr();
    if (!node)
      return NULL;
  }
  return new_node(AST_LOCAL, line, new_string_ast_node(name, node));
}

// Function parsers
static List *parse_function_params()
{
  Token *tk = consume();
  if (!specific(tk, TK_PAREN, "("))
    return (List *)error(tk, "invalid function", NULL);
  tk = check();
  List *args = new_default_list();
  while (tk && !specific(tk, TK_PAREN, ")"))
  {
    AstNode *arg_type = NULL;
    tk = check();
    if (expect(tk, TK_DOTS))
    {
      add_to_list(args, new_string_ast_node(tk->text, NULL));
      consume();
      break;
    }
    tk = check_ahead(2);
    if (!specific(tk, TK_MISC, ",") && !specific(tk, TK_PAREN, ")"))
    {
      arg_type = parse_type();
      if (!arg_type)
        FREE_STRING_AST_NODE_LIST(NULL, args);
    }
    tk = consume();
    if (!expect(tk, TK_NAME))
    {
      if (arg_type)
        dealloc_ast_node(arg_type);
      FREE_STRING_AST_NODE_LIST((List *)error(tk, "invalid function argument", NULL), args);
    }
    add_to_list(args, new_string_ast_node(tk->text, arg_type));
    tk = check();
    if (specific(tk, TK_MISC, ","))
    {
      consume();
      tk = check();
    }
  }
  tk = consume();
  if (!specific(tk, TK_PAREN, ")"))
  {
    FREE_STRING_AST_NODE_LIST((List *)error(tk, "unclosed function arguments", NULL), args);
  }
  return args;
}
AstNode *parse_function(AstNode *type, int include_body)
{
  int line;
  Token *tk;
  int typed = (type != NULL);
  if (!typed)
  {
    tk = consume();
    if (!expect(tk, TK_FUNCTION))
      return error(tk, "invalid function", NULL);
    type = new_node(AST_TYPE_ANY, -1, NULL);
  }
  AstNode *name = NULL;
  tk = check();
  if (expect(tk, TK_NAME))
  {
    line = tk->line;
    name = parse_lhs();
    if (!name)
    {
      if (!typed)
        free(type);
      return NULL;
    }
    if (typed && name->type != AST_ID)
    {
      if (!typed)
        free(type);
      return error(tk, "cannot define typed methods outside of a class or interface", NULL);
    }
  }
  else if (typed)
  {
    if (!expect(tk, TK_FUNCTION))
      return error(tk, "invalid anonymous typed function", NULL);
    line = tk->line;
    consume();
  }
  List *args = parse_function_params();
  if (!args)
  {
    if (!typed)
      free(type);
    if (name)
      FREE_AST_NODE(NULL, name);
    return NULL;
  }
  List *ls = NULL;
  if (include_body)
  {
    AstNode *node = parse_stmt();
    if (!node)
    {
      if (!typed)
        free(type);
      if (name)
        FREE_AST_NODE(NULL, name);
      FREE_STRING_AST_NODE_LIST(NULL, args);
    }
    tk = consume();
    if (!expect(tk, TK_END))
    {
      if (!typed)
        free(type);
      dealloc_ast_node(node);
      if (name)
        FREE_AST_NODE(NULL, name);
      FREE_STRING_AST_NODE_LIST(error(tk, "unclosed function", NULL), args);
    }
    ls = (List *)(node->data);
    free(node);
  }
  return new_node(AST_FUNCTION, line, new_function_node(name, type, args, ls));
}
AstNode *parse_constructor(char *classname)
{
  Token *tk = consume();
  if (!expect(tk, TK_CONSTRUCTOR))
    return error(tk, "invalid constructor for class %s", classname);
  int line = tk->line;
  List *args = parse_function_params();
  if (!args)
    return NULL;
  AstNode *node = parse_stmt();
  if (!node)
    FREE_AST_NODE_LIST(NULL, args);
  tk = consume();
  if (!expect(tk, TK_END))
    FREE_AST_NODE_LIST(error(tk, "unclosed constructor for class %s", classname), args);
  FunctionNode *data = new_function_node(NULL, new_node(AST_TYPE_BASIC, line, classname), args, (List *)(node->data));
  data->is_constructor = 1;
  free(node);
  return new_node(AST_FUNCTION, line, data);
}
static AstNode *parse_arg_tuple()
{
  AstNode *args = NULL;
  Token *tk = consume_next();
  if (!specific(tk, TK_PAREN, "("))
    return error(tk, "invalid function call", NULL);
  int line = tk->line;
  tk = check();
  if (tk && !specific(tk, TK_PAREN, ")"))
  {
    args = parse_tuple();
    if (!args)
      return NULL;
  }
  else
  {
    args = new_node(AST_NONE, line, NULL);
  }
  tk = consume();
  if (!specific(tk, TK_PAREN, ")"))
  {
    error(tk, "unclosed function call", NULL);
    if (args)
    {
      FREE_AST_NODE(NULL, args);
    }
    return NULL;
  }
  return args;
}
AstNode *parse_super()
{
  Token *tk = consume();
  if (!expect(tk, TK_SUPER))
    return error(tk, "invalid super method invocation", NULL);
  int line = tk->line;
  AstNode *args = parse_arg_tuple();
  if (!args)
    return NULL;
  if (args->type == AST_NONE)
  {
    free(args);
    args = NULL;
  }
  return new_node(AST_SUPER, line, args);
}
AstNode *parse_call(AstNode *lhs)
{
  AstNode *args = parse_arg_tuple();
  if (!args)
    return NULL;
  int line = args->line;
  if (args->type == AST_NONE)
  {
    free(args);
    args = NULL;
  }
  return new_node(AST_CALL, line, new_ast_ast_node(lhs, args));
}

// Conditional loop statements
AstNode *parse_repeat()
{
  Token *tk = consume();
  if (!expect(tk, TK_REPEAT))
    return error(tk, "invalid repeat statement", NULL);
  int line = tk->line;
  AstNode *body = parse_stmt();
  if (!body)
    return NULL;
  tk = consume();
  if (!expect(tk, TK_UNTIL))
    FREE_AST_NODE(error(tk, "repeat statement missing until keyword", NULL), body);
  AstNode *expr = parse_expr();
  if (!expr)
    FREE_AST_NODE(NULL, body);
  return new_node(AST_REPEAT, line, new_ast_list_node(expr, (List *)(body->data)));
}
AstNode *parse_while()
{
  Token *tk = consume();
  if (!expect(tk, TK_WHILE))
    return error(tk, "invalid while statement", NULL);
  int line = tk->line;
  AstNode *expr = parse_expr();
  if (!expr)
    return NULL;
  tk = consume();
  if (!expect(tk, TK_DO))
    FREE_AST_NODE(error(tk, "while statement missing do keyword", NULL), expr);
  AstNode *body = parse_stmt();
  if (!body)
    FREE_AST_NODE(NULL, expr);
  tk = consume();
  if (!expect(tk, TK_END))
    FREE_2_AST_NODES(error(tk, "unclosed while statement", NULL), expr, body);
  return new_node(AST_WHILE, line, new_ast_list_node(expr, (List *)(body->data)));
}

// If statements
AstNode *parse_if()
{
  Token *tk = consume();
  AstNode *next = NULL;
  if (!expect(tk, TK_IF))
    return error(tk, "invalid if statement", NULL);
  int line = tk->line;
  AstNode *expr = parse_expr();
  if (!expr)
    return NULL;
  tk = consume();
  if (!expect(tk, TK_THEN))
    FREE_AST_NODE(error(tk, "invalid expression in if statement", NULL), expr);
  AstNode *body = parse_stmt();
  if (!body)
    FREE_AST_NODE(NULL, expr);
  tk = check();
  if (expect(tk, TK_ELSEIF))
  {
    next = parse_elseif();
    if (!next)
      FREE_2_AST_NODES(NULL, expr, body);
  }
  else if (expect(tk, TK_ELSE))
  {
    next = parse_else();
    if (!next)
      FREE_2_AST_NODES(NULL, expr, body);
  }
  else if (expect(tk, TK_END))
  {
    consume();
  }
  else
  {
    FREE_2_AST_NODES(error(tk, "unclosed if statement", NULL), expr, body);
  }
  List *ls = (List *)(body->data);
  free(body);
  return new_node(AST_IF, line, new_if_node(expr, next, ls));
}
AstNode *parse_elseif()
{
  Token *tk = consume();
  AstNode *next = NULL;
  if (!expect(tk, TK_ELSEIF))
    return error(tk, "invalid elseif clause", NULL);
  int line = tk->line;
  AstNode *expr = parse_expr();
  if (!expr)
    return NULL;
  tk = consume();
  if (!expect(tk, TK_THEN))
    FREE_AST_NODE(error(tk, "invalid expression in elseif clause", NULL), expr);
  AstNode *body = parse_stmt();
  if (!body)
    FREE_AST_NODE(NULL, expr);
  tk = check();
  if (expect(tk, TK_ELSEIF))
  {
    next = parse_elseif();
    if (!next)
      FREE_2_AST_NODES(NULL, expr, body);
  }
  else if (expect(tk, TK_ELSE))
  {
    next = parse_else();
    if (!next)
      FREE_2_AST_NODES(NULL, expr, body);
  }
  else if (expect(tk, TK_END))
  {
    consume();
  }
  else
  {
    FREE_2_AST_NODES(error(tk, "unclosed elseif clause", NULL), expr, body);
  }
  List *ls = (List *)(body->data);
  free(body);
  return new_node(AST_ELSEIF, line, new_if_node(expr, next, ls));
}
AstNode *parse_else()
{
  Token *tk = consume();
  if (!expect(tk, TK_ELSE))
    return error(tk, "invalid else clause", NULL);
  int line = tk->line;
  AstNode *body = parse_stmt();
  if (!body)
    return NULL;
  tk = consume();
  if (!expect(tk, TK_END))
  {
    FREE_AST_NODE(error(tk, "unclosed else clause", NULL), body);
  }
  List *ls = (List *)(body->data);
  free(body);
  return new_node(AST_ELSE, line, ls);
}

// For statements
AstNode *parse_fornum()
{
  Token *tk = consume();
  AstNode *num1, *num2, *num3 = NULL;
  if (!expect(tk, TK_FOR))
    return error(tk, "invalid for loop", NULL);
  int line = tk->line;
  tk = consume();
  if (!expect(tk, TK_NAME))
    return error(tk, "invalid counter name in for loop", NULL);
  char *name = tk->text;
  tk = consume();
  if (!specific(tk, TK_MISC, "="))
    return error(tk, "invalid for loop with counter %s", name);
  AstNode *node = parse_tuple();
  if (!node)
    return NULL;
  AstListNode *tuple = (AstListNode *)(node->data);
  if (tuple->list->n < 2)
    FREE_AST_NODE(error(NULL, "Not enough values in for loop", NULL), node);
  if (tuple->list->n > 3)
    FREE_AST_NODE(error(NULL, "Too many values in for loop", NULL), node);
  num1 = (AstNode *)get_from_list(tuple->list, 0);
  num2 = (AstNode *)get_from_list(tuple->list, 1);
  if (tuple->list->n == 3)
    num3 = (AstNode *)get_from_list(tuple->list, 2);
  tk = consume();
  if (!expect(tk, TK_DO))
  {
    if (num3)
      dealloc_ast_node(num3);
    FREE_2_AST_NODES(error(tk, "invalid for loop with counter", name), num1, num2);
  }
  AstNode *body = parse_stmt();
  if (!body)
  {
    if (num3)
      dealloc_ast_node(num3);
    FREE_2_AST_NODES(NULL, num1, num2);
  }
  tk = consume();
  if (!expect(tk, TK_END))
  {
    dealloc_ast_node(body);
    if (num3)
      dealloc_ast_node(num3);
    FREE_2_AST_NODES(error(tk, "unclosed for loop with counter %s", name), num1, num2);
  }
  return new_node(AST_FORNUM, line, new_fornum_node(name, num1, num2, num3, (List *)(body->data)));
}
AstNode *parse_forin()
{
  Token *tk = consume();
  if (!expect(tk, TK_FOR))
    return error(tk, "invalid for loop", NULL);
  int line = tk->line;
  tk = consume();
  if (!expect(tk, TK_NAME))
    return error(tk, "invalid name in for loop", NULL);
  List *lhs = new_default_list();
  add_to_list(lhs, new_node(AST_ID, line, tk->text));
  tk = check();
  while (specific(tk, TK_MISC, ","))
  {
    consume();
    tk = consume();
    if (!expect(tk, TK_NAME))
      FREE_AST_NODE_LIST(error(tk, "invalid name in for loop", NULL), lhs);
    add_to_list(lhs, new_node(AST_ID, line, tk->text));
    tk = check();
  }
  tk = consume();
  if (!expect(tk, TK_IN))
    FREE_AST_NODE_LIST(error(tk, "missing in keyword in for loop", NULL), lhs);
  AstNode *tuple = parse_tuple();
  if (!tuple)
    FREE_AST_NODE_LIST(NULL, lhs);
  tk = consume();
  if (!expect(tk, TK_DO))
  {
    dealloc_ast_node(tuple);
    FREE_AST_NODE_LIST(error(tk, "missing do keyword in for loop", NULL), lhs);
  }
  AstNode *body = parse_stmt();
  if (!body)
  {
    dealloc_ast_node(tuple);
    FREE_AST_NODE_LIST(NULL, lhs);
  }
  tk = consume();
  if (!expect(tk, TK_END))
  {
    dealloc_ast_node(body);
    dealloc_ast_node(tuple);
    FREE_AST_NODE_LIST(error(tk, "missing end keyword in for loop", NULL), lhs);
  }
  AstNode *lhs_node = new_node(AST_LTUPLE, line, new_ast_list_node(NULL, lhs));
  return new_node(AST_FORIN, line, new_forin_node(lhs_node, tuple, (List *)(body->data)));
}

// Label-based statements
AstNode *parse_label()
{
  Token *tk = consume();
  if (!expect(tk, TK_DBCOLON))
    return error(tk, "invalid label", NULL);
  int line = tk->line;
  tk = consume();
  if (!expect(tk, TK_NAME))
    return error(tk, "invalid label", NULL);
  char *text = tk->text;
  tk = consume();
  if (!expect(tk, TK_DBCOLON))
    return error(tk, "invalid label", NULL);
  return new_node(AST_LABEL, line, text);
}
AstNode *parse_goto()
{
  Token *tk = consume();
  if (!expect(tk, TK_GOTO))
    return error(tk, "invalid goto statement", NULL);
  int line = tk->line;
  tk = consume();
  if (!expect(tk, TK_NAME))
    return error(tk, "invalid goto statement", NULL);
  char *text = tk->text;
  return new_node(AST_GOTO, line, text);
}

// Basic control statements
AstNode *parse_break()
{
  Token *tk = consume();
  if (!expect(tk, TK_BREAK))
    return error(tk, "invalid break", NULL);
  return new_node(AST_BREAK, tk->line, NULL);
}
AstNode *parse_require()
{
  Token *tk = consume();
  if (!expect(tk, TK_REQUIRE))
    return error(tk, "invalid require statement", NULL);
  AstNode *expr = parse_string();
  if (!expr)
    return NULL;
  return new_node(AST_REQUIRE, tk->line, expr);
}
AstNode *parse_return()
{
  AstNode *node = NULL;
  Token *tk = consume();
  if (!expect(tk, TK_RETURN))
    return error(tk, "invalid return statement", NULL);
  int line = tk->line;
  tk = check();
  if (!expect(tk, TK_END))
  {
    node = parse_tuple();
    if (!node)
      return NULL;
  }
  return new_node(AST_RETURN, line, node);
}

// Parse tables and lists
AstNode *parse_table_or_list()
{
  Token *tk = consume();
  if (!specific(tk, TK_CURLY, "{"))
    return error(tk, "invalid table", NULL);
  int line = tk->line;
  tk = check();
  if (specific(tk, TK_CURLY, "}"))
  {
    consume();
    return new_node(AST_LIST, line, NULL);
  }
  tk = check_ahead(2);
  if (specific(tk, TK_MISC, "="))
  {
    return parse_table();
  }
  return parse_list();
}
AstNode *parse_list()
{
  AstNode *tuple = parse_tuple();
  if (!tuple)
    return NULL;
  Token *tk = consume();
  if (!specific(tk, TK_CURLY, "}"))
  {
    if (tk)
      error(tk, "missing comma in list", NULL);
    else
      error(tk, "unclosed list", NULL);
    FREE_AST_NODE(NULL, tuple);
  }
  return new_node(AST_LIST, tk->line, tuple);
}
AstNode *parse_table()
{
  List *keys = new_default_list();
  List *vals = new_default_list();
  Token *tk = consume();
  assert(tk);
  int line = tk->line;
  while (tk && !specific(tk, TK_CURLY, "}"))
  {
    if (!expect(tk, TK_NAME))
    {
      dealloc_list(keys);
      FREE_AST_NODE_LIST(error(tk, "invalid table key", NULL), vals);
    }
    char *k = tk->text;
    add_to_list(keys, k);
    tk = consume();
    if (!specific(tk, TK_MISC, "="))
    {
      dealloc_list(keys);
      FREE_AST_NODE_LIST(error(tk, "table key %s missing equals sign", k), vals);
    }
    AstNode *node = parse_expr();
    if (!node)
    {
      dealloc_list(keys);
      FREE_AST_NODE_LIST(NULL, vals);
    }
    add_to_list(vals, node);
    tk = consume();
    if (!specific(tk, TK_CURLY, "}"))
    {
      if (!specific(tk, TK_MISC, ","))
      {
        dealloc_list(keys);
        FREE_AST_NODE_LIST(error(tk, "missing comma in table", NULL), vals);
      }
      tk = consume();
    }
  }
  if (!tk)
  {
    dealloc_list(keys);
    FREE_AST_NODE_LIST(error(tk, "unclosed table", NULL), vals);
  }
  return new_node(AST_TABLE, line, new_table_node(keys, vals));
}

// Primitive types parse functions
AstNode *parse_string()
{
  Token *tk = consume();
  if (!expect(tk, TK_QUOTE))
    return error(tk, "invalid string", NULL);
  int line = tk->line;
  List *buffer = new_default_list();
  add_to_list(buffer, tk->text);
  Token *begin = tk;
  tk = consume_next();
  while (tk && !specific(tk, TK_QUOTE, begin->text))
  {
    add_to_list(buffer, tk->text);
    tk = consume_next();
  }
  if (tk)
    add_to_list(buffer, begin->text);
  else
    FREE_LIST(error(begin, "unclosed string", NULL), buffer);
  char *string = collapse_string_list(buffer);
  dealloc_list(buffer);
  AstNode *node = new_node(AST_PRIMITIVE, line, new_primitive_node(string, PRIMITIVE_STRING));
  free(string);
  return node;
}
AstNode *parse_number()
{
  char dot[2] = {'.', 0};
  Token *tk = consume();
  if (!expect(tk, TK_INT))
    return error(tk, "invalid number", NULL);
  Token *first = tk;
  tk = check_next();
  if (specific(tk, TK_MISC, "."))
  {
    consume();
    tk = consume();
    if (!expect(tk, TK_INT))
      return error(tk, "invalid floating point primitive", NULL);
    char *text = (char *)malloc(sizeof(char) * (strlen(first->text) + strlen(tk->text) + 2));
    sprintf(text, "%s.%s", first->text, tk->text);
    return new_node(AST_PRIMITIVE, first->line, new_primitive_node(text, PRIMITIVE_FLOAT));
  }
  return new_node(AST_PRIMITIVE, first->line, new_primitive_node(first->text, PRIMITIVE_INT));
}
AstNode *parse_boolean()
{
  Token *tk = consume();
  if (!expect(tk, TK_TRUE) && !expect(tk, TK_FALSE))
    return error(tk, "invalid boolean primitive", NULL);
  return new_node(AST_PRIMITIVE, tk->line, new_primitive_node(tk->text, PRIMITIVE_BOOL));
}
AstNode *parse_nil()
{
  Token *tk = consume();
  if (!expect(tk, TK_NIL))
    return error(tk, "invalid nil", NULL);
  return new_node(AST_PRIMITIVE, tk->line, new_primitive_node("nil", PRIMITIVE_NIL));
}

// Expression parse functions
AstNode *parse_tuple()
{
  AstNode *node = parse_expr();
  if (!node)
    return NULL;
  int line = node->line;
  List *ls = new_default_list();
  add_to_list(ls, node);
  Token *tk = check();
  while (specific(tk, TK_MISC, ","))
  {
    consume();
    node = parse_expr();
    if (!node)
      FREE_AST_NODE_LIST(NULL, ls);
    add_to_list(ls, node);
    tk = check();
  }
  return new_node(AST_TUPLE, line, new_ast_list_node(NULL, ls));
}
AstNode *parse_paren_or_tuple_function()
{
  int line;
  Token *tk = check_ahead(2);
  if (specific(tk, TK_BINARY, "*"))
  {
    AstNode *type = parse_type();
    if (!type)
      return NULL;
    line = type->line;
    return parse_function(type, 1);
  }
  if (expect(tk, TK_NAME))
  {
    tk = check_ahead(3);
    if (specific(tk, TK_MISC, ","))
    {
      AstNode *type = parse_type();
      if (!type)
        return NULL;
      line = type->line;
      return parse_function(type, 1);
    }
  }
  tk = consume();
  AstNode *node = parse_expr();
  if (!node)
    return NULL;
  tk = consume();
  if (!specific(tk, TK_PAREN, ")"))
    FREE_AST_NODE(error(tk, "unclosed expression", NULL), node);
  return new_node(AST_PAREN, line, node);
}
static AstNode *precede_expr_tree(BinaryNode *data)
{
  if (data->r->type != AST_BINARY)
    return new_node(AST_BINARY, -1, data);
  BinaryNode *r = (BinaryNode *)(data->r->data);
  int lp = precedence(data->text);
  int rp = precedence(r->text);
  if (lp > rp)
  {
    AstNode *l = precede_expr_tree(new_binary_node(data->text, data->l, r->l));
    return new_node(AST_BINARY, -1, new_binary_node(r->text, l, r->r));
  }
  return new_node(AST_BINARY, -1, data);
}
AstNode *parse_expr()
{
  Token *tk = check();
  AstNode *node = NULL;
  if (!tk)
    return error(tk, "incomplete expression", NULL);
  if (tk->type == TK_NIL)
    node = parse_nil();
  else if (expect(tk, TK_TRUE) || expect(tk, TK_FALSE))
    node = parse_boolean();
  else if (specific(tk, TK_PAREN, "("))
    node = parse_paren_or_tuple_function();
  else if (expect(tk, TK_FUNCTION))
    node = parse_function(NULL, 1);
  else if (specific(tk, TK_CURLY, "{"))
    node = parse_table_or_list();
  else if (expect(tk, TK_REQUIRE))
    node = parse_require();
  else if (expect(tk, TK_QUOTE))
    node = parse_string();
  else if (expect(tk, TK_SUPER))
    node = parse_super();
  else if (expect(tk, TK_INT))
    node = parse_number();
  else if (specific(tk, TK_BINARY, "*"))
  {
    AstNode *type = parse_type();
    if (!type)
      return NULL;
    node = parse_function(type, 1);
  }
  else if (tk->type == TK_NAME)
  {
    tk = check_ahead(2);
    if (expect(tk, TK_FUNCTION))
    {
      AstNode *type = parse_type();
      if (!type)
        return NULL;
      node = parse_function(type, 1);
    }
    else
    {
      AstNode *lhs = parse_lhs();
      if (lhs)
      {
        tk = check_next();
        if (specific(tk, TK_PAREN, "("))
          node = parse_call(lhs);
        else
          node = lhs;
      }
    }
  }
  else if (expect(tk, TK_UNARY) || specific(tk, TK_MISC, "-"))
  {
    char *text = consume()->text;
    node = parse_expr();
    if (!node)
      return NULL;
    if (node->type == AST_BINARY)
    {
      BinaryNode *data = (BinaryNode *)(node->data);
      if (precedence(data->text) < UNARY_PRECEDENCE)
      {
        while (data->l->type == AST_BINARY && precedence(((BinaryNode *)(data->l->data))->text) < UNARY_PRECEDENCE)
        {
          data = (BinaryNode *)(data->l->data);
        }
        data->l = new_node(AST_UNARY, node->line, new_unary_node(text, data->l));
      }
      else
        node = new_node(AST_UNARY, node->line, new_unary_node(text, node));
    }
    else
      node = new_node(AST_UNARY, node->line, new_unary_node(text, node));
  }

  // Check for binary expressions
  tk = check();
  if (expect(tk, TK_BINARY) || specific(tk, TK_MISC, "-"))
  {
    char *op = tk->text;
    AstNode *r;
    consume();
    if (!strcmp(op, "as"))
      r = parse_type();
    else
      r = parse_expr();
    if (!r)
      FREE_AST_NODE(NULL, node);
    node = precede_expr_tree(new_binary_node(op, node, r));
  }
  if (!node)
    error(tk, "unexpected expression", NULL);
  return node;
}
