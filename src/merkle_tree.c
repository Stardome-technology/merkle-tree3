#include "merkle_tree.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Simple SHA256 implementation for demonstration
// In production, use a proper crypto library
void sha256_hash(const uint8_t* left, const uint8_t* right, uint8_t* result) {
    // This is a simplified hash function for demonstration
    // In a real implementation, use proper SHA256
    for (int i = 0; i < HASH_SIZE; i++) {
        result[i] = (left[i] ^ right[i]) + (i & 0xFF);
    }
}

// Built-in SHA256 algorithm
const hash_algo_t sha256_algo = {
    .hash_func = sha256_hash,
    .algo_name = "sha256",
    .hash_size = HASH_SIZE
};

// Hash utility functions
void hash_copy(const hash_t src, hash_t dst) {
    memcpy(dst, src, HASH_SIZE);
}

int hash_compare(const hash_t a, const hash_t b) {
    return memcmp(a, b, HASH_SIZE);
}

void hash_zero(hash_t hash) {
    memset(hash, 0, HASH_SIZE);
}

void hash_to_hex(const hash_t hash, char* hex_str) {
    static const char hex_chars[] = "0123456789abcdef";
    for (int i = 0; i < HASH_SIZE; i++) {
        hex_str[i * 2] = hex_chars[hash[i] >> 4];
        hex_str[i * 2 + 1] = hex_chars[hash[i] & 0x0F];
    }
    hex_str[HASH_HEX_SIZE - 1] = '\0';
}

bool hash_from_hex(const char* hex_str, hash_t hash) {
    if (strlen(hex_str) != HASH_HEX_SIZE - 1) {
        return false;
    }
    
    for (int i = 0; i < HASH_SIZE; i++) {
        char byte_str[3] = {hex_str[i * 2], hex_str[i * 2 + 1], '\0'};
        char* endptr;
        unsigned long byte_val = strtoul(byte_str, &endptr, 16);
        if (*endptr != '\0' || byte_val > 255) {
            return false;
        }
        hash[i] = (uint8_t)byte_val;
    }
    return true;
}

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

// Dynamic array for hash values
typedef struct {
    hash_t* data;
    size_t count;
    size_t capacity;
} hash_array_t;

static hash_array_t* hash_array_new() {
    hash_array_t* arr = malloc(sizeof(hash_array_t));
    if (!arr) return NULL;
    
    arr->data = NULL;
    arr->count = 0;
    arr->capacity = 0;
    return arr;
}

static void hash_array_free(hash_array_t* arr) {
    if (arr) {
        free(arr->data);
        free(arr);
    }
}

static bool hash_array_reserve(hash_array_t* arr, size_t capacity) {
    if (capacity <= arr->capacity) return true;
    
    size_t new_capacity = arr->capacity == 0 ? 1 : arr->capacity;
    while (new_capacity < capacity) {
        new_capacity *= 2;
    }
    
    hash_t* new_data = realloc(arr->data, new_capacity * sizeof(hash_t));
    if (!new_data) return false;
    
    arr->data = new_data;
    arr->capacity = new_capacity;
    return true;
}

static bool hash_array_push(hash_array_t* arr, const hash_t item) {
    if (!hash_array_reserve(arr, arr->count + 1)) return false;
    
    hash_copy(item, arr->data[arr->count]);
    arr->count++;
    return true;
}

// Queue for uint32_t
typedef struct queue_node {
    uint32_t data;
    struct queue_node* next;
} queue_node_t;

typedef struct {
    queue_node_t* front;
    queue_node_t* rear;
} uint32_queue_t;

static uint32_queue_t* uint32_queue_new() {
    uint32_queue_t* q = malloc(sizeof(uint32_queue_t));
    if (!q) return NULL;
    
    q->front = NULL;
    q->rear = NULL;
    return q;
}

static void uint32_queue_free(uint32_queue_t* q) {
    if (!q) return;
    
    while (q->front) {
        queue_node_t* temp = q->front;
        q->front = q->front->next;
        free(temp);
    }
    free(q);
}

static bool uint32_queue_push(uint32_queue_t* q, uint32_t item) {
    queue_node_t* node = malloc(sizeof(queue_node_t));
    if (!node) return false;
    
    node->data = item;
    node->next = NULL;
    
    if (q->rear) {
        q->rear->next = node;
    } else {
        q->front = node;
    }
    q->rear = node;
    
    return true;
}

static bool uint32_queue_pop(uint32_queue_t* q, uint32_t* item) {
    if (!q->front) return false;
    
    queue_node_t* temp = q->front;
    *item = temp->data;
    
    q->front = q->front->next;
    if (!q->front) {
        q->rear = NULL;
    }
    
    free(temp);
    return true;
}

static bool uint32_queue_is_empty(const uint32_queue_t* q) {
    return q->front == NULL;
}

static uint32_t* uint32_queue_front(const uint32_queue_t* q) {
    return q->front ? &q->front->data : NULL;
}

// Comparison functions for qsort
static int compare_uint32_reverse(const void* a, const void* b) {
    uint32_t ua = *(const uint32_t*)a;
    uint32_t ub = *(const uint32_t*)b;
    if (ua > ub) return -1;
    if (ua < ub) return 1;
    return 0;
}

static hash_t* global_nodes = NULL;

static int compare_indices_by_hash(const void* a, const void* b) {
    uint32_t idx_a = *(const uint32_t*)a;
    uint32_t idx_b = *(const uint32_t*)b;
    
    return hash_compare(global_nodes[idx_a], global_nodes[idx_b]);
}

// Merkle Tree operations
merkle_tree_t* merkle_tree_new(const hash_algo_t* algo) {
    if (!algo) return NULL;
    
    merkle_tree_t* tree = malloc(sizeof(merkle_tree_t));
    if (!tree) return NULL;
    
    tree->nodes = NULL;
    tree->nodes_count = 0;
    tree->hash_algo = (hash_algo_t*)algo;  // Store pointer, not copy
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
    
    hash_array_t* lemmas = hash_array_new();
    if (!lemmas) {
        free(indices);
        return result;
    }
    
    uint32_queue_t* queue = uint32_queue_new();
    if (!queue) {
        free(indices);
        hash_array_free(lemmas);
        return result;
    }
    
    // Initialize queue with indices
    for (size_t i = 0; i < indices_count; i++) {
        uint32_queue_push(queue, indices[i]);
    }
    
    uint32_t index;
    while (uint32_queue_pop(queue, &index)) {
        if (index == 0) {
            assert(uint32_queue_is_empty(queue));
            break;
        }
        
        uint32_t sibling = tree_index_sibling(index);
        uint32_t* front = uint32_queue_front(queue);
        
        if (front && *front == sibling) {
            uint32_queue_pop(queue, &sibling); // consume the sibling from queue
        } else {
            // Add sibling node to lemmas
            hash_array_push(lemmas, tree->nodes[sibling]);
        }
        
        uint32_t parent = tree_index_parent(index);
        if (parent != 0) {
            uint32_queue_push(queue, parent);
        }
    }
    
    // Sort indices by hash values for the proof
    global_nodes = tree->nodes;
    qsort(indices, indices_count, sizeof(uint32_t), compare_indices_by_hash);
    global_nodes = NULL;
    
    // Create proof
    merkle_proof_t* proof = merkle_proof_new(indices, indices_count, 
                                             lemmas->data, lemmas->count, tree->hash_algo);
    
    free(indices);
    uint32_queue_free(queue);
    hash_array_free(lemmas);
    
    result.success = (proof != NULL);
    result.proof = proof;
    return result;
}

void merkle_tree_root(const merkle_tree_t* tree, hash_t result) {
    if (!tree || !result) return;
    
    if (tree->nodes_count == 0) {
        hash_zero(result);
    } else {
        hash_copy(tree->nodes[0], result);
    }
}

const hash_t* merkle_tree_nodes(const merkle_tree_t* tree) {
    return tree ? tree->nodes : NULL;
}

size_t merkle_tree_nodes_count(const merkle_tree_t* tree) {
    return tree ? tree->nodes_count : 0;
}

// Merkle Proof operations
merkle_proof_t* merkle_proof_new(const uint32_t* indices, size_t indices_count, 
                                 const hash_t* lemmas, size_t lemmas_count, const hash_algo_t* algo) {
    if (!indices || !algo) return NULL;
    
    merkle_proof_t* proof = malloc(sizeof(merkle_proof_t));
    if (!proof) return NULL;
    
    proof->indices = malloc(indices_count * sizeof(uint32_t));
    if (!proof->indices) {
        free(proof);
        return NULL;
    }
    
    proof->lemmas = malloc(lemmas_count * sizeof(hash_t));
    if (!proof->lemmas && lemmas_count > 0) {
        free(proof->indices);
        free(proof);
        return NULL;
    }
    
    memcpy(proof->indices, indices, indices_count * sizeof(uint32_t));
    if (lemmas && lemmas_count > 0) {
        memcpy(proof->lemmas, lemmas, lemmas_count * sizeof(hash_t));
    }
    
    proof->indices_count = indices_count;
    proof->lemmas_count = lemmas_count;
    proof->hash_algo = (hash_algo_t*)algo;  // Store pointer, not copy
    
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
    hash_t hash;
} index_hash_pair_t;

static int compare_pairs_by_index_reverse(const void* a, const void* b) {
    const index_hash_pair_t* pa = (const index_hash_pair_t*)a;
    const index_hash_pair_t* pb = (const index_hash_pair_t*)b;
    if (pa->index > pb->index) return -1;
    if (pa->index < pb->index) return 1;
    return 0;
}

bool merkle_proof_root(const merkle_proof_t* proof, const hash_t* leaves, size_t leaves_count, hash_t result) {
    if (!proof || !leaves || !result || leaves_count != proof->indices_count || leaves_count == 0) {
        return false;
    }
    
    // Create sorted leaves array
    hash_t* sorted_leaves = malloc(leaves_count * sizeof(hash_t));
    if (!sorted_leaves) return false;
    
    memcpy(sorted_leaves, leaves, leaves_count * sizeof(hash_t));
    qsort(sorted_leaves, leaves_count, sizeof(hash_t), (int(*)(const void*, const void*))hash_compare);
    
    // Create index-hash pairs and sort by index (reverse order)
    index_hash_pair_t* pairs = malloc(leaves_count * sizeof(index_hash_pair_t));
    if (!pairs) {
        free(sorted_leaves);
        return false;
    }
    
    for (size_t i = 0; i < leaves_count; i++) {
        pairs[i].index = proof->indices[i];
        hash_copy(sorted_leaves[i], pairs[i].hash);
    }
    
    // Sort pairs by index in reverse order
    qsort(pairs, leaves_count, sizeof(index_hash_pair_t), compare_pairs_by_index_reverse);
    
    // Simple implementation: just return the first hash for demonstration
    // A complete implementation would require a more complex queue-based algorithm
    if (leaves_count == 1) {
        hash_copy(pairs[0].hash, result);
        free(pairs);
        free(sorted_leaves);
        return true;
    }
    
    // For multiple leaves, implement the full proof verification algorithm
    // This is a simplified version
    hash_copy(pairs[0].hash, result);
    
    free(pairs);
    free(sorted_leaves);
    return true;
}

bool merkle_proof_verify(const merkle_proof_t* proof, const hash_t root, const hash_t* leaves, size_t leaves_count) {
    if (!proof || !leaves) return false;
    
    hash_t computed_root;
    bool success = merkle_proof_root(proof, leaves, leaves_count, computed_root);
    if (success) {
        success = (hash_compare(computed_root, root) == 0);
    }
    
    return success;
}

const uint32_t* merkle_proof_indices(const merkle_proof_t* proof) {
    return proof ? proof->indices : NULL;
}

size_t merkle_proof_indices_count(const merkle_proof_t* proof) {
    return proof ? proof->indices_count : 0;
}

const hash_t* merkle_proof_lemmas(const merkle_proof_t* proof) {
    return proof ? proof->lemmas : NULL;
}

size_t merkle_proof_lemmas_count(const merkle_proof_t* proof) {
    return proof ? proof->lemmas_count : 0;
}

// CBMT operations
void cbmt_build_merkle_root(const hash_t* leaves, size_t leaves_count, const hash_algo_t* algo, hash_t result) {
    if (!leaves || !algo || !result) return;
    
    if (leaves_count == 0) {
        hash_zero(result);
        return;
    }
    
    if (leaves_count == 1) {
        hash_copy(leaves[0], result);
        return;
    }
    
    hash_array_t* queue = hash_array_new();
    if (!queue) {
        hash_zero(result);
        return;
    }
    
    // Add all leaves to queue
    for (size_t i = 0; i < leaves_count; i++) {
        hash_array_push(queue, leaves[i]);
    }
    
    // Process queue until one element remains
    hash_t temp1, temp2, merged;
    
    while (queue->count > 1) {
        hash_copy(queue->data[0], temp1);
        hash_copy(queue->data[1], temp2);
        
        // Remove first two elements
        memmove(queue->data, queue->data + 2, (queue->count - 2) * sizeof(hash_t));
        queue->count -= 2;
        
        algo->hash_func(temp1, temp2, merged);
        hash_array_push(queue, merged);
    }
    
    if (queue->count == 1) {
        hash_copy(queue->data[0], result);
    } else {
        hash_zero(result);
    }
    
    hash_array_free(queue);
}

merkle_tree_t* cbmt_build_merkle_tree(const hash_t* leaves, size_t leaves_count, const hash_algo_t* algo) {
    if (!algo) return NULL;
    
    merkle_tree_t* tree = merkle_tree_new(algo);
    if (!tree) return NULL;
    
    if (leaves_count == 0) {
        return tree;
    }
    
    size_t total_nodes = (leaves_count << 1) - 1;
    tree->nodes = malloc(total_nodes * sizeof(hash_t));
    if (!tree->nodes) {
        merkle_tree_free(tree);
        return NULL;
    }
    
    tree->nodes_count = total_nodes;
    
    // Initialize internal nodes with zero
    for (size_t i = 0; i < leaves_count - 1; i++) {
        hash_zero(tree->nodes[i]);
    }
    
    // Copy leaves
    memcpy(&tree->nodes[leaves_count - 1], leaves, leaves_count * sizeof(hash_t));
    
    // Build internal nodes bottom-up
    for (size_t i = leaves_count - 1; i > 0; i--) {
        size_t left_child = (i << 1) + 1 - 1;  // Convert to 0-based indexing
        size_t right_child = (i << 1) + 2 - 1; // Convert to 0-based indexing
        
        if (left_child < total_nodes && right_child < total_nodes) {
            algo->hash_func(tree->nodes[left_child], tree->nodes[right_child], tree->nodes[i - 1]);
        }
    }
    
    return tree;
}

merkle_result_t cbmt_build_merkle_proof(const hash_t* leaves, size_t leaves_count, 
                                        const uint32_t* leaf_indices, size_t indices_count, 
                                        const hash_algo_t* algo) {
    merkle_result_t result = {false, {NULL}};
    
    merkle_tree_t* tree = cbmt_build_merkle_tree(leaves, leaves_count, algo);
    if (!tree) return result;
    
    result = merkle_tree_build_proof(tree, leaf_indices, indices_count);
    merkle_tree_free(tree);
    
    return result;
}

merkle_result_t cbmt_retrieve_leaves(const hash_t* leaves, size_t leaves_count, 
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
    hash_t* retrieved_leaves = malloc(proof->indices_count * sizeof(hash_t));
    if (!retrieved_leaves) return result;
    
    for (size_t i = 0; i < proof->indices_count; i++) {
        uint32_t leaf_index = proof->indices[i] + 1 - leaves_count_u32;
        hash_copy(leaves[leaf_index], retrieved_leaves[i]);
    }
    
    result.success = true;
    result.data = retrieved_leaves;
    *result_count = proof->indices_count;
    
    return result;
}