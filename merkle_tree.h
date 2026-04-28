#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Hash size constants
#define HASH_SIZE 32        // SHA256 hash size in bytes
#define HASH_HEX_SIZE 65    // Hex string size (64 chars + null terminator)

// Security constants for attack mitigation
#define LEAF_PREFIX 0x00        // Prefix for leaf nodes
#define INTERNAL_PREFIX 0x01    // Prefix for internal nodes
#define MAX_TREE_DEPTH 32       // Maximum allowed tree depth

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

// Enhanced hash algorithm with security features
typedef struct {
    hash_fn_t hash_func;        // Base hash function pointer
    char* algo_name;            // Algorithm name
    size_t hash_size;           // Hash size in bytes
    bool use_double_leaf_hash;  // Enable double hashing for leaves
    bool use_depth_prefix;      // Enable depth prefixing
    bool use_node_prefix;       // Enable leaf/internal prefixing
} secure_hash_algo_t;

// Merkle Tree structure
struct merkle_tree {
    hash_t* nodes;          // Array of hash nodes
    size_t nodes_count;     // Number of nodes
    hash_algo_t* hash_algo;       // Hash algorithm information
};

// Enhanced Merkle Tree structure with security
// Note: Uses unified CBOR serialization structure with version field
// Version 1: Standard tree (tree_depth and security_flags omitted)
// Version 2: Secure tree (tree_depth and security_flags included as optional fields)
typedef struct {
    hash_t* nodes;              // Array of hash nodes
    size_t nodes_count;         // Number of nodes
    uint8_t tree_depth;         // Tree depth for validation (optional in CBOR)
    secure_hash_algo_t* algo;   // Enhanced hash algorithm
    bool security_enabled;      // Security features enabled
} secure_merkle_tree_t;

// Merkle Proof structure
struct merkle_proof {
    // Indices are CBMT *node indices* into the implicit array-based tree layout
    // (root at 0, leaves start at leaves_count-1).
    // NOTE: These are NOT leaf-layer indices.
    uint32_t* indices;      // Array of CBMT node indices
    size_t indices_count;   // Number of indices
    hash_t* lemmas;         // Array of hash lemmas
    size_t lemmas_count;    // Number of lemmas
    hash_algo_t* hash_algo;       // Hash algorithm information
};

// Enhanced Merkle Proof structure with security
// Note: Uses unified CBOR serialization structure with version field and optional expected_depth
// Version 1: Legacy proof (expected_depth field omitted in CBOR)
// Version 2+: Secure proof (expected_depth field included for depth validation)
typedef struct {
    // Indices are *leaf-layer indices* (0..2^tree_depth-1) for secure single-leaf proofs.
    // NOTE: This differs from legacy proofs which use CBMT node indices.
    uint32_t* indices;          // Array of leaf-layer indices
    size_t indices_count;       // Number of indices
    hash_t* lemmas;             // Array of hash lemmas
    size_t lemmas_count;        // Number of lemmas
    // For secure proofs this must match the tree depth; the verifier expects
    // lemmas_count == expected_depth.
    uint8_t expected_depth;     // Expected tree depth for validation
    secure_hash_algo_t* algo;   // Enhanced hash algorithm
} secure_merkle_proof_t;

// CBOR Serialization Notes:
// The secure_merkle_tree_encode, secure_merkle_proof_encode, and
// secure_merkle_tree_decode functions use the unified CBOR structures
// defined in stardome-merkle-tree.cddl with optional security fields.

// Result structure for operations that may fail
typedef struct {
    bool success;
    union {
        merkle_tree_t* tree;
        merkle_proof_t* proof;
        secure_merkle_tree_t* secure_tree;
        secure_merkle_proof_t* secure_proof;
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
// Single-leaf verification helper.
// Expects a proof generated for exactly one leaf (proof->indices_count == 1).
// The leaf value provided here corresponds to proof->indices[0].
bool merkle_proof_verify_single(const merkle_proof_t* proof, const hash_t root, const hash_t leaf);
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

// Security-enhanced hash functions
void secure_leaf_hash(const uint8_t* data, size_t data_len, uint8_t depth, 
                     const secure_hash_algo_t* algo, uint8_t* result);
void secure_internal_hash(const uint8_t* left, const uint8_t* right, uint8_t depth,
                         const secure_hash_algo_t* algo, uint8_t* result);

// Secure Merkle Tree operations
secure_merkle_tree_t* secure_merkle_tree_new(const secure_hash_algo_t* algo);
void secure_merkle_tree_free(secure_merkle_tree_t* tree);
merkle_result_t secure_cbmt_build_merkle_tree(const hash_t* leaves, size_t leaves_count, 
                                             const secure_hash_algo_t* algo);
merkle_result_t secure_merkle_tree_build_proof(const secure_merkle_tree_t* tree, 
                                              const uint32_t* leaf_indices, size_t indices_count);

// Secure Merkle Proof operations
// Note: Uses unified CBOR structure with optional expected_depth field
// Version 1 (legacy): without expected_depth
// Version 2+ (secure): expected_depth is required for strict verification
secure_merkle_proof_t* secure_merkle_proof_new(const uint32_t* indices, size_t indices_count,
                                              const hash_t* lemmas, size_t lemmas_count,
                                              uint8_t expected_depth, const secure_hash_algo_t* algo);
void secure_merkle_proof_free(secure_merkle_proof_t* proof);
bool secure_merkle_proof_verify(const secure_merkle_proof_t* proof, const hash_t root, 
                               const hash_t* leaves, size_t leaves_count);
// Single-leaf verification helper.
// Expects a proof generated for exactly one leaf (proof->indices_count == 1).
// The leaf value provided here corresponds to proof->indices[0].
bool secure_merkle_proof_verify_single(const secure_merkle_proof_t* proof, const hash_t root,
                                      const hash_t leaf);

// Built-in secure hash algorithms
extern const secure_hash_algo_t secure_sha256_max;      // Maximum security
extern const secure_hash_algo_t secure_sha256_moderate; // Moderate security

#endif // MERKLE_TREE_H