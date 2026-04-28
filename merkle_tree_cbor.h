#ifndef MERKLE_TREE_CBOR_H
#define MERKLE_TREE_CBOR_H

#ifdef WITH_CBOR

#include "merkle_tree.h"

// QCBOR-based encode: no heap allocation, writes into caller-supplied buffer.
// Returns encoded byte count, or 0 on error.
// Upper bound for buf_size: ~64 + nodes_count * 34 bytes for tree;
//                           ~64 + lemmas_count * 34 bytes for proof.
size_t secure_merkle_tree_encode(const secure_merkle_tree_t *tree,
                                 uint8_t *buf, size_t buf_size);
size_t secure_merkle_proof_encode(const secure_merkle_proof_t *proof,
                                  uint8_t *buf, size_t buf_size);

// QCBOR-based decode: parses a standalone CBOR map buffer into a
// heap-allocated secure_merkle_tree_t. Caller must call
// secure_merkle_tree_free() on the returned pointer.
// Returns NULL on any parse or allocation error.
secure_merkle_tree_t *secure_merkle_tree_decode(const uint8_t *buf,
                                                size_t buf_size);

// CBOR schema information
#define CBOR_MERKLE_TREE_VERSION 1                 // Standard tree version
#define CBOR_MERKLE_TREE_VERSION_SECURE 2          // Secure tree version (tree_depth mandatory, security_flags optional)
#define CBOR_MERKLE_PROOF_VERSION_LEGACY 1         // Legacy proof version (without expected_depth)
#define CBOR_MERKLE_PROOF_VERSION_SECURE 2         // Secure proof version (with optional expected_depth)

// CBOR map keys (using integers for efficiency)
// Merkle tree and merkle proof use overlapping key numbers (1-5) but as separate CBOR objects
// Secure variants use additional keys (6-8) which are optional in unified structures
enum cbor_merkle_keys {
    CBOR_KEY_VERSION = 1,                  // Version field (both tree and proof)
    CBOR_KEY_NODES = 2,                    // Merkle tree: nodes array
    CBOR_KEY_INDICES = 2,                  // Merkle proof: indices array (reused)
    CBOR_KEY_NODES_COUNT = 3,              // Merkle tree: nodes count
    CBOR_KEY_LEMMAS = 3,                   // Merkle proof: lemmas array (reused)
    CBOR_KEY_ALGORITHM = 4,                // Merkle tree: algorithm name
    CBOR_KEY_INDICES_COUNT = 4,            // Merkle proof: indices count (reused)
    CBOR_KEY_HASH_SIZE = 5,                // Merkle tree: hash size
    CBOR_KEY_LEMMAS_COUNT = 5,             // Merkle proof: lemmas count (reused)
    
    // Keys for secure versions (tree_depth is mandatory)
    CBOR_KEY_TREE_DEPTH = 6,               // Mandatory: Tree depth for security validation
    CBOR_KEY_SECURITY_FLAGS = 7,           // Optional: Security configuration flags
    CBOR_KEY_EXPECTED_DEPTH = 8            // Optional: Expected depth for secure proofs
};

#endif // WITH_CBOR

#endif // MERKLE_TREE_CBOR_H