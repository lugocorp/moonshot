#include "./internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static List *scopes; // List of Scopes
static int first;    // Flag to ensure we only register primitive types once

/*
  Initialize a new scope object
*/
static Scope *new_scope(int type, void *data)
{
  Scope *scope = (Scope *)malloc(sizeof(Scope));
  scope->interfaces_registry = new_default_list();
  scope->functions_registry = new_default_list();
  scope->classes_registry = new_default_list();
  scope->types_registry = new_default_list();
  scope->defs = new_default_list();
  scope->type = type;
  scope->data = data;
  return scope;
}

/*
  Stuff to be done before we start working with scopes
*/
void preempt_scopes()
{
  scopes = NULL;
  first = 1;
}

/*
  Initializes the scope lists used by this module
*/
void init_scopes()
{
  scopes = new_default_list();
}

/*
  Deallocs all scope Lists
*/
void dealloc_scopes()
{
  dealloc_list(scopes);
}

/*
  Enters a new innermost scope
*/
void push_scope()
{
  add_to_list(scopes, new_scope(SCOPE_NONE, NULL));
  if (scopes->n == 1)
  {
    assert(first); // This can only happen once
    register_primitive(PRIMITIVE_STRING);
    register_primitive(PRIMITIVE_FLOAT);
    register_primitive(PRIMITIVE_BOOL);
    register_primitive(PRIMITIVE_INT);
    register_primitive(PRIMITIVE_NIL);
    first = 0;
  }
}

/*
  Exits the current innermost scope
*/
void pop_scope()
{
  Scope *scope = remove_from_list(scopes, scopes->n - 1);
  for (int a = 0; a < scope->defs->n; a++)
  {
    StringAstNode *var = (StringAstNode *)get_from_list(scope->defs, a);
    if (scope->type == SCOPE_CLASS && !strcmp(var->text, "this") && var->node->type == AST_TYPE_BASIC)
    {
      ClassNode *class = (ClassNode *)(scope->data);
      char *type = (char *)(var->node->data);
      if (!strcmp(class->name, type))
      {
        free(var->text);
        free(var->node);
      }
    }
    free(var);
  }
  dealloc_list(scope->defs);
  for (int a = 0; a < scope->types_registry->n; a++)
  {
    char *type = (char *)get_from_list(scope->types_registry, a);
    if (!strcmp(type, PRIMITIVE_STRING) || !strcmp(type, PRIMITIVE_FLOAT) || !strcmp(type, PRIMITIVE_BOOL) || !strcmp(type, PRIMITIVE_INT) || !strcmp(type, PRIMITIVE_NIL))
    {
      free(type);
    }
  }
  dealloc_list(scope->interfaces_registry);
  dealloc_list(scope->functions_registry);
  dealloc_list(scope->classes_registry);
  dealloc_list(scope->types_registry);
  free(scope);
}

/*
  Enters a new inner function scope
*/
void push_function_scope(FunctionNode *node)
{
  add_to_list(scopes, new_scope(SCOPE_FUNCTION, node));
  for (int a = 0; a < node->args->n; a++)
  {
    StringAstNode *arg = (StringAstNode *)get_from_list(node->args, a);
    StringAstNode *var = new_string_ast_node(arg->text, arg->node);
    if (!add_scoped_var(var))
    {
      // This should never ever happen
      assert(0);
      free(var);
    }
  }
}

/*
  Returns the innermost function scope
  Returns NULL if the current scope is not within any function
*/
FunctionNode *get_function_scope()
{
  for (int a = scopes->n - 1; a >= 0; a--)
  {
    Scope *scope = (Scope *)get_from_list(scopes, a);
    if (scope->type == SCOPE_FUNCTION)
    {
      return (FunctionNode *)(scope->data);
    }
  }
  return NULL;
}

/*
  Returns the innermost class's method scope
  Returns NULL if the current scope is not within any class's method
*/
FunctionNode *get_method_scope()
{
  for (int a = scopes->n - 1; a >= 0; a--)
  {
    Scope *scope = (Scope *)get_from_list(scopes, a);
    if (scope->type == SCOPE_FUNCTION && a)
    {
      FunctionNode *func = (FunctionNode *)(scope->data);
      scope = (Scope *)get_from_list(scopes, a - 1);
      if (scope->type == SCOPE_CLASS)
      {
        return func;
      }
    }
  }
  return NULL;
}

/*
  Enters a new inner class scope
*/
void push_class_scope(ClassNode *node)
{
  add_to_list(scopes, new_scope(SCOPE_CLASS, node));
  char *this = (char *)malloc(sizeof(char) * 5);
  sprintf(this, "this");
  AstNode *type = new_node(AST_TYPE_BASIC, -1, node->name);
  StringAstNode *var = new_string_ast_node(this, type);
  if (!add_scoped_var(var))
  {
    // This should never ever happen
    assert(0);
    free(type);
    free(this);
    free(var);
  }
}

/*
  Returns the innermost class scope
  Returns NULL if the current scope is not within any class
*/
ClassNode *get_class_scope()
{
  for (int a = scopes->n - 1; a >= 0; a--)
  {
    Scope *scope = (Scope *)get_from_list(scopes, a);
    if (scope->type == SCOPE_CLASS)
    {
      return (ClassNode *)(scope->data);
    }
  }
  return NULL;
}

/*
  Adds a new typed variable to the current scope
  node must be allocated specifically for this function
  node will be freed automatically in pop_scope
*/
int add_scoped_var(StringAstNode *node)
{
  Scope *scope = (Scope *)get_from_list(scopes, scopes->n - 1);
  for (int a = 0; a < scope->defs->n; a++)
  {
    if (!strcmp(((StringAstNode *)get_from_list(scope->defs, a))->text, node->text))
    {
      return 0;
    }
  }
  add_to_list(scope->defs, node);
  return 1;
}

/*
  Returns the BinaryNode representing the typed variable called name
  Return NULL if no such typed variable exists
  Searches through every scope from innermost to outermost
*/
StringAstNode *get_scoped_var(char *name)
{
  for (int a = (scopes->n) - 1; a >= 0; a--)
  {
    Scope *scope = (Scope *)get_from_list(scopes, a);
    for (int b = 0; b < scope->defs->n; b++)
    {
      StringAstNode *n = (StringAstNode *)get_from_list(scope->defs, b);
      if (!strcmp(n->text, name))
      {
        return n;
      }
    }
  }
  return NULL;
}

/*
  Returns 1 if the given field is defined as a class field
*/
int field_defined_in_class(char *name)
{
  StringAstNode *node = get_scoped_var(name);
  for (int a = scopes->n - 1; a >= 0; a--)
  {
    Scope *scope = (Scope *)get_from_list(scopes, a);
    for (int b = 0; b < scope->defs->n; b++)
    {
      StringAstNode *n = (StringAstNode *)get_from_list(scope->defs, b);
      if (!strcmp(n->text, name))
      {
        return scope->type == SCOPE_CLASS;
      }
    }
  }
  return 0;
}

/*
  Returns the number of scopes
*/
int get_num_scopes()
{
  return scopes->n;
}

/*
  Returns the innermost scope
*/
Scope *get_scope()
{
  return (Scope *)get_from_list(scopes, scopes->n - 1);
}

/*
  Registers a new primitive type (a typedef, class or interface)
*/
void register_primitive(const char *name)
{
  Scope *scope = get_scope();
  char *type = (char *)malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(type, name);
  add_to_list(scope->types_registry, type);
}

/*
  Registers a type
*/
void register_type(char *name)
{
  Scope *scope = get_scope();
  add_to_list(scope->types_registry, name);
}

/*
  Registers a function
*/
void register_function(FunctionNode *node)
{
  Scope *scope = get_scope();
  add_to_list(scope->functions_registry, node);
}

/*
  Registers an interface
*/
void register_interface(InterfaceNode *node)
{
  Scope *scope = get_scope();
  add_to_list(scope->interfaces_registry, node);
}

/*
  Registers a class
*/
void register_class(ClassNode *node)
{
  Scope *scope = get_scope();
  add_to_list(scope->classes_registry, node);
}

/*
  Returns 1 if type name is registered
*/
int type_exists(char *name)
{
  for (int a = scopes->n - 1; a >= 0; a--)
  {
    Scope *scope = (Scope *)get_from_list(scopes, a);
    List *ls = scope->types_registry;
    for (int b = 0; b < ls->n; b++)
    {
      if (!strcmp((char *)get_from_list(ls, b), name))
        return 1;
    }
  }
  return 0;
}

/*
  Returns the registered FunctionNode if name is a registered function
  Returns NULL if function name does not exist
*/
FunctionNode *function_exists(char *name)
{
  if (!name)
    return NULL;
  for (int a = scopes->n - 1; a >= 0; a--)
  {
    Scope *scope = (Scope *)get_from_list(scopes, a);
    List *ls = scope->functions_registry;
    for (int b = 0; b < ls->n; b++)
    {
      FunctionNode *node = (FunctionNode *)get_from_list(ls, b);
      assert(node->name->type == AST_ID); // I'm assuming func->name is of type AST_ID
      char *funcname = (char *)(node->name->data);
      if (node->name && !strcmp(name, funcname))
      {
        return node;
      }
    }
  }
  return NULL;
}

/*
  Returns the registered InterfaceNode if name is a registered interface
  Returns NULL if interface name does not exist
*/
InterfaceNode *interface_exists(char *name)
{
  if (!name)
    return NULL;
  name = base_type(name);
  for (int a = scopes->n - 1; a >= 0; a--)
  {
    Scope *scope = (Scope *)get_from_list(scopes, a);
    List *ls = scope->interfaces_registry;
    for (int b = 0; b < ls->n; b++)
    {
      InterfaceNode *node = (InterfaceNode *)get_from_list(ls, b);
      if (!strcmp(name, node->name))
      {
        return node;
      }
    }
  }
  return NULL;
}

/*
  Returns the registered ClassNode if name is a registered class
  Returns NULL if class name does not exist
*/
ClassNode *class_exists(char *name)
{
  if (!name)
    return NULL;
  name = base_type(name);
  for (int a = scopes->n - 1; a >= 0; a--)
  {
    Scope *scope = (Scope *)get_from_list(scopes, a);
    List *ls = scope->classes_registry;
    for (int b = 0; b < ls->n; b++)
    {
      ClassNode *node = (ClassNode *)get_from_list(ls, b);
      if (!strcmp(name, node->name))
      {
        return node;
      }
    }
  }
  return NULL;
}
