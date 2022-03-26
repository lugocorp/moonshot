#include "./internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
  Return the number of constructors present within a single class
  Used in class validation procedure, because classes should only have 1 constructor
*/
int num_constructors(ClassNode *data)
{
  int cons = 0;
  for (int a = 0; a < data->ls->n; a++)
  {
    AstNode *e = (AstNode *)get_from_list(data->ls, a);
    if (e->type == AST_FUNCTION)
    {
      FunctionNode *func = (FunctionNode *)(e->data);
      if (func->is_constructor)
        cons++;
    }
  }
  return cons;
}

/*
  Searches a class's methods for a constructor
  Returns NULL if the class has no custom constructor
*/
FunctionNode *get_constructor(ClassNode *data)
{
  for (int a = 0; a < data->ls->n; a++)
  {
    AstNode *e = (AstNode *)get_from_list(data->ls, a);
    if (e->type == AST_FUNCTION)
    {
      FunctionNode *func = (FunctionNode *)(e->data);
      if (func->is_constructor)
        return func;
    }
  }
  return NULL;
}

/*
  Returns a list of all fields given a AST_CLASS or AST_INTERFACE node
  The returned list is full of AstNodes
*/
List *get_all_expected_fields(AstNode *node)
{
  List *ls = new_default_list();
  if (node->type == AST_CLASS)
  {
    ClassNode *clas = (ClassNode *)(node->data);
    append_all(ls, clas->ls);
    for (int a = 0; a < clas->interfaces->n; a++)
    {
      InterfaceNode *inter = interface_exists((char *)get_from_list(clas->interfaces, a));
      AstNode *node1 = new_node(AST_INTERFACE, -1, inter);
      append_all(ls, get_all_expected_fields(node1));
      free(node1);
    }
    clas = class_exists(clas->parent);
    if (clas)
    {
      AstNode *node1 = new_node(AST_CLASS, -1, clas);
      append_all(ls, get_all_expected_fields(node1));
      free(node1);
    }
  }
  else if (node->type == AST_INTERFACE)
  {
    InterfaceNode *inter = (InterfaceNode *)(node->data);
    while (inter)
    {
      append_all(ls, inter->ls);
      inter = interface_exists(inter->parent);
    }
  }
  return ls;
}

/*
  Collects every method from a class or interface's interface ancestors
  Travels back in a straight path for interface nodes
  Travels in a branching path for class nodes
*/
static List *get_interface_ancestor_methods(AstNode *node)
{
  char *name;
  List *ls = new_default_list();
  if (node->type == AST_CLASS)
  {
    ClassNode *c = (ClassNode *)(node->data);
    AstNode *inode = new_node(AST_INTERFACE, -1, NULL);
    while (c)
    {
      for (int a = 0; a < c->interfaces->n; a++)
      {
        name = (char *)get_from_list(c->interfaces, a);
        inode->data = interface_exists(name);
        if (inode->data)
        {
          List *ls1 = get_interface_ancestor_methods(inode);
          append_all(ls, ls1);
          dealloc_list(ls1);
        }
      }
      c = class_exists(c->parent);
    }
    free(inode);
  }
  else if (node->type == AST_INTERFACE)
  {
    InterfaceNode *i = (InterfaceNode *)(node->data);
    while (i)
    {
      for (int a = 0; a < i->ls->n; a++)
        add_to_list(ls, ((AstNode *)get_from_list(i->ls, a))->data);
      i = interface_exists(i->parent);
    }
  }
  return ls;
}

/*
  Collects every method from a class and its class ancestors
  Just goes back through parent classes in a straight path
*/
static List *get_class_ancestor_methods(ClassNode *node)
{
  List *ls = new_default_list();
  while (node)
  {
    for (int a = 0; a < node->ls->n; a++)
    {
      AstNode *e = (AstNode *)get_from_list(node->ls, a);
      if (e->type == AST_FUNCTION)
        add_to_list(ls, e->data);
    }
    node = class_exists(node->parent);
  }
  return ls;
}

/*
  Retrieves all the ancestor methods from a class's ancestor classes and interfaces
  Then subtracts the two lists, returning any missing implementations in a List
*/
List *get_missing_class_methods(ClassNode *c)
{
  AstNode *node = new_node(AST_CLASS, -1, c);
  List *missing = get_interface_ancestor_methods(node);
  List *found = get_class_ancestor_methods(c);
  free(node);
  int a = 0;
  while (a < missing->n)
  {
    int removed = 0;
    for (int b = 0; b < found->n; b++)
    {
      FunctionNode *f1 = (FunctionNode *)get_from_list(missing, a);
      FunctionNode *f2 = (FunctionNode *)get_from_list(found, b);
      if (methods_equivalent(f1, f2))
      {
        remove_from_list(missing, a);
        removed = 1;
        break;
      }
    }
    if (!removed)
      a++;
  }
  dealloc_list(found);
  return missing;
}

/*
  Returns 1 if the two functions have the same signature (name and type)
  Helpful for overriding or implementing methods from ancestors
*/
int methods_equivalent(FunctionNode *f1, FunctionNode *f2)
{
  if (!f1->name || !f2->name)
    return 0;
  assert(f1->name->type == AST_ID); // Assumes the two methods belong to classes (name nodes are of type AST_ID)
  assert(f2->name->type == AST_ID); // Assumes the two methods belong to classes (name nodes are of type AST_ID)
  AstNode *node = new_node(AST_FUNCTION, -1, f1);
  AstNode *type1 = get_type(node);
  free(node);
  node = new_node(AST_FUNCTION, -1, f2);
  AstNode *type2 = get_type(node);
  free(node);
  return !strcmp((char *)(f1->name->data), (char *)(f2->name->data)) && typed_match(type1, type2);
}

/*
  Returns a List containing all the fields or a class and all its ancestors
  The youngest subclasses' methods are first in the List
*/
List *get_all_class_fields(ClassNode *data)
{
  List *ls = new_default_list();
  while (data)
  {
    append_all(ls, data->ls);
    data = class_exists(data->parent);
  }
  return ls;
}

/*
  Calculates subclass fields by flattening ancestor fields into a Map
  ls is a list of ancestor fields (pass result of get_all_class_fields)
  Returns NULL if multiple fields share the same name but different types
*/
Map *collapse_ancestor_class_fields(List *ls)
{
  Map *m = new_default_map();
  for (int a = 0; a < ls->n; a++)
  {
    AstNode *node = (AstNode *)get_from_list(ls, a);
    char *name = NULL;
    if (node->type == AST_DEFINE)
      name = ((BinaryNode *)(node->data))->text;
    if (node->type == AST_FUNCTION)
    {
      FunctionNode *data = (FunctionNode *)(node->data);
      if (!data->name)
        continue;
      assert(data->name->type == AST_ID); // Assumes the function's name is AST_ID
      name = (char *)(data->name->data);
    }
    AstNode *e = (AstNode *)get_from_map(m, name);
    if (e)
    {
      AstNode *rtype = get_type(e);
      AstNode *ltype = get_type(node);
      if (node->type != e->type || !typed_match(ltype, rtype))
      {
        dealloc_map(m);
        return NULL;
      }
    }
    else
    {
      put_in_map(m, name, node);
    }
  }
  return m;
}

/*
  Grabs the FunctionNode that corresponds to the child class's function
  clas should be the parent of the class who has the function method
*/
FunctionNode *get_parent_method(ClassNode *clas, FunctionNode *method)
{
  while (clas)
  {
    for (int a = 0; a < clas->ls->n; a++)
    {
      AstNode *e = (AstNode *)get_from_list(clas->ls, a);
      if (e->type != AST_FUNCTION)
        continue;
      FunctionNode *func = (FunctionNode *)(e->data);
      if (method->is_constructor)
      {
        if (func->is_constructor)
          return func;
      }
      else if (func->is_constructor)
      {
        continue;
      }
      else
      {
        assert(method->name->type == AST_ID); // I'm assuming both method->name and func->name are AST_ID types
        assert(func->name->type == AST_ID);   // I'm assuming both method->name and func->name are AST_ID types
        if (strcmp((char *)(method->name->data), (char *)(func->name->data)))
          continue;
        AstNode *func_type = get_type(e);
        AstNode *m = new_node(AST_FUNCTION, -1, method);
        AstNode *method_type = get_type(m);
        free(m);
        if (typed_match(func_type, method_type))
        {
          return func;
        }
      }
    }
    clas = class_exists(clas->parent);
  }
  return NULL;
}
