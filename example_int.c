/* Example for left-leaning red-black tree: set and map with uint
 * keys and values */

#define _GNU_SOURCE  /* for random() */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "llrb.h"

/*
  Now-ubiquitous container_of macro, reconstructing containing
  structure pointer by member pointer.
*/
#define container_of(ptr, type, member)         \
  ((type*)(((char*)ptr)-offsetof(type,member)))

void fail(const char*str) { fputs(str,stderr); abort(); }

void* nooom(void* ptr)
{
  if (!ptr) {
    fail("out of memory");
  }
  return ptr;
}

/* fail fast on out-of-memory condition */
void* xmalloc(size_t size)
{
  return nooom(malloc(size));
}


struct SetItem {
  struct LLRBNode n;
  unsigned int key;
};

struct MapItem {
  struct LLRBNode n;
  unsigned int key;
  unsigned int value;
};

static int rbcmp_SetItem(struct LLRBNode *a, struct LLRBNode *b, struct LLRBTree*t)
{
  (void)t;
  return
      container_of(a,struct SetItem,n)->key -
      container_of(b,struct SetItem,n)->key;
}

static int rbcmp_MapItem(struct LLRBNode *a, struct LLRBNode *b, struct LLRBTree*t)
{
  (void)t;
  return
      container_of(a,struct MapItem,n)->key -
      container_of(b,struct MapItem,n)->key;
}

int main(void)
{

  struct LLRBTree mySet, myMap;
  struct LLRBNode *rn;
  struct SetItem *s;
  struct MapItem *m;

  int i;
  unsigned int nreplaced = 0, sum = 0;

  llrb_init(&mySet, rbcmp_SetItem);
  llrb_init(&myMap, rbcmp_MapItem);

  puts("Filling set and map:");
  for (i=0; i<20000; ++i) {
    unsigned int k = random() % 20000, v = random();
    s = xmalloc(sizeof *s);
    m = xmalloc(sizeof *m);
    m->key = k;
    m->value = v;
    s->key = k;
    s = container_of(llrb_insert_or_replace(&mySet,&s->n),
                     struct SetItem,n);
    if (s)
      ++ nreplaced;
    free (s);
    m = container_of(llrb_insert_or_replace(&myMap,&m->n),
                     struct MapItem,n);
    free (m);
  }

  printf("Items replaced: %u\n",nreplaced);

  m = container_of(llrb_min(&myMap),struct MapItem,n);
  s = container_of(llrb_min(&mySet),struct SetItem,n);
  printf("Min key in map: %u, set: %u\n",m->key,s->key);

  m = container_of(llrb_max(&myMap),struct MapItem,n);
  s = container_of(llrb_max(&mySet),struct SetItem,n);
  printf("Max key in map: %u, set: %u\n",m->key,s->key);

  puts("Traversing map, printing first 10 pairs");
  for(i = 0, rn = llrb_min(&myMap); rn; ++i, rn = llrb_next(&myMap,rn)) {
    m = container_of(rn,struct MapItem,n);
    sum += m->key;
    if (i<10)
      printf("Map [%u]: %u\n", m->key, m->value);
    /* totally wasting time */
  }
  printf("Sum of all keys: %u\n",sum);

  return 0;
}
