#include "./internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
  Instantiates a new Map object with some initial max capacity
*/
Map *new_map(int max)
{
  Pair *items = (Pair *)malloc(max * sizeof(Pair));
  Map *m = (Map *)malloc(sizeof(Map));
  m->data = items;
  m->max = max;
  m->n = 0;
  return m;
}

/*
  Instantiates a Map with the default initial max capacity
*/
Map *new_default_map()
{
  return new_map(10);
}

/*
  Returns a value associated with some key from a map
*/
void *get_from_map(Map *m, char *k)
{
  for (int a = 0; a < m->n; a++)
  {
    Pair p = m->data[a];
    if (!strcmp(p.k, k))
      return p.v;
  }
  return NULL;
}

/*
  Returns a value at arbitrary position i within a map
  Used in traversal algorithms
*/
void *iterate_from_map(Map *m, int i)
{
  assert(i < m->n && i >= 0); // Safety check
  return (m->data[i]).v;
}

/*
  Puts a key-value pair in a map
  Doubles the max length of the map if it's already full
  Replaces the value associates with key k if it already exists
*/
void put_in_map(Map *m, char *k, void *v)
{
  for (int a = 0; a < m->n; a++)
  {
    Pair p = m->data[a];
    if (!strcmp(p.k, k))
    {
      (m->data[a]).v = v;
      return;
    }
  }
  if (m->n == m->max)
  {
    m->max *= 2;
    Pair *tmp = (Pair *)malloc(sizeof(Pair) * m->max);
    for (int a = 0; a < m->n; a++)
      tmp[a] = m->data[a];
    free(m->data);
    m->data = tmp;
  }
  m->data[m->n].k = k;
  m->data[m->n].v = v;
  m->n++;
}

/*
  Deallocates a map
  Does not touch the map's contents
*/
void dealloc_map(Map *m)
{
  free(m->data);
  free(m);
}
