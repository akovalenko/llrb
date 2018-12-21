/* Example usage left-leaning red-black tree. */

#define _GNU_SOURCE  /* for getline() */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "llrb.h"

/*
  Now-ubiquitous container_of macro, reconstructing containing
  structure pointer by member pointer. Can be defined in a safer way
  than here when typeof extension is available.
*/

#define container_of(ptr, type, member)         \
  ((type*)(((char*)ptr)-offsetof(type,member)))

void fail(const char*str)
{
  fputs(str,stderr);
  abort();
}

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

char *xstrdup(const char *s)
{
  return nooom(strdup(s));
}

/* struct StringNode is our data item, containing string as user data
 * and participation in TWO trees independently. */

struct StringNode {
  struct LLRBNode n;
  struct LLRBNode aux;
  char* str;
};

/* Getting original StringNode from LLRBNode member "n" is possible
 * with type cast, because "n" is the first member in StringNode.
 * Otherwise we'd have to use container_of (which we have for auxilary
 * tree and aux member, anyway).
 *
 * There's an advantage of having LLRBNode as a first member, namely,
 * that pointer to LLRBNode is null iff pointer to StringNode is
 * null. */
#define TO_SN(node) ((struct StringNode*)node)

/* trivial wrapper for strcmp */
int rbcmp_string(struct LLRBNode *a, struct LLRBNode *b, struct LLRBTree*t)
{
  (void)t;
  return strcmp(TO_SN(a)->str,TO_SN(b)->str);
}

int main(void)
{

  /* getline stuff */
  char *line = NULL;
  size_t sz = 0;

  /* sorted by string value */
  struct LLRBTree tree;

  /* sorted by pointer location */
  struct LLRBTree auxtree;

  struct StringNode *sn;
  int ndups = 0;

  struct LLRBNode *rn;

  llrb_init(&tree, rbcmp_string);
  llrb_init(&auxtree, llrb_ptrcmp);

  for(;;) {  /* for each line of <stdin> */
    ssize_t r = getline(&line,&sz,stdin);

    if (r<0)  /* EOF/error? */
      break;

    if (r>0 && line[r-1]=='\n')
      line[r-1] = 0;  /* chop final newline */

    sn = xmalloc(sizeof *sn);
    sn->str = xstrdup(line);

    /* add node to auxtree first */
    if (NULL != llrb_insert_or_replace(&auxtree, &sn->aux))
      fail("should not happen: replaced node in pointer-sorted tree!");

    sn = TO_SN(llrb_insert_or_replace(&tree,&sn->n));
    if (sn) {
      /* old node, not in &tree anymore */
      /* delete from &auxtree as well */
      llrb_delete(&auxtree,&sn->aux);
      ++ndups;
      free(sn->str);
      free(sn);
    }
  }
  free(line);

  printf("\nDuplicate string: %d\n",ndups);

  puts("\nTraversal by strcmp order:");

  /* TO_SN cast preserves NULLness, so we need no separate node
   * pointer during traversal of &tree */
  for(sn = TO_SN(llrb_min(&tree)); sn; sn = TO_SN(llrb_next(&tree,&sn->n))) {
    puts(sn->str);
  }

  puts("\nTraversal by memory order:");

  /* That's how it's done with container_of: LLRBNode member might be
   * anywhere in our structure */
  for(rn = llrb_min(&auxtree); rn; rn = llrb_next(&auxtree,rn)) {
    sn = container_of(rn,struct StringNode,aux);
    puts(sn->str);
  }

  {
    struct StringNode key = { .str = "password" };
    rn = llrb_find(&tree,&key.n);
    if (rn) {
      printf("Seen in the tree: %s\n", container_of(rn,struct StringNode,n)->str);
    }
  }



  /* Free allocated memory using linked-list traversal.  Both trees
     become invalid as StringNodes containing both kind of LLRBNodes
     are freed. */

  for(rn = llrb_min(&tree); rn;) {
    /* NB we MUST get next pointer before freeing current node. */
    struct LLRBNode *newrn = llrb_next(&tree,rn);

    /* TO_SN cast would do. container_of used for demonstration */
    sn = container_of(rn,struct StringNode,n);
    free(sn->str);
    free(sn);
    rn = newrn;
  }

  return 0;
}
