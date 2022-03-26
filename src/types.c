#include "./internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static List *types_graph; // List of EqualTypesNodes

/*
  Initialize the data structures used in this module
*/
void init_types()
{
  types_graph = new_default_list();
}

/*
  Deallocate registers and equivalence graphs
*/
void dealloc_types()
{
  // Quelling scoped type equivalences means there shouldn't be
  // any equivalences left by the time we get here
  assert(types_graph->n == 0);
  dealloc_list(types_graph);
}

/*
  Deep copies a type to prevent deallocation errors
*/
static AstNode *copy_type(AstNode *node)
{
  switch (node->type)
  {
  case AST_TYPE_VARARG:
    return new_node(AST_TYPE_VARARG, -1, NULL);
  case AST_TYPE_BASIC:
    return new_node(AST_TYPE_BASIC, -1, node->data);
  case AST_TYPE_TUPLE:
  {
    List *data = (List *)(node->data);
    List *ls = new_default_list();
    for (int a = 0; a < data->n; a++)
    {
      AstNode *copy = copy_type((AstNode *)get_from_list(data, a));
      add_to_list(ls, copy);
    }
    return new_node(AST_TYPE_TUPLE, -1, ls);
  }
  case AST_TYPE_FUNC:
  {
    AstListNode *data = (AstListNode *)(node->data);
    List *ls = new_default_list();
    for (int a = 0; a < data->list->n; a++)
    {
      AstNode *e = copy_type((AstNode *)get_from_list(data->list, a));
      add_to_list(ls, e);
    }
    return new_node(AST_TYPE_FUNC, -1, new_ast_list_node(copy_type(data->node), ls));
  }
  }
  return new_node(AST_TYPE_ANY, -1, NULL);
}

/*
  Returns 1 if the AST_TYPE_* AstNode node is a AST_TYPE_BASIC node with value type
*/
int is_primitive(AstNode *node, const char *type)
{
  return node->type == AST_TYPE_BASIC && !strcmp((char *)(node->data), type);
}

/*
  Returns the type of some entity's field name
  node is either a ClassNode or InterfaceNode, as specified by is_interface
*/
static AstNode *get_type_of_field(char *name, void *data, int is_interface)
{
  AstNode *node = new_node(is_interface ? AST_INTERFACE : AST_CLASS, -1, data);
  List *body = get_all_expected_fields(node);
  free(node);
  for (int a = 0; a < body->n; a++)
  {
    AstNode *e = (AstNode *)get_from_list(body, a);
    if (e->type == AST_FUNCTION)
    {
      FunctionNode *func = (FunctionNode *)(e->data);
      if (func->name)
      {
        char *funcname = (char *)(func->name->data);
        if (!strcmp(funcname, name))
          return get_type(e);
      }
    }
    else
    {
      BinaryNode *def = (BinaryNode *)(e->data);
      if (!strcmp(def->text, name))
        return def->l;
    }
  }
  return NULL;
}

// Functions for getting type nodes from various AstNodes
static AstNode *get_ltuple_type(AstListNode *data)
{
  if (!data->node)
  {
    List *ls = data->list;
    List *types = new_default_list();
    for (int a = 0; a < ls->n; a++)
    {
      AstNode *e = (AstNode *)get_from_list(ls, a);
      add_to_list(types, copy_type(get_type(e)));
    }
    data->node = new_node(AST_TYPE_TUPLE, -1, types);
  }
  return data->node;
}
static AstNode *get_tuple_type(AstListNode *data)
{
  if (!data->node)
  {
    List *ls = data->list;
    List *types = new_default_list();
    for (int a = 0; a < ls->n; a++)
    {
      AstNode *e = (AstNode *)get_from_list(ls, a);
      add_to_list(types, copy_type(get_type(e)));
    }
    data->node = new_node(AST_TYPE_TUPLE, -1, types);
  }
  return data->node;
}
static AstNode *get_function_type(FunctionNode *data)
{
  if (!data->functype)
  {
    List *ls = new_default_list();
    for (int a = 0; a < data->args->n; a++)
    {
      AstNode *e;
      StringAstNode *arg = (StringAstNode *)get_from_list(data->args, a);
      if (arg->node)
        e = copy_type(arg->node);
      else if (!strcmp(arg->text, "..."))
        e = new_node(AST_TYPE_VARARG, -1, NULL);
      else
        e = new_node(AST_TYPE_ANY, -1, NULL);
      add_to_list(ls, e);
    }
    data->functype = new_node(AST_TYPE_FUNC, -1, new_ast_list_node(copy_type(data->type), ls));
  }
  return data->functype;
}
static AstNode *get_super_type()
{
  FunctionNode *method = get_method_scope();
  if (!method)
    return any_type_const();
  AstNode *node = new_node(AST_FUNCTION, -1, method);
  AstListNode *functype = (AstListNode *)(get_type(node)->data);
  free(node);
  return functype->node;
}
static AstNode *get_call_type(AstAstNode *data)
{
  AstNode *l = data->l;
  if (l->type == AST_ID)
  {
    char *name = (char *)(l->data);
    FunctionNode *func = function_exists(name);
    if (func)
      return func->type;
    ClassNode *cnode = class_exists(name);
    if (cnode)
      return cnode->type;
    return any_type_const();
  }
  AstNode *type = get_type(l);
  if (type->type == AST_TYPE_ANY)
    return any_type_const();
  return ((AstListNode *)(type->data))->node;
}
static AstNode *get_id_type(AstNode *node)
{
  char *name = (char *)(node->data);
  StringAstNode *var = get_scoped_var(name);
  return (var && var->node) ? (var->node) : any_type_const();
}
static AstNode *get_field_type(StringAstNode *data)
{
  AstNode *ltype = get_type(data->node);
  if (ltype->type == AST_TYPE_BASIC)
  {
    char *name = (char *)(ltype->data);
    InterfaceNode *inode = interface_exists(name);
    if (inode)
    {
      AstNode *type = get_type_of_field(data->text, inode, 1);
      if (type)
        return type;
      add_error(-1, "interface %s has no such field %s", inode->name, data->text);
      return any_type_const();
    }
    ClassNode *cnode = class_exists(name);
    if (cnode)
    {
      AstNode *type = get_type_of_field(data->text, cnode, 0);
      if (type)
        return type;
      add_error(-1, "class %s has no such field %s", cnode->name, data->text);
      return any_type_const();
    }
  }
  return any_type_const();
}
static AstNode *get_binary_type(BinaryNode *data)
{
  if (!strcmp(data->text, "as"))
  {
    return data->r;
  }
  AstNode *tl = get_type(data->l);
  AstNode *tr = get_type(data->r);
  if (!strcmp(data->text, ".."))
  {
    if (is_primitive(tl, PRIMITIVE_STRING) && is_primitive(tr, PRIMITIVE_STRING))
      return tl;
    return any_type_const();
  }
  if (!strcmp(data->text, "/"))
  {
    return float_type_const();
  }
  if (!strcmp(data->text, "+") || !strcmp(data->text, "-") || !strcmp(data->text, "*"))
  {
    if (is_primitive(tl, PRIMITIVE_FLOAT))
      return tl;
    if (is_primitive(tr, PRIMITIVE_FLOAT))
      return tr;
    return int_type_const();
  }
  return bool_type_const();
}

/*
  Finds an AST_TYPE_* AstNode for the input AstNode
  Never free the result of this function, it will be deallocated elsewhere
*/
AstNode *get_type(AstNode *node)
{
  int type = node->type;
  switch (node->type)
  {
  case AST_FUNCTION:
    return get_function_type((FunctionNode *)(node->data));
  case AST_FIELD:
    return get_field_type((StringAstNode *)(node->data));
  case AST_LTUPLE:
    return get_ltuple_type((AstListNode *)(node->data));
  case AST_BINARY:
    return get_binary_type((BinaryNode *)(node->data));
  case AST_TUPLE:
    return get_tuple_type((AstListNode *)(node->data));
  case AST_PRIMITIVE:
    return ((StringAstNode *)(node->data))->node;
  case AST_CALL:
    return get_call_type((AstAstNode *)(node->data));
  case AST_PAREN:
    return get_type((AstNode *)(node->data));
  case AST_DEFINE:
    return ((BinaryNode *)(node->data))->l;
  case AST_UNARY:
    return ((BinaryNode *)(node->data))->r;
  case AST_REQUIRE:
    return any_type_const();
  case AST_SUPER:
    return get_super_type();
  case AST_SUB:
    return any_type_const();
  case AST_ID:
    return get_id_type(node);
  default:
    return any_type_const();
  }
}

/*
  Returns 1 if the FunctionNode has a variadic parameter
  args is a list of AST_TYPE_* nodes
*/
int is_variadic_function(List *args)
{
  if (!args->n)
    return 0;
  AstNode *arg = (AstNode *)get_from_list(args, args->n - 1);
  return arg->type == AST_TYPE_VARARG;
}

/*
  Returns 1 if the type r can be placed into a variable of type l
*/
static int typed_match_no_equivalence(AstNode *l, AstNode *r)
{
  if (l->type == AST_TYPE_ANY)
    return 1;
  if (!r)
    return 0;
  if (is_primitive(r, PRIMITIVE_NIL))
    return 1;
  if (is_primitive(l, PRIMITIVE_FLOAT) && is_primitive(r, PRIMITIVE_INT))
    return 1;
  if (l->type == AST_TYPE_BASIC && r->type == AST_TYPE_BASIC)
  {
    return !strcmp((char *)(l->data), (char *)(r->data));
  }
  if (l->type == AST_TYPE_TUPLE && r->type == AST_TYPE_TUPLE)
  {
    List *lls = (List *)(l->data);
    List *rls = (List *)(r->data);
    if (lls->n != rls->n)
      return 0;
    for (int a = 0; a < lls->n; a++)
    {
      AstNode *nl = (AstNode *)get_from_list(lls, a);
      AstNode *nr = (AstNode *)get_from_list(rls, a);
      int match = typed_match(nl, nr);
      if (!match)
        return 0;
    }
    return 1;
  }
  if (l->type == AST_TYPE_FUNC && r->type == AST_TYPE_FUNC)
  {
    AstListNode *ldata = (AstListNode *)(l->data);
    AstListNode *rdata = (AstListNode *)(r->data);
    if (!typed_match(ldata->node, rdata->node))
      return 0;
    int max = ldata->list->n;
    if (is_variadic_function(ldata->list))
    {
      if (rdata->list->n < ldata->list->n - 1)
        return 0;
      max = ldata->list->n - 1;
    }
    else
    {
      if (rdata->list->n != ldata->list->n)
        return 0;
    }
    for (int a = 0; a < max; a++)
    {
      if (!typed_match((AstNode *)get_from_list(ldata->list, a), (AstNode *)get_from_list(rdata->list, a)))
        return 0;
    }
    return 1;
  }
  return 0;
}

/*
  Returns 1 if the types r and l are an exact match
  Or returns 1 if the type r can be placed into a variable of type l
*/
int typed_match(AstNode *l, AstNode *r)
{
  if (!l)
    return 1;
  if (r && l->type == AST_TYPE_BASIC && types_equivalent((char *)(l->data), r))
    return 1;
  return typed_match_no_equivalence(l, r);
}

/*
  Boils a typedef type down into its lowest-level typedef
  Returns the input type if it is already at its lowest typedef link
*/
char *base_type(char *name)
{
  while (1)
  {
    for (int a = 0; a < types_graph->n; a++)
    {
      EqualTypesNode *node = (EqualTypesNode *)get_from_list(types_graph, a);
      if (node->relation == RL_EQUALS && node->type->type == AST_TYPE_BASIC && !strcmp(node->name, name))
      {
        name = (char *)(node->type->data);
        continue;
      }
    }
    break;
  }
  return name;
}

/*
  Goes recursively through a compound type (tuple or function) or singular type
  Returns 1 if every type referenced in the compound type exists
*/
int compound_type_exists(AstNode *node)
{
  if (!node)
    return 1;
  switch (node->type)
  {
  case AST_TYPE_ANY:
    return 1;
  case AST_TYPE_VARARG:
    return 1;
  case AST_TYPE_BASIC:
  {
    char *t = (char *)(node->data);
    if (!type_exists(t))
      return 0;
    return 1;
  }
  case AST_TYPE_TUPLE:
  {
    List *ls = (List *)(node->data);
    for (int a = 0; a < ls->n; a++)
    {
      if (!compound_type_exists((AstNode *)get_from_list(ls, a)))
        return 0;
    }
    return 1;
  }
  case AST_TYPE_FUNC:
  {
    AstListNode *data = (AstListNode *)(node->data);
    if (!compound_type_exists(data->node))
      return 0;
    for (int a = 0; a < data->list->n; a++)
    {
      if (!compound_type_exists((AstNode *)get_from_list(data->list, a)))
        return 0;
    }
    return 1;
  }
  }
  assert(0); // You should never get here
}

/*
  Checks for a path from name to type in the equivalent types graph
  Returns 1 if such a path exists
*/
static int path_exists(char *name, AstNode *type)
{
  List *ls = get_equivalent_types(name);
  int a = 0;
  while (a < ls->n)
  {
    AstNode *node = get_from_list(ls, a);
    if (typed_match_no_equivalence(node, type))
    {
      dealloc_list(ls);
      return 1;
    }
    if (node->type == AST_TYPE_BASIC)
    {
      List *ls1 = get_equivalent_types((char *)(node->data));
      for (int b = 0; b < ls1->n; b++)
        add_to_list(ls, get_from_list(ls1, b));
      dealloc_list(ls1);
    }
    a++;
  }
  dealloc_list(ls);
  return 0;
}

/*
  Creates a type equivalence for entity types (classes and interfaces)
  Just a wrapper around add_type_equivalence
*/
int add_child_type(char *child, char *parent, int relation)
{
  AstNode *r = new_node(AST_TYPE_BASIC, -1, child);
  return add_type_equivalence(parent, r, relation);
}

/*
  Registers a type relationship as a path in the equivalent types graph
  name points to type in the graph
  Returns 0 if this will create a cycle
*/
int add_type_equivalence(char *name, AstNode *type, int relation)
{
  if (type->type == AST_TYPE_BASIC)
  {
    AstNode *r = new_node(AST_TYPE_BASIC, -1, name);
    char *l = (char *)(type->data);
    int cycle = path_exists(l, r);
    free(r);
    if (cycle)
      return 0;
  }
  assert(get_num_scopes() > 0); // Ensure that there is a scope
  add_to_list(types_graph, new_equal_types_node(name, type, relation, get_num_scopes()));
  return 1;
}

/*
  Returns a List of AST_TYPE_* AstNodes
  List is full of types that are equivalent to name with 1 degree of separation
*/
List *get_equivalent_types(char *name)
{
  List *ls = new_default_list();
  for (int a = 0; a < types_graph->n; a++)
  {
    EqualTypesNode *node = (EqualTypesNode *)get_from_list(types_graph, a);
    if (!strcmp(node->name, name))
      add_to_list(ls, node->type);
  }
  return ls;
}

/*
  Returns 1 if two types are equivalent (or if type is a subtype of name)
*/
int types_equivalent(char *name, AstNode *type)
{
  if (type->type == AST_TYPE_BASIC && !strcmp(name, (char *)(type->data)))
    return 1;
  return path_exists(name, type);
}

/*
  Cleans expired edges from the types equivalency graph
  Expired means we have exited the scope that an equivalence was defined in
*/
void quell_expired_scope_equivalences(int scope)
{
  int a = 0;
  while (a < types_graph->n)
  {
    EqualTypesNode *node = (EqualTypesNode *)get_from_list(types_graph, a);
    assert(node->scope <= scope);
    if (node->scope == scope)
    {
      remove_from_list(types_graph, a);
    }
    else
    {
      a++;
    }
  }
}

/*
  Internal recursive function that helps with type stringification
*/
static void stringify_type_internal(List *ls, AstNode *node)
{
  if (!node || node->type == AST_TYPE_ANY)
  {
    add_to_list(ls, "var");
  }
  else if (node->type == AST_TYPE_VARARG)
  {
    add_to_list(ls, "...");
  }
  else if (node->type == AST_TYPE_BASIC)
  {
    add_to_list(ls, node->data);
  }
  else if (node->type == AST_TYPE_TUPLE)
  {
    List *tls = (List *)(node->data);
    add_to_list(ls, "(");
    for (int a = 0; a < tls->n; a++)
    {
      if (a)
        add_to_list(ls, ",");
      stringify_type_internal(ls, (AstNode *)get_from_list(tls, a));
    }
    add_to_list(ls, ")");
  }
  else if (node->type == AST_TYPE_FUNC)
  {
    AstListNode *data = (AstListNode *)(node->data);
    if (ls->n == 0 || strcmp((char *)get_from_list(ls, ls->n - 1), "*"))
    {
      add_to_list(ls, "*");
    }
    stringify_type_internal(ls, data->node);
    add_to_list(ls, "(");
    for (int a = 0; a < data->list->n; a++)
    {
      if (a)
        add_to_list(ls, ",");
      stringify_type_internal(ls, (AstNode *)get_from_list(data->list, a));
    }
    add_to_list(ls, ")");
  }
}

/*
  Converts a AST_TYPE_* AstNode into a string representation
  Very helpful for error formatting and debugging
*/
char *stringify_type(AstNode *node)
{
  int len = 1;
  List *ls = new_default_list();
  stringify_type_internal(ls, node);
  for (int a = 0; a < ls->n; a++)
    len += strlen((char *)get_from_list(ls, a));
  char *type = (char *)malloc(sizeof(char) + len);
  type[0] = 0;
  for (int a = 0; a < ls->n; a++)
    strcat(type, (char *)get_from_list(ls, a));
  dealloc_list(ls);
  return type;
}

/*
  Print out types equivalence graph
*/
void print_types_graph()
{
  for (int a = 0; a < types_graph->n; a++)
  {
    EqualTypesNode *e = (EqualTypesNode *)get_from_list(types_graph, a);
    char *type = stringify_type(e->type);
    printf("%i: %s -> %s\n", e->relation, e->name, type);
    free(type);
  }
}
