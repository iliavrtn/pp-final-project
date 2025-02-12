#ifndef AVL_H
#define AVL_H

#include <stddef.h>

/* AVL node structure.
 * Each node holds a key (a retired pointer address), a height,
 * and a “marked” flag that indicates whether the pointer is still referenced.
 */
typedef struct avl_node {
    size_t key;
    int height;
    int marked;               /* 0 = unmarked, 1 = marked */
    struct avl_node *left;
    struct avl_node *right;
} avl_node_t;

/* AVL tree structure – holds a pointer to the root node. */
typedef struct avl_tree {
    avl_node_t *root;
} avl_tree_t;

/* Create an empty AVL tree. */
avl_tree_t *avl_create(void);

/* Destroy the AVL tree and free all its nodes. */
void avl_destroy(avl_tree_t *tree);

/* Insert a key into the AVL tree.
 * Returns 1 if the key was inserted, or 0 if the key already exists.
 */
int avl_insert(avl_tree_t *tree, size_t key);

/* Check whether the AVL tree contains a key.
 * Returns 1 if found, 0 otherwise.
 */
int avl_contains(avl_tree_t *tree, size_t key);

/* Mark a key in the AVL tree.
 * If the key is found, set its marked flag (if not already marked) and return 1.
 * If not found, return 0.
 */
int avl_mark(avl_tree_t *tree, size_t key);

/* Build an AVL tree from an unsorted array of keys.
 * 'keys' is an array of 'n' size_t values.
 */
avl_tree_t *avl_build_from_array(size_t *keys, int n);

/* In-order traversal of the AVL tree.
 * For each node, function 'f' is called with the node and the provided 'data' pointer.
 */
void avl_inorder(avl_tree_t *tree,
                 void (*f)(avl_node_t *node, void *data),
                 void *data);

#endif /* AVL_H */