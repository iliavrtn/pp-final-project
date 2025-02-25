//
// Serious test for comparing quicksort-based deletion vs. AVL tree marking.
// Deletion in quicksort scenario is done by performing a binary search on the sorted
// array and marking the found pointer (using a parallel flags array).
// In the AVL tree scenario deletion is simulated by using contains_node/avl_mark,
// marking the node if found.
//
// Note: Memory for new pointer buffers is allocated once in each scenario to avoid bias.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define forkscan_free(ptr) free(ptr)

// =======================================================================
//                           Quicksort Implementation
// =======================================================================

static void swap(size_t *addrs, int n, int m) {
    size_t tmp = addrs[n];
    addrs[n] = addrs[m];
    addrs[m] = tmp;
}

static int partition(size_t *addrs, int min, int max) {
    int pivot = (min + max) / 2;
    size_t pivot_val = addrs[pivot];
    int mid = min;
    swap(addrs, pivot, max);
    for (int i = min; i < max; i++) {
        if (addrs[i] <= pivot_val) {
            swap(addrs, i, mid);
            mid++;
        }
    }
    swap(addrs, mid, max);
    return mid;
}

static void insertion_sort(size_t *addrs, int min, int max) {
    for (int i = min + 1; i <= max; i++) {
        for (int j = i; j > min && addrs[j - 1] > addrs[j]; j--) {
            swap(addrs, j, j - 1);
        }
    }
}

#define SORT_THRESHOLD 16

static void quicksort(size_t *addrs, int min, int max) {
    if (max - min > SORT_THRESHOLD) {
        int mid = partition(addrs, min, max);
        quicksort(addrs, min, mid - 1);
        quicksort(addrs, mid + 1, max);
    } else {
        insertion_sort(addrs, min, max);
    }
}

void forkscan_util_sort(size_t *a, int length) {
    quicksort(a, 0, length - 1);
}

// -----------------------------------------------------------------------
// Binary search deletion for the quicksort scenario.
// Given a sorted array and a parallel flags array (0 = unmarked, 1 = marked),
// perform binary search for target and mark it if found.
int binary_search_mark(size_t *arr, int n, size_t target, int *flags) {
    int low = 0, high = n - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid] == target) {
            if (flags[mid] == 0) { // not already marked
                flags[mid] = 1;  // mark it
            }
            return mid;
        } else if (target < arr[mid]) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }
    return -1;
}

// Filter out the marked elements (those with flag == 1)
// and update the count for the next iteration.
void filter_marked_array(size_t *arr, int *n, int *flags) {
    int new_n = 0;
    for (int i = 0; i < *n; i++) {
        if (flags[i] == 0) {
            arr[new_n] = arr[i];
            flags[new_n] = 0;  // reset flag for the new array
            new_n++;
        }
    }
    *n = new_n;
}

// =======================================================================
//                           AVL Tree Implementation
// =======================================================================

typedef struct avl_node {
    size_t key;
    int height;
    int marked;               /* 0 = unmarked, 1 = marked */
    struct avl_node *left;
    struct avl_node *right;
} avl_node_t;

typedef struct avl_tree {
    avl_node_t *root;
} avl_tree_t;

static int height(avl_node_t *node) {
    return node ? node->height : 0;
}

static int max_int(int a, int b) {
    return (a > b) ? a : b;
}

static avl_node_t *new_node(size_t key) {
    avl_node_t *node = (avl_node_t *)malloc(sizeof(avl_node_t));
    if (!node)
        exit(1);
    node->key = key;
    node->height = 1;
    node->marked = 0;
    node->left = node->right = NULL;
    return node;
}

static avl_node_t *right_rotate(avl_node_t *y) {
    avl_node_t *x = y->left;
    avl_node_t *T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = max_int(height(y->left), height(y->right)) + 1;
    x->height = max_int(height(x->left), height(x->right)) + 1;
    return x;
}

static avl_node_t *left_rotate(avl_node_t *x) {
    avl_node_t *y = x->right;
    avl_node_t *T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = max_int(height(x->left), height(x->right)) + 1;
    y->height = max_int(height(y->left), height(y->right)) + 1;
    return y;
}

static int get_balance(avl_node_t *node) {
    return node ? height(node->left) - height(node->right) : 0;
}

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

static void destroy_node(avl_node_t *node) {
    if (node) {
        destroy_node(node->left);
        destroy_node(node->right);
        forkscan_free(node);
    }
}

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
        forkscan_free(tree);
    }
}

avl_tree_t *avl_build_from_array(size_t *keys, int n) {
    avl_tree_t *tree = avl_create();
    for (int i = 0; i < n; i++) {
        avl_insert(tree, keys[i]);
    }
    return tree;
}

// =======================================================================
//                         Utility Functions
// =======================================================================

// Generate random "pointer" values.
void generate_random_array(size_t *arr, int n) {
    for (int i = 0; i < n; i++) {
        arr[i] = (size_t)rand();
    }
}

// Timing helper function.
double get_time_in_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Test parameters.
#define INITIAL_SIZE 1000
#define NEW_POINTERS 500000
#define ITERATIONS 100

// =======================================================================
//                   Test: Quicksort Scenario
// =======================================================================

void test_quicksort_scenario() {
    printf("Starting quicksort scenario test...\n");
    int total_capacity = INITIAL_SIZE + ITERATIONS * NEW_POINTERS;
    size_t *array = (size_t *)malloc(sizeof(size_t) * total_capacity);
    int *flags = (int *)calloc(total_capacity, sizeof(int)); // 0 = unmarked, 1 = marked
    int curr_n = INITIAL_SIZE;
    generate_random_array(array, curr_n);

    double start = get_time_in_sec();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        // Merge in NEW_POINTERS new pointers.
        generate_random_array(&array[curr_n], NEW_POINTERS);
        // Initialize flags for the new elements.
        for (int i = curr_n; i < curr_n + NEW_POINTERS; i++) {
            flags[i] = 0;
        }
        curr_n += NEW_POINTERS;

        // Sort the merged array.
        forkscan_util_sort(array, curr_n);

        // For deletion, simulate marking by doing binary search on the sorted array.
        // Here, for every 10th element (which is guaranteed to exist in the sorted array),
        // we try to mark it.
        for (int i = 0; i < curr_n; i += 2) {
            binary_search_mark(array, curr_n, array[i], flags);
        }

        // Remove the marked elements and update the array for the next iteration.
        filter_marked_array(array, &curr_n, flags);
    }
    double end = get_time_in_sec();
    printf("Quicksort scenario: %d pointers remain. Time: %f sec\n", curr_n, end - start);
    free(array);
    free(flags);
}

// =======================================================================
//                      Test: AVL Tree Scenario
// =======================================================================

void test_avl_scenario() {
    printf("Starting AVL tree scenario test...\n");
    // Build the initial AVL tree.
    size_t *init_array = (size_t *)malloc(sizeof(size_t) * INITIAL_SIZE);
    generate_random_array(init_array, INITIAL_SIZE);
    avl_tree_t *tree = avl_build_from_array(init_array, INITIAL_SIZE);
    free(init_array);

    // Preallocate a buffer for new pointers (reuse it each iteration).
    size_t *new_array = (size_t *)malloc(sizeof(size_t) * NEW_POINTERS);

    double start = get_time_in_sec();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        // Generate new pointers.
        generate_random_array(new_array, NEW_POINTERS);
        // Insert new pointers into the AVL tree.
        for (int i = 0; i < NEW_POINTERS; i++) {
            avl_insert(tree, new_array[i]);
        }
        // Simulate deletion by marking every 10th new pointer.
        for (int i = 0; i < NEW_POINTERS; i += 2) {
            avl_mark(tree, new_array[i]);
        }
    }
    double end = get_time_in_sec();
    printf("AVL tree scenario completed. Time: %f sec\n", end - start);
    avl_destroy(tree);
    free(new_array);
}

// =======================================================================
//                                Main
// =======================================================================

int main() {
    srand((unsigned)time(NULL));
    test_quicksort_scenario();
    test_avl_scenario();
    return 0;
}
