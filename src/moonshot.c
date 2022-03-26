#include "./moonshot.h"
#include "./internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#define ERROR_BUFFER_LENGTH 256 // Maximum length for an error message
static int line_written;        // Zero if there's no content on the current output line yet
static List *srcs;              // Stack of files you're parsing/traversing
static List *requires;          // List of required files
static List *errors;            // List of error strings
static int error_i;             // Index of currently consumed error
static FILE *_input;            // Input for source code

/*
  Return the number of compilation errors
*/
int moonshot_num_errors()
{
  return errors->n;
}

/*
  Returns the next error message from compilation
  Returns NULL if there's no more errors
*/
char *moonshot_next_error()
{
  if (errors && error_i < errors->n)
    return (char *)get_from_list(errors, error_i++);
  return NULL;
}

/*
  Custom string format function
*/
char *format_string(int indent, const char *msg, va_list args)
{
  List *ls = new_default_list();
  char symbol[2] = {0, 0};
  int n = strlen(msg);
  for (int a = 0; a < n; a++)
  {
    if (indent)
    {
      if (msg[a] == '\n')
      {
        line_written = 0;
      }
      else if (!line_written)
      {
        line_written = 1;
        for (int a = 0; a < get_num_indents(); a++)
        {
          char *tab = (char *)malloc(sizeof(char) * 3);
          sprintf(tab, "\t");
          add_to_list(ls, tab);
        }
      }
    }
    if (a < n - 1 && msg[a] == '%')
    {
      if (msg[a + 1] == 's')
      {
        char *str = va_arg(args, char *);
        add_to_list(ls, copy_string(str));
        a++;
        continue;
      }
      if (msg[a + 1] == 't')
      {
        AstNode *type = (AstNode *)va_arg(args, AstNode *);
        char *str = stringify_type(type);
        add_to_list(ls, str);
        a++;
        continue;
      }
      if (msg[a + 1] == 'i')
      {
        int v = va_arg(args, int);
        char *str = string_from_int(v);
        add_to_list(ls, str);
        a++;
        continue;
      }
    }
    symbol[0] = msg[a];
    add_to_list(ls, copy_string(symbol));
  }
  char *str = collapse_string_list(ls);
  for (int a = 0; a < ls->n; a++)
    free(get_from_list(ls, a));
  free(ls);
  return str;
}

/*
  Adds an error into the compiler error stream
  Must include at least one parameter after msg or you'll get an error
  Exits with a special error if your msg and va_args cause the error buffer to overflow
*/
void add_error(int line, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  add_error_internal(line, msg, args);
  va_end(args);
}
void add_error_internal(int line, const char *msg, va_list args)
{
  List *ls = new_default_list();
  add_to_list(ls, format_string(0, msg, args));
  if (srcs->n)
  {
    char *file = (char *)get_from_list(srcs, srcs->n - 1);
    char *suffix = (char *)malloc(sizeof(char) * (strlen(file) + 5));
    sprintf(suffix, " in %s", file);
    add_to_list(ls, suffix);
  }
  if (line >= 0)
  {
    char *str = string_from_int(line);
    char *suffix = (char *)malloc(sizeof(char) * (strlen(str) + 9));
    sprintf(suffix, " (line %i)", line);
    add_to_list(ls, suffix);
  }
  char *err = collapse_string_list(ls);
  for (int a = 0; a < ls->n; a++)
    free(get_from_list(ls, a));
  free(ls);
  add_to_list(errors, err);
}

/*
  Deallocates all tokens within the token list and the list itself
*/
void dealloc_token_buffer(List *ls)
{
  for (int a = 0; a < ls->n; a++)
    dealloc_token((Token *)get_from_list(ls, a));
  dealloc_list(ls);
}

/*
  Deallocates all error strings in the errors list and the list itself
*/
static void dealloc_errors()
{
  for (int a = 0; a < errors->n; a++)
    free(get_from_list(errors, a));
  dealloc_list(errors);
  errors = NULL;
}

/*
  Takes a List of strings and collapses it into one single string
*/
char *collapse_string_list(List *ls)
{
  int l = 0;
  for (int a = 0; a < ls->n; a++)
    l += strlen((char *)get_from_list(ls, a));
  char *copy = (char *)malloc(sizeof(char) * (l + 1));
  copy[0] = 0;
  for (int a = 0; a < ls->n; a++)
    strcat(copy, (char *)get_from_list(ls, a));
  return copy;
}

/*
  Copies a string but without the first and last characters (quotes)
*/
char *strip_quotes(char *str)
{
  int l = strlen(str);
  char *copy = (char *)malloc(sizeof(char) * (l - 1));
  strncpy(copy, str + 1, l - 2);
  copy[l - 2] = 0;
  return copy;
}

/*
  Copies a string
*/
char *copy_string(char *str)
{
  int l = strlen(str);
  char *copy = (char *)malloc(sizeof(char) * (l + 1));
  strcpy(copy, str);
  return copy;
}

/*
  Get string from int
*/
char *string_from_int(int a)
{
  int n = (a / 10) + 1;
  if (n < 0)
    n++;
  char *msg = (char *)malloc(sizeof(char) * (n + 1));
  sprintf(msg, "%i", a);
  return msg;
}

/*
  Initializes this module
*/
void init_requires()
{
  requires = new_default_list();
  srcs = new_default_list();
}

/*
  Deallocates a list of required files
*/
static void dealloc_requires()
{
  for (int a = requires->n - 1; a >= 0; a--)
  {
    Require *r = (Require *)get_from_list(requires, a);
    if (r->tree)
      dealloc_ast_node(r->tree);
    if (r->tokens)
      dealloc_token_buffer(r->tokens);
    free(r->filename);
    free(r);
  }
  dealloc_list(requires);
  dealloc_list(srcs);
  requires = NULL;
  srcs = NULL;
}

/*
  Manually adds a dummy node for the given filename
*/
void dummy_required_file(char *filename)
{
  char *copy = copy_string(filename);
  if (srcs->n)
  {
    remove_from_list(srcs, srcs->n - 1);
  }
  Require *r = (Require *)malloc(sizeof(Require));
  r->completed = STEP_OUTPUT;
  r->filename = copy;
  r->tokens = NULL;
  r->tree = NULL;
  add_to_list(requires, r);
  add_to_list(srcs, copy);
}

/*
  Tokenizes, parses and traverses another file to import external Moon types
  Will only bother if the filename ends in .moon (is Moonshot source code)
  Also checks to ensure that we're not processing a file we've already processed
  Returns 1 if the required file is a Moonshot source file
*/
int require_file(char *filename, int step)
{
  char *copy = strip_quotes(filename);
  int l = strlen(copy);
  if (l < 5 || strcmp(copy + l - 5, ".moon"))
  {
    free(copy);
    return 0;
  }
  for (int a = 0; a < requires->n; a++)
  {
    Require *r = (Require *)get_from_list(requires, a);
    if (!strcmp(r->filename, copy))
    {
      if (r->completed < step)
      {
        if (step < STEP_OUTPUT)
          r->completed = step;
      }
      else
      {
        free(copy);
        return 1;
      }
    }
  }
  add_to_list(srcs, copy);
  if (step == STEP_TYPEDEF)
  {
    FILE *f = fopen(copy, "r");
    if (!f)
    {
      add_error(-1, "cannot open file %s", copy);
      remove_from_list(srcs, srcs->n - 1);
      free(copy);
      return 1;
    }
    List *ls = tokenize(f);
    fclose(f);
    if (!ls)
    {
      add_error(-1, "tokenization buffer overflow", NULL);
      remove_from_list(srcs, srcs->n - 1);
      free(copy);
      return 1;
    }
    AstNode *root = parse(ls);
    if (!root)
    {
      remove_from_list(srcs, srcs->n - 1);
      dealloc_token_buffer(ls);
      free(copy);
      return 1;
    }
    Require *r = (Require *)malloc(sizeof(Require));
    r->filename = copy;
    r->completed = 0;
    r->tokens = ls;
    r->tree = root;
    add_to_list(requires, r);
  }
  for (int a = 0; a < requires->n; a++)
  {
    Require *r = (Require *)get_from_list(requires, a);
    if (!strcmp(r->filename, copy) && (step != STEP_OUTPUT || r->completed < STEP_OUTPUT))
    {
      if (step == STEP_OUTPUT)
        r->completed = STEP_OUTPUT;
      if (r->tree)
      {
        assert(r->tree->type == AST_STMT);
        List *ls = (List *)(r->tree->data);
        for (int b = 0; b < ls->n; b++)
        {
          process_node((AstNode *)get_from_list(ls, b));
        }
      }
      break;
    }
  }
  if (step != STEP_TYPEDEF)
    free(copy);
  remove_from_list(srcs, srcs->n - 1);
  return 1;
}

/*
  Initializes data used by the Moonshot library
*/
void moonshot_init()
{
  line_written = 0;
  requires = NULL;
  errors = NULL;
  _input = NULL;
  srcs = NULL;
  error_i = 0;
}

/*
  Deallocate remaining memory from Moonshot's compilation process
*/
void moonshot_destroy()
{
  if (errors)
    dealloc_errors();
}

/*
  Sets configuration for compilation
  Sets source code and output I/O
  Also controls whether or not to write any output
*/
void moonshot_configure(FILE *input, FILE *output)
{
  set_output(output);
  _input = input;
}

/*
  Read from your configured input and compile Moonshot code
  Will only write Lua code to output if it's set in the configuration
*/
int moonshot_compile()
{
  if (!requires)
    init_requires();
  if (errors)
    dealloc_errors();
  errors = new_default_list();

  // Tokenize
  List *ls = tokenize(_input);
  if (!ls)
  {
    add_error(-1, "tokenization buffer overflow", NULL);
    return 0;
  }

  // Parse tokens
  AstNode *root = parse(ls);
  if (!root)
  {
    dealloc_token_buffer(ls);
    return 0;
  }

  // AST traversal
  init_traverse();
  traverse(root, STEP_CHECK);
  if (!errors->n)
    traverse(root, STEP_OUTPUT);
  dealloc_traverse();
  dealloc_requires();
  dealloc_ast_node(root);
  dealloc_token_buffer(ls);
  return (errors->n) ? 0 : 1;
}
