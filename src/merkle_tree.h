#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct merkle_tree merkle_tree_t;
typedef struct merkle_proof merkle_proof_t;

// Function pointer type for merge operations
typedef void (*merge_fn_t)(const void* left, const void* right, void* result);

// Function pointer type for comparison operations
typedef int (*compare_fn_t)(const void* a, const void* b);

// Function pointer type for copy operations
typedef void (*copy_fn_t)(const void* src, void* dst);

// Function pointer type for default value initialization
typedef void (*default_fn_t)(void* item);

// Structure to hold type-specific operations
typedef struct {
    merge_fn_t merge;
    compare_fn_t compare;
    copy_fn_t copy;
    default_fn_t default_init;
    size_t item_size;
} type_ops_t;

// Merkle Tree structure
struct merkle_tree {
    void* nodes;           // Array of nodes
    size_t nodes_count;    // Number of nodes
    type_ops_t ops;        // Type operations
};

// Merkle Proof structure
struct merkle_proof {
    uint32_t* indices;     // Array of indices
    size_t indices_count;  // Number of indices
    void* lemmas;          // Array of lemmas
    size_t lemmas_count;   // Number of lemmas
    type_ops_t ops;        // Type operations
};

// Result structure for operations that may fail
typedef struct {
    bool success;
    union {
        merkle_tree_t* tree;
        merkle_proof_t* proof;
        void* data;
    };
} merkle_result_t;

// Tree index operations
uint32_t tree_index_sibling(uint32_t index);
uint32_t tree_index_parent(uint32_t index);
bool tree_index_is_left(uint32_t index);

// Merkle Tree operations
merkle_tree_t* merkle_tree_new(const type_ops_t* ops);
void merkle_tree_free(merkle_tree_t* tree);
merkle_result_t merkle_tree_build_proof(const merkle_tree_t* tree, const uint32_t* leaf_indices, size_t indices_count);
void merkle_tree_root(const merkle_tree_t* tree, void* result);
const void* merkle_tree_nodes(const merkle_tree_t* tree);
size_t merkle_tree_nodes_count(const merkle_tree_t* tree);

// Merkle Proof operations
merkle_proof_t* merkle_proof_new(const uint32_t* indices, size_t indices_count, 
                                 const void* lemmas, size_t lemmas_count, const type_ops_t* ops);
void merkle_proof_free(merkle_proof_t* proof);
bool merkle_proof_root(const merkle_proof_t* proof, const void* leaves, size_t leaves_count, void* result);
bool merkle_proof_verify(const merkle_proof_t* proof, const void* root, const void* leaves, size_t leaves_count);
const uint32_t* merkle_proof_indices(const merkle_proof_t* proof);
size_t merkle_proof_indices_count(const merkle_proof_t* proof);
const void* merkle_proof_lemmas(const merkle_proof_t* proof);
size_t merkle_proof_lemmas_count(const merkle_proof_t* proof);

// CBMT operations
void cbmt_build_merkle_root(const void* leaves, size_t leaves_count, const type_ops_t* ops, void* result);
merkle_tree_t* cbmt_build_merkle_tree(const void* leaves, size_t leaves_count, const type_ops_t* ops);
merkle_result_t cbmt_build_merkle_proof(const void* leaves, size_t leaves_count, 
                                        const uint32_t* leaf_indices, size_t indices_count, 
                                        const type_ops_t* ops);
merkle_result_t cbmt_retrieve_leaves(const void* leaves, size_t leaves_count, 
                                     const merkle_proof_t* proof, size_t* result_count);

// Utility functions for common types
void merge_int32(const void* left, const void* right, void* result);
int compare_int32(const void* a, const void* b);
void copy_int32(const void* src, void* dst);
void default_int32(void* item);

// Type operations for int32_t
extern const type_ops_t int32_ops;

#endif // MERKLE_TREE_H