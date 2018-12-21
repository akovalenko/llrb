/* Left-Leaning Red-Black tree with optional ordered double-linked
   list of nodes.

   2018-12-22 Anton Kovalenko <anton@sw4me.com>: this code is hereby
   declared to be in public domain. Don't hesitate to drop me a line
   if you use this code.

   - Readability: Let's keep close to Sedgewick's presentation in
     function names for the sake of it. We specifically avoided adding
     parent pointers, even though it would make possible non-recursive
     tree fixup and also deletion by node pointer without looking up
     the node again.

   - Abstraction: No memory allocation here (caller might want to use
     custom allocators or memory pools); deallocation of nodes that
     are replaced or deleted is also the caller's responsibility.

     It's up to the user whether a tree is intrusive (with user's data
     structure containg LLRBNode). Comparison function has a
     strcmp-like interface (as in Sedgewick), but accepts LLRBTree* as
     an argument, which proved to be useful when the result of
     comparison depends on per-tree information.

   - Portability: With the exception of llrb_compare_pointers, it
     should be all standard C, semantically.

     llrb_compare_pointers is useful as default comparator for
     intrusive elements on the machines where pointers to different
     objects, both casted to char*, have a stable order under the `<'
     operator. Its use on independently malloc'ed structures is
     therefore non-compliant; OTOH, if `LLRBNode's are in the same
     array used as memory pool, llrb_compare_pointers's behavior is
     well-defined.

     As of language dialects, I tried to stay C89-to-C18 compatible.

   - Decisions (enumerated to avoid rehashing them again):

     - No opacity for data structures - to make caller responsible for
     (de)allocation and allow intrusive use with container_of-style
     macro.

     - No parent pointers for the sake of simplicity.

     - Neighbour pointers on nodes are enabled by default, allowing
     non-recursive in-order traversal and instant min/max
     retrieval. It costs extra storage for nodes and trees, and some
     runtime at insertion / deletion; define LLRB_NO_LIST before
     including "llrb.h" to disable them (should be the same when
     compiling llrb.c and using it). Slower, recursive versions of
     llrb_min, llrb_max are provided in this case, but no llrb_next
     and llrb_prev.

     - No "const" for comparator arguments, to avoid imposing
     constness or reckless casting on user (e.g. if there is some
     per-tree or per-node caching going on).

     - Ugliness of having to create a temporal node as a search
     pattern for deletion: let's just live with it.

     - Inefficiency of going down the tree when deleting LLRBNode*
     whose pointer is already known: ditto.

     - When a node is deleted or replaced, pointers in the node aren't
     reset to NULL: it would usually be a waste of time, and examining
     former neighbours with llrb_prev / llrb_next proved to be useful
     sometimes. Let's keep it valid.
*/

#ifndef LLRB_H__INCLUDED
#define LLRB_H__INCLUDED

struct LLRBNode;
struct LLRBTree;

/* LLRBComparator function, like strcmp. For integer keys, just return
 * key(a)-key(b). */
typedef int LLRBComparator (struct LLRBNode *a, struct LLRBNode *b, struct LLRBTree *t);

struct LLRBNode {
  int color;  /* Not bool: avoid stdbool.h dependency */
  struct LLRBNode *child[2];
#ifndef LLRB_NO_LIST
  struct LLRBNode *neigh[2];
#endif
};

struct LLRBTree {
#ifndef LLRB_NO_LIST
  /* End-of-list marker for neigh[] links */
  struct LLRBNode anchor;
#endif
  struct LLRBNode *root;
  LLRBComparator* compare;
};

void
llrb_init(struct LLRBTree* t, LLRBComparator* compare);

struct LLRBNode*
llrb_insert_or_replace(struct LLRBTree *tree, struct LLRBNode* node);

struct LLRBNode*
llrb_find(struct LLRBTree *tree, struct LLRBNode* key);

struct LLRBNode*
llrb_pop_min(struct LLRBTree *tree);

struct LLRBNode*
llrb_delete(struct LLRBTree *tree, struct LLRBNode *needle);

struct LLRBNode*
llrb_min(struct LLRBTree *tree);

struct LLRBNode*
llrb_max(struct LLRBTree *tree);

#ifndef LLRB_NO_LIST

struct LLRBNode*
llrb_next(struct LLRBTree *tree, struct LLRBNode* h);

struct LLRBNode*
llrb_prev(struct LLRBTree *tree, struct LLRBNode* h);

#endif  /* LLRB_NO_LIST */

LLRBComparator llrb_ptrcmp;

#endif  /* LLRB_H__INCLUDED */
