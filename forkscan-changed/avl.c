#include "avl.h"
#include <stdlib.h>

/* --- Internal Helper Functions --- */

/* Return the height of a node (0 if node is NULL). */
static int height(avl_node_t *node) {
    return node ? node->height : 0;
}

/* Return the maximum of two integers. */
static int max_int(int a, int b) {
    return (a > b) ? a : b;
}

/* Create a new AVL node for a given key. */
static avl_node_t *new_node(size_t key) {
    avl_node_t *node = (avl_node_t *)malloc(sizeof(avl_node_t));
    if (!node)
        exit(1);  /* Fatal error; adjust error handling as needed */
    node->key = key;
    node->height = 1;
    node->marked = 0;
    node->left = node->right = NULL;
    return node;
}

/* Right-rotate the subtree rooted with y. */
static avl_node_t *right_rotate(avl_node_t *y) {
    avl_node_t *x = y->left;
    avl_node_t *T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = max_int(height(y->left), height(y->right)) + 1;
    x->height = max_int(height(x->left), height(x->right)) + 1;

    return x;
}

/* Left-rotate the subtree rooted with x. */
static avl_node_t *left_rotate(avl_node_t *x) {
    avl_node_t *y = x->right;
    avl_node_t *T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max_int(height(x->left), height(x->right)) + 1;
    y->height = max_int(height(y->left), height(y->right)) + 1;

    return y;
}

/* Get the balance factor of a node. */
static int get_balance(avl_node_t *node) {
    return node ? height(node->left) - height(node->right) : 0;
}

/* Recursively insert a key into the subtree rooted at 'node'.
 * The flag 'inserted' is set to 1 if a new node was created.
 */
static avl_node_t *insert_node(avl_node_t *node, size_t key, int *inserted) {
    if (!node) {
        *inserted = 1;
        return new_node(key);
    }
    if (key < node->key)
        node->left = insert_node(node->left, key, inserted);
    else if (key > node->key)
        node->right = insert_node(node->right, key, inserted);
    else {
        *inserted = 0;
        return node;
    }

    node->height = 1 + max_int(height(node->left), height(node->right));

    int balance = get_balance(node);

    /* Balance the node if needed */
    if (balance > 1 && key < node->left->key)
        return right_rotate(node);
    if (balance < -1 && key > node->right->key)
        return left_rotate(node);
    if (balance > 1 && key > node->left->key) {
        node->left = left_rotate(node->left);
        return right_rotate(node);
    }
    if (balance < -1 && key < node->right->key) {
        node->right = right_rotate(node->right);
        return left_rotate(node);
    }

    return node;
}

/* Recursively search for a key in the subtree. */
static int contains_node(avl_node_t *node, size_t key) {
    if (!node)
        return 0;
    if (key == node->key)
        return 1;
    else if (key < node->key)
        return contains_node(node->left, key);
    else
        return contains_node(node->right, key);
}

/* Recursively search for a key and, if found, mark the node.
 * The flag 'found' is set to 1 if the key is found.
 */
static int mark_node(avl_node_t *node, size_t key, int *found) {
    if (!node)
        return 0;
    if (key == node->key) {
        if (!node->marked)
            node->marked = 1;
        *found = 1;
        return 1;
    } else if (key < node->key)
        return mark_node(node->left, key, found);
    else
        return mark_node(node->right, key, found);
}

/* Recursively free the nodes of the tree. */
static void destroy_node(avl_node_t *node) {
    if (node) {
        destroy_node(node->left);
        destroy_node(node->right);
        free(node);
    }
}

/* In-order traversal helper. */
static void inorder_traverse(avl_node_t *node,
                             void (*f)(avl_node_t *node, void *data),
                             void *data) {
    if (!node)
        return;
    inorder_traverse(node->left, f, data);
    f(node, data);
    inorder_traverse(node->right, f, data);
}

/* --- Public API Functions --- */

avl_tree_t *avl_create(void) {
    avl_tree_t *tree = (avl_tree_t *)malloc(sizeof(avl_tree_t));
    if (!tree)
        exit(1);
    tree->root = NULL;
    return tree;
}

int avl_insert(avl_tree_t *tree, size_t key) {
    int inserted = 0;
    tree->root = insert_node(tree->root, key, &inserted);
    return inserted;
}

int avl_contains(avl_tree_t *tree, size_t key) {
    return contains_node(tree->root, key);
}

int avl_mark(avl_tree_t *tree, size_t key) {
    int found = 0;
    mark_node(tree->root, key, &found);
    return found;
}

void avl_destroy(avl_tree_t *tree) {
    if (tree) {
        destroy_node(tree->root);
        free(tree);
    }
}

avl_tree_t *avl_build_from_array(size_t *keys, int n) {
    avl_tree_t *tree = avl_create();
    for (int i = 0; i < n; i++) {
        avl_insert(tree, keys[i]);
    }
    return tree;
}

void avl_inorder(avl_tree_t *tree,
                 void (*f)(avl_node_t *node, void *data),
                 void *data) {
    inorder_traverse(tree->root, f, data);
}
