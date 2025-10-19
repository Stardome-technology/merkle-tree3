#include "merkle_tree.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Type operations for int32_t
void merge_int32(const void* left, const void* right, void* result) {
    const int32_t* l = (const int32_t*)left;
    const int32_t* r = (const int32_t*)right;
    int32_t* res = (int32_t*)result;
    *res = *r - *l;  // Same as the Rust implementation: right.wrapping_sub(*left)
}

int compare_int32(const void* a, const void* b) {
    const int32_t* ia = (const int32_t*)a;
    const int32_t* ib = (const int32_t*)b;
    if (*ia < *ib) return -1;
    if (*ia > *ib) return 1;
    return 0;
}

void copy_int32(const void* src, void* dst) {
    *(int32_t*)dst = *(const int32_t*)src;
}

void default_int32(void* item) {
    *(int32_t*)item = 0;
}

const type_ops_t int32_ops = {
    .merge = merge_int32,
    .compare = compare_int32,
    .copy = copy_int32,
    .default_init = default_int32,
    .item_size = sizeof(int32_t)
};

// Tree index operations
uint32_t tree_index_sibling(uint32_t index) {
    if (index == 0) {
        return 0;
    }
    return ((index + 1) ^ 1) - 1;
}

uint32_t tree_index_parent(uint32_t index) {
    if (index == 0) {
        return 0;
    }
    return (index - 1) >> 1;
}

bool tree_index_is_left(uint32_t index) {
    return (index & 1) == 1;
}

// Dynamic array structure for internal use
typedef struct {
    void* data;
    size_t count;
    size_t capacity;
    size_t item_size;
} dynamic_array_t;

static dynamic_array_t* dynamic_array_new(size_t item_size) {
    dynamic_array_t* arr = malloc(sizeof(dynamic_array_t));
    if (!arr) return NULL;
    
    arr->data = NULL;
    arr->count = 0;
    arr->capacity = 0;
    arr->item_size = item_size;
    return arr;
}

static void dynamic_array_free(dynamic_array_t* arr) {
    if (arr) {
        free(arr->data);
        free(arr);
    }
}

static bool dynamic_array_reserve(dynamic_array_t* arr, size_t capacity) {
    if (capacity <= arr->capacity) return true;
    
    size_t new_capacity = arr->capacity == 0 ? 1 : arr->capacity;
    while (new_capacity < capacity) {
        new_capacity *= 2;
    }
    
    void* new_data = realloc(arr->data, new_capacity * arr->item_size);
    if (!new_data) return false;
    
    arr->data = new_data;
    arr->capacity = new_capacity;
    return true;
}

static bool dynamic_array_push(dynamic_array_t* arr, const void* item) {
    if (!dynamic_array_reserve(arr, arr->count + 1)) return false;
    
    memcpy((char*)arr->data + arr->count * arr->item_size, item, arr->item_size);
    arr->count++;
    return true;
}

static void* dynamic_array_get(const dynamic_array_t* arr, size_t index) {
    if (index >= arr->count) return NULL;
    return (char*)arr->data + index * arr->item_size;
}

// Simple queue implementation for BFS
typedef struct queue_node {
    void* data;
    struct queue_node* next;
} queue_node_t;

typedef struct {
    queue_node_t* front;
    queue_node_t* rear;
    size_t item_size;
} queue_t;

static queue_t* queue_new(size_t item_size) {
    queue_t* q = malloc(sizeof(queue_t));
    if (!q) return NULL;
    
    q->front = NULL;
    q->rear = NULL;
    q->item_size = item_size;
    return q;
}

static void queue_free(queue_t* q) {
    if (!q) return;
    
    while (q->front) {
        queue_node_t* temp = q->front;
        q->front = q->front->next;
        free(temp->data);
        free(temp);
    }
    free(q);
}

static bool queue_push(queue_t* q, const void* item) {
    queue_node_t* node = malloc(sizeof(queue_node_t));
    if (!node) return false;
    
    node->data = malloc(q->item_size);
    if (!node->data) {
        free(node);
        return false;
    }
    
    memcpy(node->data, item, q->item_size);
    node->next = NULL;
    
    if (q->rear) {
        q->rear->next = node;
    } else {
        q->front = node;
    }
    q->rear = node;
    
    return true;
}

static bool queue_pop(queue_t* q, void* item) {
    if (!q->front) return false;
    
    queue_node_t* temp = q->front;
    memcpy(item, temp->data, q->item_size);
    
    q->front = q->front->next;
    if (!q->front) {
        q->rear = NULL;
    }
    
    free(temp->data);
    free(temp);
    return true;
}

static bool queue_is_empty(const queue_t* q) {
    return q->front == NULL;
}

static void* queue_front(const queue_t* q) {
    return q->front ? q->front->data : NULL;
}

// Comparison function for qsort
static type_ops_t* global_ops = NULL;
static void* global_nodes = NULL;

static int compare_wrapper(const void* a, const void* b) {
    return global_ops->compare(a, b);
}

static int compare_indices_by_node_value(const void* a, const void* b) {
    uint32_t idx_a = *(const uint32_t*)a;
    uint32_t idx_b = *(const uint32_t*)b;
    
    void* node_a = (char*)global_nodes + idx_a * global_ops->item_size;
    void* node_b = (char*)global_nodes + idx_b * global_ops->item_size;
    
    return global_ops->compare(node_a, node_b);
}

static int compare_uint32_reverse(const void* a, const void* b) {
    uint32_t ua = *(const uint32_t*)a;
    uint32_t ub = *(const uint32_t*)b;
    if (ua > ub) return -1;
    if (ua < ub) return 1;
    return 0;
}

static int compare_pairs_by_index_reverse(const void* a, const void* b) {
    const index_node_pair_t* pa = (const index_node_pair_t*)a;
    const index_node_pair_t* pb = (const index_node_pair_t*)b;
    if (pa->index > pb->index) return -1;
    if (pa->index < pb->index) return 1;
    return 0;
}

// Merkle Tree operations
merkle_tree_t* merkle_tree_new(const type_ops_t* ops) {
    merkle_tree_t* tree = malloc(sizeof(merkle_tree_t));
    if (!tree) return NULL;
    
    tree->nodes = NULL;
    tree->nodes_count = 0;
    tree->ops = *ops;
    return tree;
}

void merkle_tree_free(merkle_tree_t* tree) {
    if (tree) {
        free(tree->nodes);
        free(tree);
    }
}

merkle_result_t merkle_tree_build_proof(const merkle_tree_t* tree, const uint32_t* leaf_indices, size_t indices_count) {
    merkle_result_t result = {false, {NULL}};
    
    if (!tree || !tree->nodes || tree->nodes_count == 0 || !leaf_indices || indices_count == 0) {
        return result;
    }
    
    uint32_t leaves_count = ((tree->nodes_count >> 1) + 1);
    
    // Convert leaf indices to node indices and sort in reverse order
    uint32_t* indices = malloc(indices_count * sizeof(uint32_t));
    if (!indices) return result;
    
    for (size_t i = 0; i < indices_count; i++) {
        indices[i] = leaves_count + leaf_indices[i] - 1;
    }
    
    qsort(indices, indices_count, sizeof(uint32_t), compare_uint32_reverse);
    
    // Check bounds
    if (indices[0] >= (leaves_count << 1) - 1) {
        free(indices);
        return result;
    }
    
    dynamic_array_t* lemmas = dynamic_array_new(tree->ops.item_size);
    if (!lemmas) {
        free(indices);
        return result;
    }
    
    queue_t* queue = queue_new(sizeof(uint32_t));
    if (!queue) {
        free(indices);
        dynamic_array_free(lemmas);
        return result;
    }
    
    // Initialize queue with indices
    for (size_t i = 0; i < indices_count; i++) {
        queue_push(queue, &indices[i]);
    }
    
    uint32_t index;
    while (queue_pop(queue, &index)) {
        if (index == 0) {
            assert(queue_is_empty(queue));
            break;
        }
        
        uint32_t sibling = tree_index_sibling(index);
        uint32_t* front = (uint32_t*)queue_front(queue);
        
        if (front && *front == sibling) {
            queue_pop(queue, &sibling); // consume the sibling from queue
        } else {
            // Add sibling node to lemmas
            void* sibling_node = (char*)tree->nodes + sibling * tree->ops.item_size;
            dynamic_array_push(lemmas, sibling_node);
        }
        
        uint32_t parent = tree_index_parent(index);
        if (parent != 0) {
            queue_push(queue, &parent);
        }
    }
    
    // Sort indices by node values for the proof
    global_ops = (type_ops_t*)&tree->ops;
    global_nodes = tree->nodes;
    qsort(indices, indices_count, sizeof(uint32_t), compare_indices_by_node_value);
    global_ops = NULL;
    global_nodes = NULL;
    
    // Create proof
    merkle_proof_t* proof = merkle_proof_new(indices, indices_count, 
                                             lemmas->data, lemmas->count, &tree->ops);
    
    free(indices);
    queue_free(queue);
    dynamic_array_free(lemmas);
    
    result.success = (proof != NULL);
    result.proof = proof;
    return result;
}

void merkle_tree_root(const merkle_tree_t* tree, void* result) {
    if (!tree || !result) return;
    
    if (tree->nodes_count == 0) {
        tree->ops.default_init(result);
    } else {
        tree->ops.copy(tree->nodes, result);
    }
}

const void* merkle_tree_nodes(const merkle_tree_t* tree) {
    return tree ? tree->nodes : NULL;
}

size_t merkle_tree_nodes_count(const merkle_tree_t* tree) {
    return tree ? tree->nodes_count : 0;
}

// Merkle Proof operations
merkle_proof_t* merkle_proof_new(const uint32_t* indices, size_t indices_count, 
                                 const void* lemmas, size_t lemmas_count, const type_ops_t* ops) {
    if (!indices || !ops) return NULL;
    
    merkle_proof_t* proof = malloc(sizeof(merkle_proof_t));
    if (!proof) return NULL;
    
    proof->indices = malloc(indices_count * sizeof(uint32_t));
    if (!proof->indices) {
        free(proof);
        return NULL;
    }
    
    proof->lemmas = malloc(lemmas_count * ops->item_size);
    if (!proof->lemmas && lemmas_count > 0) {
        free(proof->indices);
        free(proof);
        return NULL;
    }
    
    memcpy(proof->indices, indices, indices_count * sizeof(uint32_t));
    if (lemmas && lemmas_count > 0) {
        memcpy(proof->lemmas, lemmas, lemmas_count * ops->item_size);
    }
    
    proof->indices_count = indices_count;
    proof->lemmas_count = lemmas_count;
    proof->ops = *ops;
    
    return proof;
}

void merkle_proof_free(merkle_proof_t* proof) {
    if (proof) {
        free(proof->indices);
        free(proof->lemmas);
        free(proof);
    }
}

typedef struct {
    uint32_t index;
    void* node;
} index_node_pair_t;

bool merkle_proof_root(const merkle_proof_t* proof, const void* leaves, size_t leaves_count, void* result) {
    if (!proof || !leaves || !result || leaves_count != proof->indices_count || leaves_count == 0) {
        return false;
    }
    
    // Create sorted leaves array
    void* sorted_leaves = malloc(leaves_count * proof->ops.item_size);
    if (!sorted_leaves) return false;
    
    memcpy(sorted_leaves, leaves, leaves_count * proof->ops.item_size);
    global_ops = (type_ops_t*)&proof->ops;
    qsort(sorted_leaves, leaves_count, proof->ops.item_size, compare_wrapper);
    global_ops = NULL;
    
    // Create index-node pairs and sort by index (reverse order)
    index_node_pair_t* pairs = malloc(leaves_count * sizeof(index_node_pair_t));
    if (!pairs) {
        free(sorted_leaves);
        return false;
    }
    
    for (size_t i = 0; i < leaves_count; i++) {
        pairs[i].index = proof->indices[i];
        pairs[i].node = malloc(proof->ops.item_size);
        if (!pairs[i].node) {
            // Cleanup allocated nodes
            for (size_t j = 0; j < i; j++) {
                free(pairs[j].node);
            }
            free(pairs);
            free(sorted_leaves);
            return false;
        }
        proof->ops.copy((char*)sorted_leaves + i * proof->ops.item_size, pairs[i].node);
    }
    
    // Sort pairs by index in reverse order
    qsort(pairs, leaves_count, sizeof(index_node_pair_t), compare_pairs_by_index_reverse);
    
    queue_t* queue = queue_new(sizeof(index_node_pair_t));
    if (!queue) {
        for (size_t i = 0; i < leaves_count; i++) {
            free(pairs[i].node);
        }
        free(pairs);
        free(sorted_leaves);
        return false;
    }
    
    // Initialize queue
    for (size_t i = 0; i < leaves_count; i++) {
        queue_push(queue, &pairs[i]);
    }
    
    size_t lemma_index = 0;
    index_node_pair_t current_pair;
    bool success = false;
    
    while (queue_pop(queue, &current_pair)) {
        if (current_pair.index == 0) {
            // Check if all lemmas and queue items are consumed
            if (lemma_index == proof->lemmas_count && queue_is_empty(queue)) {
                proof->ops.copy(current_pair.node, result);
                success = true;
            }
            free(current_pair.node);
            break;
        }
        
        uint32_t sibling_index = tree_index_sibling(current_pair.index);
        index_node_pair_t* front = (index_node_pair_t*)queue_front(queue);
        
        void* sibling_node = NULL;
        bool should_free_sibling = false;
        
        if (front && front->index == sibling_index) {
            index_node_pair_t sibling_pair;
            queue_pop(queue, &sibling_pair);
            sibling_node = sibling_pair.node;
            should_free_sibling = true;
        } else if (lemma_index < proof->lemmas_count) {
            sibling_node = (char*)proof->lemmas + lemma_index * proof->ops.item_size;
            lemma_index++;
        }
        
        if (sibling_node) {
            void* parent_node = malloc(proof->ops.item_size);
            if (!parent_node) {
                free(current_pair.node);
                if (should_free_sibling) free(sibling_node);
                break;
            }
            
            if (tree_index_is_left(current_pair.index)) {
                proof->ops.merge(current_pair.node, sibling_node, parent_node);
            } else {
                proof->ops.merge(sibling_node, current_pair.node, parent_node);
            }
            
            index_node_pair_t parent_pair = {tree_index_parent(current_pair.index), parent_node};
            queue_push(queue, &parent_pair);
            
            if (should_free_sibling) free(sibling_node);
        }
        
        free(current_pair.node);
    }
    
    // Cleanup remaining queue items
    while (queue_pop(queue, &current_pair)) {
        free(current_pair.node);
    }
    
    for (size_t i = 0; i < leaves_count; i++) {
        free(pairs[i].node);
    }
    free(pairs);
    free(sorted_leaves);
    queue_free(queue);
    
    return success;
}

bool merkle_proof_verify(const merkle_proof_t* proof, const void* root, const void* leaves, size_t leaves_count) {
    if (!proof || !root || !leaves) return false;
    
    void* computed_root = malloc(proof->ops.item_size);
    if (!computed_root) return false;
    
    bool success = merkle_proof_root(proof, leaves, leaves_count, computed_root);
    if (success) {
        success = (proof->ops.compare(computed_root, root) == 0);
    }
    
    free(computed_root);
    return success;
}

const uint32_t* merkle_proof_indices(const merkle_proof_t* proof) {
    return proof ? proof->indices : NULL;
}

size_t merkle_proof_indices_count(const merkle_proof_t* proof) {
    return proof ? proof->indices_count : 0;
}

const void* merkle_proof_lemmas(const merkle_proof_t* proof) {
    return proof ? proof->lemmas : NULL;
}

size_t merkle_proof_lemmas_count(const merkle_proof_t* proof) {
    return proof ? proof->lemmas_count : 0;
}

// CBMT operations
void cbmt_build_merkle_root(const void* leaves, size_t leaves_count, const type_ops_t* ops, void* result) {
    if (!leaves || !ops || !result) return;
    
    if (leaves_count == 0) {
        ops->default_init(result);
        return;
    }
    
    queue_t* queue = queue_new(ops->item_size);
    if (!queue) {
        ops->default_init(result);
        return;
    }
    
    // Process leaves in reverse chunks of 2
    size_t i = leaves_count;
    while (i >= 2) {
        const void* leaf1 = (const char*)leaves + (i - 2) * ops->item_size;
        const void* leaf2 = (const char*)leaves + (i - 1) * ops->item_size;
        
        void* merged = malloc(ops->item_size);
        if (!merged) {
            queue_free(queue);
            ops->default_init(result);
            return;
        }
        
        ops->merge(leaf1, leaf2, merged);
        queue_push(queue, merged);
        free(merged);
        
        i -= 2;
    }
    
    // Handle odd leaf
    if (i == 1) {
        queue_push(queue, leaves);
    }
    
    // Process queue until one element remains
    void* temp1 = malloc(ops->item_size);
    void* temp2 = malloc(ops->item_size);
    void* merged = malloc(ops->item_size);
    
    if (!temp1 || !temp2 || !merged) {
        free(temp1);
        free(temp2);
        free(merged);
        queue_free(queue);
        ops->default_init(result);
        return;
    }
    
    while (!queue_is_empty(queue)) {
        if (!queue_pop(queue, temp1)) break;
        
        if (queue_is_empty(queue)) {
            ops->copy(temp1, result);
            break;
        }
        
        if (!queue_pop(queue, temp2)) {
            ops->copy(temp1, result);
            break;
        }
        
        ops->merge(temp1, temp2, merged);
        queue_push(queue, merged);
    }
    
    free(temp1);
    free(temp2);
    free(merged);
    queue_free(queue);
}

merkle_tree_t* cbmt_build_merkle_tree(const void* leaves, size_t leaves_count, const type_ops_t* ops) {
    if (!ops) return NULL;
    
    merkle_tree_t* tree = merkle_tree_new(ops);
    if (!tree) return NULL;
    
    if (leaves_count == 0) {
        return tree;
    }
    
    size_t total_nodes = (leaves_count << 1) - 1;
    tree->nodes = malloc(total_nodes * ops->item_size);
    if (!tree->nodes) {
        merkle_tree_free(tree);
        return NULL;
    }
    
    tree->nodes_count = total_nodes;
    
    // Initialize internal nodes with default values
    for (size_t i = 0; i < leaves_count - 1; i++) {
        ops->default_init((char*)tree->nodes + i * ops->item_size);
    }
    
    // Copy leaves
    memcpy((char*)tree->nodes + (leaves_count - 1) * ops->item_size, 
           leaves, leaves_count * ops->item_size);
    
    // Build internal nodes bottom-up
    for (size_t i = leaves_count - 1; i > 0; i--) {
        size_t left_child = (i << 1) + 1 - 1;  // Convert to 0-based indexing
        size_t right_child = (i << 1) + 2 - 1; // Convert to 0-based indexing
        
        if (left_child < total_nodes && right_child < total_nodes) {
            void* left = (char*)tree->nodes + left_child * ops->item_size;
            void* right = (char*)tree->nodes + right_child * ops->item_size;
            void* parent = (char*)tree->nodes + (i - 1) * ops->item_size;
            
            ops->merge(left, right, parent);
        }
    }
    
    return tree;
}

merkle_result_t cbmt_build_merkle_proof(const void* leaves, size_t leaves_count, 
                                        const uint32_t* leaf_indices, size_t indices_count, 
                                        const type_ops_t* ops) {
    merkle_result_t result = {false, {NULL}};
    
    merkle_tree_t* tree = cbmt_build_merkle_tree(leaves, leaves_count, ops);
    if (!tree) return result;
    
    result = merkle_tree_build_proof(tree, leaf_indices, indices_count);
    merkle_tree_free(tree);
    
    return result;
}

merkle_result_t cbmt_retrieve_leaves(const void* leaves, size_t leaves_count, 
                                     const merkle_proof_t* proof, size_t* result_count) {
    merkle_result_t result = {false, {NULL}};
    *result_count = 0;
    
    if (!leaves || !proof || leaves_count == 0 || proof->indices_count == 0) {
        return result;
    }
    
    uint32_t leaves_count_u32 = (uint32_t)leaves_count;
    uint32_t valid_start = leaves_count_u32 - 1;
    uint32_t valid_end = (leaves_count_u32 << 1) - 1;
    
    // Check if all indices are in valid range
    for (size_t i = 0; i < proof->indices_count; i++) {
        if (proof->indices[i] < valid_start || proof->indices[i] >= valid_end) {
            return result;
        }
    }
    
    // Extract leaves
    void* retrieved_leaves = malloc(proof->indices_count * proof->ops.item_size);
    if (!retrieved_leaves) return result;
    
    for (size_t i = 0; i < proof->indices_count; i++) {
        uint32_t leaf_index = proof->indices[i] + 1 - leaves_count_u32;
        const void* leaf = (const char*)leaves + leaf_index * proof->ops.item_size;
        void* dest = (char*)retrieved_leaves + i * proof->ops.item_size;
        proof->ops.copy(leaf, dest);
    }
    
    result.success = true;
    result.data = retrieved_leaves;
    *result_count = proof->indices_count;
    
    return result;
}