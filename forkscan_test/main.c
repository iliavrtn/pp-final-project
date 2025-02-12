#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "forkscan.h"

// -------------------------------------------------------------------------
// A trivial array-based data structure for concurrency testing
// -------------------------------------------------------------------------

typedef struct Node {
    int key;
    _Atomic(int) collected; // New field for tracking reclamation state.
} Node;

#define KEY_RANGE 1048576
// Our array of Node*:
static _Atomic(Node*) array[KEY_RANGE];

// -------------------------------------------------------------------------
// add(key): Return true if inserted, false if key already in
// -------------------------------------------------------------------------
bool add(int key) {
    Node* oldval = atomic_load_explicit(&array[key], memory_order_acquire);
    if (oldval != NULL) {
        return false;
    }

    // Allocate a node for this key
    Node* newNode = (Node*)forkscan_malloc(sizeof(Node));
    if (!newNode) {
        perror("malloc for newNode");
        exit(EXIT_FAILURE);
    }
    newNode->key = key;
    // Initialize the collected flag to 0 (not yet collected)
    atomic_init(&newNode->collected, 0);

    // Attempt CAS to store newNode if array[key] is still NULL
    Node* expected = NULL;
    bool success = atomic_compare_exchange_strong_explicit(
        &array[key],
        &expected,
        newNode,
        memory_order_acq_rel,
        memory_order_acquire);

    if (success) {
        return true;
    } else {
        free(newNode);
        return false;
    }
}

// -------------------------------------------------------------------------
// remove_node(key): Return true if removed, false if not present
// -------------------------------------------------------------------------
bool remove_node(int key) {

    // Atomically swap out array[key] = NULL.
    // This ensures that only one thread will remove the node.
    Node* oldval = atomic_exchange_explicit(&array[key], NULL, memory_order_acq_rel);
    if (oldval == NULL) {
        return false;
    }

    // Once the pointer is out of the array, we call forkscan_retire.
    // But first, we check and update the node's "collected" flag.
    int wasCollected = atomic_exchange_explicit(&oldval->collected, 1, memory_order_acq_rel);
    if (wasCollected == 0) {
        // The node has not yet been scheduled for reclamation.
        forkscan_retire(oldval);
    } else {
        // The node has already been handed off; do not call forkscan_retire again.
    }

    return true;
}

// -------------------------------------------------------------------------
// contains(key): return true if key is present, else false
// -------------------------------------------------------------------------
bool contains(int key) {
    fflush(stdout);

    Node* n = atomic_load_explicit(&array[key], memory_order_acquire);
    bool present = (n != NULL);
    fflush(stdout);

    return present;
}

// -------------------------------------------------------------------------
// Test / Benchmark
// -------------------------------------------------------------------------

#define INITIAL_SIZE  524288  // how many random elements to pre-insert
#define UPDATE_RATIO  20    // out of 100: % of operations that update (insert/remove)
#define REMOVE_RATIO  10    // half the update operations are removes, half are adds
#define RUNTIME_SEC   5     // each thread runs for 5 seconds

static int num_threads = 1;

// We'll track how many operations each thread does:
typedef struct {
    int thread_id;
    long long operations;
} thread_arg_t;

void* worker_thread(void* arg) {
    thread_arg_t* targs = (thread_arg_t*) arg;
    int tid = targs->thread_id;

    // Per-thread random seed.
    unsigned int seed = (unsigned int)(time(NULL) ^ (tid * 123456789ULL));

    time_t start = time(NULL);
    while ((time(NULL) - start) < RUNTIME_SEC) {
        int r   = rand_r(&seed) % 100;   // random number 0..99
        int key = rand_r(&seed) % KEY_RANGE;
        if (r < UPDATE_RATIO) {
            // ~50% perform updates.
            if (r < REMOVE_RATIO) {
                // Half of updates are removes.
                remove_node(key);
            } else {
                // The other half are adds.
                add(key);
            }
        } else {
            // The other ~50% perform contains.
            contains(key);
        }
        targs->operations++;
    }

    return NULL;
}

int main(int argc, char** argv) {
    if (argc > 1) {
        num_threads = atoi(argv[1]);
        if (num_threads < 1)
            num_threads = 1;
        if (num_threads > 80)
            num_threads = 80; // up to 80 threads
    }

    printf("[MAIN] Using %d threads, each runs for %d seconds, array populated with %d elements, key range %d...\n",
           num_threads, RUNTIME_SEC, atoi(argv[2]), atoi(argv[3]));
    fflush(stdout);

    // Clear the array.
    memset((void*)array, 0, sizeof(array));

    srand(time(NULL));
    int inserted = 0;
    while (inserted < atoi(argv[2])) {
        int k = rand() % atoi(argv[3]);
        if (add(k)) {
            inserted++;
        }
    }

    // Launch worker threads.
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    thread_arg_t* args = (thread_arg_t*)calloc(num_threads, sizeof(thread_arg_t));
    if (!threads || !args) {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].operations = 0;
        pthread_create(&threads[i], NULL, worker_thread, &args[i]);
    }

    // Wait for them to finish.
    long long total_ops = 0;
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total_ops += args[i].operations;
    }
    free(threads);
    free(args);

    double ops_per_sec = (double)total_ops / (double)RUNTIME_SEC;
    printf("[MAIN] Throughput: %f ops/sec\n", ops_per_sec);
    printf("=========================================================\n\n");
    return 0;
}