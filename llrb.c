#include "llrb.h"
#include <stddef.h>

enum {LEFT=0,RIGHT,SIDES};
enum {BLACK=0,RED,COLORS};

/* Right rotation:

     h     x
    /   =>  \
   x         h

*/
static struct LLRBNode* rotate(struct LLRBNode *h, int side)
{
  /* Let paradigm rotation be right.  Code below is
   * direction-agnostic, but better understood if we keep one example
   * direction in mind (see also the picture above). */

  int vRight = side, vLeft = !side;
  struct LLRBNode* x = h->child[vLeft];

  h->child[vLeft] = x->child[vRight];
  x->child[vRight] = h;
  x->color = h->color;
  h->color = RED;

  return x;
}

static void colorFlip(struct LLRBNode *h)
{
  h->color = !h->color;
  h->child[LEFT]->color = !h->child[LEFT]->color;
  h->child[RIGHT]->color = !h->child[RIGHT]->color;
}

static int isRed(struct LLRBNode* h)
{
  return h && (h->color == RED);
}

static struct LLRBNode* fixUp(struct LLRBNode *h)
{
  if (isRed(h->child[RIGHT]) && !isRed(h->child[LEFT]))
    h = rotate(h, LEFT);
  if (isRed(h->child[LEFT]) && (isRed(h->child[LEFT]->child[LEFT])))
    h = rotate(h, RIGHT);
  if (isRed(h->child[LEFT]) && (isRed(h->child[RIGHT])))
    colorFlip(h);
  return h;
}

static struct LLRBNode* insert(struct LLRBNode* h, struct LLRBNode* x,
                                  struct LLRBTree* t, struct LLRBNode **old,
                                  struct LLRBNode* p, int side)
{
  int cmp;
  if (!h) {
    x->color = RED;
    x->child[LEFT] = x->child[RIGHT] = NULL;

#ifndef LLRB_NO_LIST
    /* we're left child -> our right neighbour is p */
    x->neigh[1-side] = p;
    /* our left neighbour is p's old left neighbour */
    x->neigh[side] = p->neigh[side];
    /* correct neighbour pointers */
    x->neigh[1-side]->neigh[side] = x;
    x->neigh[side]->neigh[1-side] = x;
#else
    (void)p;
#endif

    return x;
  }
  cmp = t->compare(h,x,t);
  if (cmp==0) {
    *old = h;
    x->child[LEFT] = h->child[LEFT];
    x->child[RIGHT] = h->child[RIGHT];
    x->color = h->color;
#ifndef LLRB_NO_LIST
    x->neigh[LEFT] = h->neigh[LEFT];
    x->neigh[RIGHT] = h->neigh[RIGHT];
    x->neigh[LEFT]->neigh[RIGHT] = x;
    x->neigh[RIGHT]->neigh[LEFT] = x;
#endif
    return x;
  } else {
    side = cmp<0 ? RIGHT : LEFT;
    h->child[side] = insert(h->child[side], x, t, old, h, side);
    return fixUp(h);
  }
}

static struct LLRBNode* moveRedRight(struct LLRBNode* h)
{
  colorFlip(h);
  if (isRed(h->child[LEFT]->child[LEFT])) {
    h = rotate(h,RIGHT);
    colorFlip(h);
  }
  return h;
}

static struct LLRBNode* moveRedLeft(struct LLRBNode* h)
{
  colorFlip(h);
  if (isRed(h->child[RIGHT]->child[LEFT])) {
    h->child[RIGHT] = rotate(h->child[RIGHT],RIGHT);
    h = rotate(h,LEFT);
    colorFlip(h);
  }
  return h;
}

static struct LLRBNode* deleteMin(struct LLRBNode* h, struct LLRBNode** old)
{
  if (!h->child[LEFT]) {
    *old = h;
#ifndef LLRB_NO_LIST
    h->neigh[LEFT]->neigh[RIGHT] = h->neigh[RIGHT];
    h->neigh[RIGHT]->neigh[LEFT] = h->neigh[LEFT];
#endif
    return NULL;
  } else {
    if (!isRed(h->child[LEFT])&&!isRed(h->child[LEFT]->child[LEFT])) {
      h = moveRedLeft(h);
    }
    h->child[LEFT] = deleteMin(h->child[LEFT], old);
    return fixUp(h);
  }
}

static struct LLRBNode* deleteKey(struct LLRBNode* h, struct LLRBNode* key,
                                  struct LLRBTree* t, struct LLRBNode **old)
{
  if (t->compare(key,h,t) < 0) {
    /* going left */
    if (!isRed(h->child[LEFT])&&!isRed(h->child[LEFT]->child[LEFT]))
      h = moveRedLeft(h);
    h->child[LEFT] = deleteKey(h->child[LEFT], key, t, old);
  } else {
    if (isRed(h->child[LEFT]))
      h = rotate(h,RIGHT);

    if (t->compare(key,h,t) == 0 && !h->child[RIGHT]) {
      *old = h;
#ifndef LLRB_NO_LIST
      h->neigh[LEFT]->neigh[RIGHT] = h->neigh[RIGHT];
      h->neigh[RIGHT]->neigh[LEFT] = h->neigh[LEFT];
#endif
      return NULL;
    }

    if (!isRed(h->child[RIGHT])&& !isRed(h->child[RIGHT]->child[LEFT]))
      h = moveRedRight(h);

    if (t->compare(key,h,t) == 0) {
      struct LLRBNode* rmin;
      h->child[RIGHT] = deleteMin(h->child[RIGHT], &rmin);
      rmin->child[LEFT] = h->child[LEFT];
      rmin->child[RIGHT] = h->child[RIGHT];
      rmin->color = h->color;
#ifndef LLRB_NO_LIST
      rmin->neigh[LEFT] = h->neigh[LEFT];
      rmin->neigh[RIGHT] = h->neigh[RIGHT];
      rmin->neigh[LEFT]->neigh[RIGHT] = rmin;
      rmin->neigh[RIGHT]->neigh[LEFT] = rmin;
#endif
      *old = h;
      h = rmin;
    } else {
      h->child[RIGHT] = deleteKey(h->child[RIGHT],key,t,old);
    }
  }
  return fixUp(h);
}

/* Public functions */
void llrb_init(struct LLRBTree* t, LLRBComparator* compare)
{
  t->compare = compare;
  t->root = NULL;
#ifndef LLRB_NO_LIST
  t->anchor.neigh[LEFT] = t->anchor.neigh[RIGHT] = &t->anchor;
  t->anchor.child[LEFT] = t->anchor.child[RIGHT] = NULL;
#endif
}


/* insert node into tree, returning old node with the same key */
struct LLRBNode *llrb_insert_or_replace(struct LLRBTree *tree, struct LLRBNode* node)
{
  struct LLRBNode *old = NULL;
#ifndef LLRB_NO_LIST
  tree->root = insert(tree->root, node, tree, &old, &tree->anchor, LEFT);
#else
  tree->root = insert(tree->root, node, tree, &old, NULL, LEFT);
#endif
  return old;
}

/* find node equal to key under tree->compare */
struct LLRBNode *llrb_find(struct LLRBTree *tree, struct LLRBNode* key)
{
  struct LLRBNode *h = tree->root;
  while (h) {
    int cmp = tree->compare(key,h,tree);
    if (cmp == 0) {
      return h;
    } else if (cmp < 0) {
      h = h->child[LEFT];
    } else {
      h = h->child[RIGHT];
    }
  }
  return NULL;
}


/* unlink minimum (leftmost) node and return it */
struct LLRBNode* llrb_pop_min(struct LLRBTree *tree)
{
  struct LLRBNode *old = NULL;
  if (tree->root) {
    tree->root = deleteMin(tree->root, &old);
    if (tree->root)
      tree->root->color = BLACK;
  }
  return old;
}

/* unlink node equal to key under tree->compare, return unlinked
 * node or NULL if not found. */
struct LLRBNode* llrb_delete(struct LLRBTree *tree, struct LLRBNode *key)
{
  struct LLRBNode *old = NULL;
  tree->root = deleteKey(tree->root, key, tree, &old);
  if (tree->root)
    tree->root->color = BLACK;
  return old;
}

#ifndef LLRB_NO_LIST
/* Follow neighbour pointers (to the right if isRight, to the left
 * otherwise), return corresponding neighbour if it exists, NULL
 * otherwise */

static struct LLRBNode* llrb_neighbour(struct LLRBTree *tree, struct LLRBNode* h, int side)
{
  struct LLRBNode* x = h->neigh[side];
  if (x == &tree->anchor)
    return NULL;
  return x;
}

struct LLRBNode* llrb_next(struct LLRBTree *tree, struct LLRBNode* h)
{
  return llrb_neighbour(tree, h, 1);
}

struct LLRBNode* llrb_prev(struct LLRBTree *tree, struct LLRBNode* h)
{
  return llrb_neighbour(tree, h, 0);
}

/* Minimum (leftmost) node */
struct LLRBNode* llrb_min(struct LLRBTree *tree)
{
  struct LLRBNode *h = tree->anchor.neigh[RIGHT];
  if (h == &tree->anchor)
    return NULL;
  return h;
}

/* Maximum (rightmost) node */
struct LLRBNode* llrb_max(struct LLRBTree *tree)
{
  struct LLRBNode *h = tree->anchor.neigh[LEFT];
  if (h == &tree->anchor)
    return NULL;
  return h;
}

#else  /* LLRB_NO_LIST defined */

struct LLRBNode* llrb_min(struct LLRBTree *tree)
{
  struct LLRBNode *h = tree->root;
  if (h) {
    while (h->child[LEFT])
      h = h->child[LEFT];
  }
  return h;
}

struct LLRBNode* llrb_max(struct LLRBTree *tree)
{
  struct LLRBNode *h = tree->root;
  if (h) {
    while (h->child[RIGHT])
      h = h->child[RIGHT];
  }
  return h;
}

#endif


/* Comparator for pointers */
int llrb_ptrcmp(struct LLRBNode* a, struct LLRBNode* b,struct LLRBTree *tree)
{
  (void)tree;
  return a == b ? 0 : ((char*)a < (char*)b ? -1 : 1);
}
