#ifndef MERKLE_TREE_CBOR_H
#define MERKLE_TREE_CBOR_H

#ifdef WITH_CBOR

#include "merkle_tree.h"
#include <cbor.h>

// CBOR serialization functions for Merkle Tree
cbor_item_t* merkle_tree_to_cbor(const merkle_tree_t* tree);
merkle_tree_t* merkle_tree_from_cbor(cbor_item_t* item);

// CBOR serialization functions for Merkle Proof
cbor_item_t* merkle_proof_to_cbor(const merkle_proof_t* proof);
merkle_proof_t* merkle_proof_from_cbor(cbor_item_t* item);

// Convenience functions for direct byte array operations
typedef struct {
    uint8_t* data;
    size_t size;
    bool success;
} cbor_buffer_t;

// Serialize to byte buffer
cbor_buffer_t merkle_tree_to_cbor_buffer(const merkle_tree_t* tree);
cbor_buffer_t merkle_proof_to_cbor_buffer(const merkle_proof_t* proof);

// Deserialize from byte buffer
merkle_tree_t* merkle_tree_from_cbor_buffer(const uint8_t* buffer, size_t size);
merkle_proof_t* merkle_proof_from_cbor_buffer(const uint8_t* buffer, size_t size);

// Free buffer allocated by *_to_cbor_buffer functions
void cbor_buffer_free(cbor_buffer_t* buffer);

// Utility functions
cbor_item_t* hash_to_cbor(const hash_t hash);
bool cbor_to_hash(cbor_item_t* item, hash_t hash);
cbor_item_t* hash_array_to_cbor(const hash_t* hashes, size_t count);
hash_t* cbor_to_hash_array(cbor_item_t* item, size_t* count);

// CBOR schema information
#define CBOR_MERKLE_TREE_VERSION 1
#define CBOR_MERKLE_PROOF_VERSION 1

// CBOR map keys (using integers for efficiency)
enum cbor_merkle_keys {
    CBOR_KEY_VERSION = 1,
    CBOR_KEY_NODES = 2,
    CBOR_KEY_NODES_COUNT = 3,
    CBOR_KEY_ALGORITHM = 4,
    CBOR_KEY_HASH_SIZE = 5,
    CBOR_KEY_INDICES = 6,
    CBOR_KEY_LEMMAS = 7,
    CBOR_KEY_INDICES_COUNT = 8,
    CBOR_KEY_LEMMAS_COUNT = 9
};

#endif // WITH_CBOR

#endif // MERKLE_TREE_CBOR_H