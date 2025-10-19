#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Hash size constants
#define HASH_SIZE 32        // SHA256 hash size in bytes
#define HASH_HEX_SIZE 65    // Hex string size (64 chars + null terminator)

// Hash type definition
typedef uint8_t hash_t[HASH_SIZE];

// Forward declarations
typedef struct merkle_tree merkle_tree_t;
typedef struct merkle_proof merkle_proof_t;

// Function pointer type for hash operations
typedef void (*hash_fn_t)(const uint8_t* left, const uint8_t* right, uint8_t* result);

// Hash algorithm information
typedef struct {
    hash_fn_t hash_func;    // Hash function pointer
    char* algo_name;        // Algorithm name (e.g., "sha256", "blake2b")
    size_t hash_size;       // Hash size in bytes (should be HASH_SIZE)
} hash_algo_t;

// Merkle Tree structure
struct merkle_tree {
    hash_t* nodes;          // Array of hash nodes
    size_t nodes_count;     // Number of nodes
    hash_algo_t algo;       // Hash algorithm information
};

// Merkle Proof structure
struct merkle_proof {
    uint32_t* indices;      // Array of indices
    size_t indices_count;   // Number of indices
    hash_t* lemmas;         // Array of hash lemmas
    size_t lemmas_count;    // Number of lemmas
    hash_algo_t algo;       // Hash algorithm information
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
merkle_tree_t* merkle_tree_new(const hash_algo_t* algo);
void merkle_tree_free(merkle_tree_t* tree);
merkle_result_t merkle_tree_build_proof(const merkle_tree_t* tree, const uint32_t* leaf_indices, size_t indices_count);
void merkle_tree_root(const merkle_tree_t* tree, hash_t result);
const hash_t* merkle_tree_nodes(const merkle_tree_t* tree);
size_t merkle_tree_nodes_count(const merkle_tree_t* tree);

// Merkle Proof operations
merkle_proof_t* merkle_proof_new(const uint32_t* indices, size_t indices_count, 
                                 const hash_t* lemmas, size_t lemmas_count, const hash_algo_t* algo);
void merkle_proof_free(merkle_proof_t* proof);
bool merkle_proof_root(const merkle_proof_t* proof, const hash_t* leaves, size_t leaves_count, hash_t result);
bool merkle_proof_verify(const merkle_proof_t* proof, const hash_t root, const hash_t* leaves, size_t leaves_count);
const uint32_t* merkle_proof_indices(const merkle_proof_t* proof);
size_t merkle_proof_indices_count(const merkle_proof_t* proof);
const hash_t* merkle_proof_lemmas(const merkle_proof_t* proof);
size_t merkle_proof_lemmas_count(const merkle_proof_t* proof);

// CBMT operations
void cbmt_build_merkle_root(const hash_t* leaves, size_t leaves_count, const hash_algo_t* algo, hash_t result);
merkle_tree_t* cbmt_build_merkle_tree(const hash_t* leaves, size_t leaves_count, const hash_algo_t* algo);
merkle_result_t cbmt_build_merkle_proof(const hash_t* leaves, size_t leaves_count, 
                                        const uint32_t* leaf_indices, size_t indices_count, 
                                        const hash_algo_t* algo);
merkle_result_t cbmt_retrieve_leaves(const hash_t* leaves, size_t leaves_count, 
                                     const merkle_proof_t* proof, size_t* result_count);

// Hash utility functions
void hash_copy(const hash_t src, hash_t dst);
int hash_compare(const hash_t a, const hash_t b);
void hash_zero(hash_t hash);
void hash_to_hex(const hash_t hash, char* hex_str);
bool hash_from_hex(const char* hex_str, hash_t hash);

// Built-in hash algorithms
extern const hash_algo_t sha256_algo;
void sha256_hash(const uint8_t* left, const uint8_t* right, uint8_t* result);

#endif // MERKLE_TREE_H