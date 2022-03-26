#include "./internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
  Instantiates a new List object with some initial max capacity
*/
List *new_list(int max)
{
  void **items = (void **)malloc(max * sizeof(void *));
  List *ls = (List *)malloc(sizeof(List));
  ls->items = items;
  ls->max = max;
  ls->n = 0;
  return ls;
}

/*
  Instantiates a List with the default initial max capacity
*/
List *new_default_list()
{
  return new_list(10);
}

/*
  Gets the i-th item from a list
  returns null if i is out of range
*/
void *get_from_list(List *ls, int i)
{
  assert(i < ls->n && i >= 0); // Safety check
  return ls->items[i];
}

void *remove_from_list(List *ls, int i)
{
  assert(i < ls->n && i >= 0); // Safety check
  void *e = get_from_list(ls, i);
  for (int a = i + 1; a < ls->n; a++)
    ls->items[a - 1] = ls->items[a];
  ls->items[--(ls->n)] = NULL;
  return e;
}

/*
  Appends an item to a list
  Doubles the list's capacity if it's already full
*/
void add_to_list(List *ls, void *e)
{
  if (ls->n == ls->max)
  {
    void **items = (void **)malloc(ls->max * 2 * sizeof(void *));
    memcpy(items, ls->items, ls->max * sizeof(void *));
    free(ls->items);
    ls->items = items;
    ls->max *= 2;
  }
  ls->items[ls->n++] = e;
}

/*
  Appends every element from ls1 to ls
*/
void append_all(List *ls, List *ls1)
{
  for (int a = 0; a < ls1->n; a++)
  {
    add_to_list(ls, get_from_list(ls1, a));
  }
}

/*
  Deallocates a list object
  Does not touch the list's contents
*/
void dealloc_list(List *ls)
{
  free(ls->items);
  free(ls);
}
