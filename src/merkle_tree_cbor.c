#ifdef WITH_CBOR

#include "merkle_tree_cbor.h"
#include <stdlib.h>
#include <string.h>

// Convert hash to CBOR byte string
cbor_item_t* hash_to_cbor(const hash_t hash) {
    return cbor_build_bytestring((const unsigned char*)hash, HASH_SIZE);
}

// Convert CBOR byte string to hash
bool cbor_to_hash(cbor_item_t* item, hash_t hash) {
    if (!cbor_isa_bytestring(item)) {
        return false;
    }
    
    size_t size = cbor_bytestring_length(item);
    if (size != HASH_SIZE) {
        return false;
    }
    
    unsigned char* data = cbor_bytestring_handle(item);
    memcpy(hash, data, HASH_SIZE);
    return true;
}

// Convert hash array to CBOR array
cbor_item_t* hash_array_to_cbor(const hash_t* hashes, size_t count) {
    cbor_item_t* array = cbor_new_definite_array(count);
    if (!array) return NULL;
    
    for (size_t i = 0; i < count; i++) {
        cbor_item_t* hash_item = hash_to_cbor(hashes[i]);
        if (!hash_item) {
            cbor_decref(&array);
            return NULL;
        }
        if (!cbor_array_push(array, hash_item)) {
            cbor_decref(&hash_item);
            cbor_decref(&array);
            return NULL;
        }
        cbor_decref(&hash_item);
    }
    
    return array;
}

// Convert CBOR array to hash array
hash_t* cbor_to_hash_array(cbor_item_t* item, size_t* count) {
    if (!cbor_isa_array(item)) {
        return NULL;
    }
    
    *count = cbor_array_size(item);
    if (*count == 0) {
        return NULL;
    }
    
    hash_t* hashes = malloc(*count * sizeof(hash_t));
    if (!hashes) {
        return NULL;
    }
    
    for (size_t i = 0; i < *count; i++) {
        cbor_item_t* hash_item = cbor_array_get(item, i);
        if (!hash_item || !cbor_to_hash(hash_item, hashes[i])) {
            free(hashes);
            if (hash_item) cbor_decref(&hash_item);
            return NULL;
        }
        cbor_decref(&hash_item);
    }
    
    return hashes;
}

// Serialize merkle tree to CBOR
cbor_item_t* merkle_tree_to_cbor(const merkle_tree_t* tree) {
    if (!tree) return NULL;
    
    cbor_item_t* map = cbor_new_definite_map(5);
    if (!map) return NULL;
    
    // Version
    cbor_item_t* version_key = cbor_build_uint8(CBOR_KEY_VERSION);
    cbor_item_t* version_val = cbor_build_uint8(CBOR_MERKLE_TREE_VERSION);
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = version_key, .value = version_val
    })) {
        cbor_decref(&map);
        cbor_decref(&version_key);
        cbor_decref(&version_val);
        return NULL;
    }
    
    // Nodes count
    cbor_item_t* nodes_count_key = cbor_build_uint8(CBOR_KEY_NODES_COUNT);
    cbor_item_t* nodes_count_val = cbor_build_uint32(tree->nodes_count);
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = nodes_count_key, .value = nodes_count_val
    })) {
        cbor_decref(&map);
        return NULL;
    }
    
    // Nodes array
    cbor_item_t* nodes_key = cbor_build_uint8(CBOR_KEY_NODES);
    cbor_item_t* nodes_val = hash_array_to_cbor(tree->nodes, tree->nodes_count);
    if (!nodes_val || !cbor_map_add(map, (struct cbor_pair) {
        .key = nodes_key, .value = nodes_val
    })) {
        cbor_decref(&map);
        if (nodes_val) cbor_decref(&nodes_val);
        return NULL;
    }
    
    // Algorithm name
    cbor_item_t* algo_key = cbor_build_uint8(CBOR_KEY_ALGORITHM);
    const char* algo_name = tree->hash_algo ? tree->hash_algo->algo_name : "unknown";
    cbor_item_t* algo_val = cbor_build_string(algo_name);
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = algo_key, .value = algo_val
    })) {
        cbor_decref(&map);
        return NULL;
    }
    
    // Hash size
    cbor_item_t* hash_size_key = cbor_build_uint8(CBOR_KEY_HASH_SIZE);
    uint32_t hash_size = tree->hash_algo ? tree->hash_algo->hash_size : HASH_SIZE;
    cbor_item_t* hash_size_val = cbor_build_uint32(hash_size);
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = hash_size_key, .value = hash_size_val
    })) {
        cbor_decref(&map);
        return NULL;
    }
    
    return map;
}

// Deserialize merkle tree from CBOR
merkle_tree_t* merkle_tree_from_cbor(cbor_item_t* item) {
    if (!cbor_isa_map(item)) {
        return NULL;
    }
    
    merkle_tree_t* tree = malloc(sizeof(merkle_tree_t));
    if (!tree) return NULL;
    
    memset(tree, 0, sizeof(merkle_tree_t));
    
    // Extract nodes count
    cbor_item_t* nodes_count_key = cbor_build_uint8(CBOR_KEY_NODES_COUNT);
    cbor_item_t* nodes_count_val = cbor_map_get(item, nodes_count_key);
    cbor_decref(&nodes_count_key);
    
    if (!nodes_count_val || !cbor_isa_uint(nodes_count_val)) {
        free(tree);
        if (nodes_count_val) cbor_decref(&nodes_count_val);
        return NULL;
    }
    
    tree->nodes_count = cbor_get_uint32(nodes_count_val);
    cbor_decref(&nodes_count_val);
    
    // Extract nodes array
    cbor_item_t* nodes_key = cbor_build_uint8(CBOR_KEY_NODES);
    cbor_item_t* nodes_val = cbor_map_get(item, nodes_key);
    cbor_decref(&nodes_key);
    
    if (!nodes_val) {
        free(tree);
        return NULL;
    }
    
    size_t nodes_array_count;
    tree->nodes = cbor_to_hash_array(nodes_val, &nodes_array_count);
    cbor_decref(&nodes_val);
    
    if (!tree->nodes || nodes_array_count != tree->nodes_count) {
        free(tree->nodes);
        free(tree);
        return NULL;
    }
    
    // Extract algorithm name (optional)
    cbor_item_t* algo_key = cbor_build_uint8(CBOR_KEY_ALGORITHM);
    cbor_item_t* algo_val = cbor_map_get(item, algo_key);
    cbor_decref(&algo_key);
    
    if (algo_val && cbor_isa_string(algo_val)) {
        size_t algo_len = cbor_string_length(algo_val);
        char* algo_name = malloc(algo_len + 1);
        if (algo_name) {
            memcpy(algo_name, cbor_string_handle(algo_val), algo_len);
            algo_name[algo_len] = '\0';
            
            // For simplicity, assume SHA256 for now
            // In a full implementation, you'd match against known algorithms
            if (strcmp(algo_name, "sha256") == 0) {
                tree->hash_algo = &sha256_algo;
            }
            free(algo_name);
        }
        cbor_decref(&algo_val);
    }
    
    if (!tree->hash_algo) {
        tree->hash_algo = &sha256_algo;  // Default
    }
    
    return tree;
}

// Serialize merkle proof to CBOR
cbor_item_t* merkle_proof_to_cbor(const merkle_proof_t* proof) {
    if (!proof) return NULL;
    
    cbor_item_t* map = cbor_new_definite_map(4);
    if (!map) return NULL;
    
    // Version (legacy version 1 for standard proofs)
    cbor_item_t* version_key = cbor_build_uint8(CBOR_KEY_VERSION);
    cbor_item_t* version_val = cbor_build_uint8(CBOR_MERKLE_PROOF_VERSION_LEGACY);
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = version_key, .value = version_val
    })) {
        cbor_decref(&map);
        return NULL;
    }
    
    // Indices count
    cbor_item_t* indices_count_key = cbor_build_uint8(CBOR_KEY_INDICES_COUNT);
    cbor_item_t* indices_count_val = cbor_build_uint32(proof->indices_count);
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = indices_count_key, .value = indices_count_val
    })) {
        cbor_decref(&map);
        return NULL;
    }
    
    // Indices array
    cbor_item_t* indices_key = cbor_build_uint8(CBOR_KEY_INDICES);
    cbor_item_t* indices_array = cbor_new_definite_array(proof->indices_count);
    if (!indices_array) {
        cbor_decref(&map);
        return NULL;
    }
    
    for (uint32_t i = 0; i < proof->indices_count; i++) {
        cbor_item_t* index_item = cbor_build_uint32(proof->indices[i]);
        if (!cbor_array_push(indices_array, index_item)) {
            cbor_decref(&indices_array);
            cbor_decref(&map);
            return NULL;
        }
        cbor_decref(&index_item);
    }
    
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = indices_key, .value = indices_array
    })) {
        cbor_decref(&map);
        return NULL;
    }
    
    // Lemmas array
    cbor_item_t* lemmas_key = cbor_build_uint8(CBOR_KEY_LEMMAS);
    cbor_item_t* lemmas_val = hash_array_to_cbor(proof->lemmas, proof->lemmas_count);
    if (!lemmas_val || !cbor_map_add(map, (struct cbor_pair) {
        .key = lemmas_key, .value = lemmas_val
    })) {
        cbor_decref(&map);
        if (lemmas_val) cbor_decref(&lemmas_val);
        return NULL;
    }
    
    return map;
}

// Deserialize merkle proof from CBOR
merkle_proof_t* merkle_proof_from_cbor(cbor_item_t* item) {
    if (!cbor_isa_map(item)) {
        return NULL;
    }
    
    merkle_proof_t* proof = malloc(sizeof(merkle_proof_t));
    if (!proof) return NULL;
    
    memset(proof, 0, sizeof(merkle_proof_t));
    
    // Extract indices count
    cbor_item_t* indices_count_key = cbor_build_uint8(CBOR_KEY_INDICES_COUNT);
    cbor_item_t* indices_count_val = cbor_map_get(item, indices_count_key);
    cbor_decref(&indices_count_key);
    
    if (!indices_count_val || !cbor_isa_uint(indices_count_val)) {
        free(proof);
        if (indices_count_val) cbor_decref(&indices_count_val);
        return NULL;
    }
    
    proof->indices_count = cbor_get_uint32(indices_count_val);
    cbor_decref(&indices_count_val);
    
    // Extract indices array
    cbor_item_t* indices_key = cbor_build_uint8(CBOR_KEY_INDICES);
    cbor_item_t* indices_val = cbor_map_get(item, indices_key);
    cbor_decref(&indices_key);
    
    if (!indices_val || !cbor_isa_array(indices_val)) {
        free(proof);
        if (indices_val) cbor_decref(&indices_val);
        return NULL;
    }
    
    proof->indices = malloc(proof->indices_count * sizeof(uint32_t));
    if (!proof->indices) {
        free(proof);
        cbor_decref(&indices_val);
        return NULL;
    }
    
    for (uint32_t i = 0; i < proof->indices_count; i++) {
        cbor_item_t* index_item = cbor_array_get(indices_val, i);
        if (!index_item || !cbor_isa_uint(index_item)) {
            free(proof->indices);
            free(proof);
            cbor_decref(&indices_val);
            if (index_item) cbor_decref(&index_item);
            return NULL;
        }
        proof->indices[i] = cbor_get_uint32(index_item);
        cbor_decref(&index_item);
    }
    cbor_decref(&indices_val);
    
    // Extract lemmas array
    cbor_item_t* lemmas_key = cbor_build_uint8(CBOR_KEY_LEMMAS);
    cbor_item_t* lemmas_val = cbor_map_get(item, lemmas_key);
    cbor_decref(&lemmas_key);
    
    if (!lemmas_val) {
        free(proof->indices);
        free(proof);
        return NULL;
    }
    
    size_t lemmas_count;
    proof->lemmas = cbor_to_hash_array(lemmas_val, &lemmas_count);
    proof->lemmas_count = (uint32_t)lemmas_count;
    cbor_decref(&lemmas_val);
    
    if (!proof->lemmas) {
        free(proof->indices);
        free(proof);
        return NULL;
    }
    
    return proof;
}

// Convenience function: serialize tree to buffer
cbor_buffer_t merkle_tree_to_cbor_buffer(const merkle_tree_t* tree) {
    cbor_buffer_t result = {0};
    
    cbor_item_t* cbor_tree = merkle_tree_to_cbor(tree);
    if (!cbor_tree) {
        return result;
    }
    
    result.size = cbor_serialize_alloc(cbor_tree, &result.data, &result.size);
    result.success = (result.data != NULL);
    
    cbor_decref(&cbor_tree);
    return result;
}

// Convenience function: serialize proof to buffer
cbor_buffer_t merkle_proof_to_cbor_buffer(const merkle_proof_t* proof) {
    cbor_buffer_t result = {0};
    
    cbor_item_t* cbor_proof = merkle_proof_to_cbor(proof);
    if (!cbor_proof) {
        return result;
    }
    
    result.size = cbor_serialize_alloc(cbor_proof, &result.data, &result.size);
    result.success = (result.data != NULL);
    
    cbor_decref(&cbor_proof);
    return result;
}

// Convenience function: deserialize tree from buffer
merkle_tree_t* merkle_tree_from_cbor_buffer(const uint8_t* buffer, size_t size) {
    struct cbor_load_result result;
    cbor_item_t* item = cbor_load(buffer, size, &result);
    
    if (result.error.code != CBOR_ERR_NONE || !item) {
        if (item) cbor_decref(&item);
        return NULL;
    }
    
    merkle_tree_t* tree = merkle_tree_from_cbor(item);
    cbor_decref(&item);
    
    return tree;
}

// Convenience function: deserialize proof from buffer
merkle_proof_t* merkle_proof_from_cbor_buffer(const uint8_t* buffer, size_t size) {
    struct cbor_load_result result;
    cbor_item_t* item = cbor_load(buffer, size, &result);
    
    if (result.error.code != CBOR_ERR_NONE || !item) {
        if (item) cbor_decref(&item);
        return NULL;
    }
    
    merkle_proof_t* proof = merkle_proof_from_cbor(item);
    cbor_decref(&item);
    
    return proof;
}

// Free buffer allocated by serialization functions
void cbor_buffer_free(cbor_buffer_t* buffer) {
    if (buffer && buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
        buffer->size = 0;
        buffer->success = false;
    }
}

// Serialize secure merkle proof to CBOR (unified structure with optional expected_depth)
cbor_item_t* secure_merkle_proof_to_cbor(const secure_merkle_proof_t* proof) {
    if (!proof) return NULL;
    
    // Map size is 5 or 6 depending on whether expected_depth is included
    cbor_item_t* map = cbor_new_definite_map(6);
    if (!map) return NULL;
    
    // Version (secure version 2)
    cbor_item_t* version_key = cbor_build_uint8(CBOR_KEY_VERSION);
    cbor_item_t* version_val = cbor_build_uint8(CBOR_MERKLE_PROOF_VERSION_SECURE);
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = version_key, .value = version_val
    })) {
        cbor_decref(&map);
        return NULL;
    }
    
    // Indices array
    cbor_item_t* indices_key = cbor_build_uint8(CBOR_KEY_INDICES);
    cbor_item_t* indices_array = cbor_new_definite_array(proof->indices_count);
    if (!indices_array) {
        cbor_decref(&map);
        return NULL;
    }
    
    for (size_t i = 0; i < proof->indices_count; i++) {
        cbor_item_t* index_item = cbor_build_uint32(proof->indices[i]);
        if (!cbor_array_push(indices_array, index_item)) {
            cbor_decref(&indices_array);
            cbor_decref(&map);
            return NULL;
        }
        cbor_decref(&index_item);
    }
    
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = indices_key, .value = indices_array
    })) {
        cbor_decref(&map);
        return NULL;
    }
    
    // Lemmas array
    cbor_item_t* lemmas_key = cbor_build_uint8(CBOR_KEY_LEMMAS);
    cbor_item_t* lemmas_val = hash_array_to_cbor(proof->lemmas, proof->lemmas_count);
    if (!lemmas_val || !cbor_map_add(map, (struct cbor_pair) {
        .key = lemmas_key, .value = lemmas_val
    })) {
        cbor_decref(&map);
        if (lemmas_val) cbor_decref(&lemmas_val);
        return NULL;
    }
    
    // Indices count
    cbor_item_t* indices_count_key = cbor_build_uint8(CBOR_KEY_INDICES_COUNT);
    cbor_item_t* indices_count_val = cbor_build_uint32(proof->indices_count);
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = indices_count_key, .value = indices_count_val
    })) {
        cbor_decref(&map);
        return NULL;
    }
    
    // Lemmas count
    cbor_item_t* lemmas_count_key = cbor_build_uint8(CBOR_KEY_LEMMAS_COUNT);
    cbor_item_t* lemmas_count_val = cbor_build_uint32(proof->lemmas_count);
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = lemmas_count_key, .value = lemmas_count_val
    })) {
        cbor_decref(&map);
        return NULL;
    }
    
    // Expected depth (optional, included for secure proofs)
    cbor_item_t* expected_depth_key = cbor_build_uint8(CBOR_KEY_EXPECTED_DEPTH);
    cbor_item_t* expected_depth_val = cbor_build_uint8(proof->expected_depth);
    if (!cbor_map_add(map, (struct cbor_pair) {
        .key = expected_depth_key, .value = expected_depth_val
    })) {
        cbor_decref(&map);
        return NULL;
    }
    
    return map;
}

// Deserialize secure merkle proof from CBOR
secure_merkle_proof_t* secure_merkle_proof_from_cbor(cbor_item_t* item) {
    if (!cbor_isa_map(item)) {
        return NULL;
    }
    
    secure_merkle_proof_t* proof = malloc(sizeof(secure_merkle_proof_t));
    if (!proof) return NULL;
    
    memset(proof, 0, sizeof(secure_merkle_proof_t));
    
    // Extract indices count
    cbor_item_t* indices_count_key = cbor_build_uint8(CBOR_KEY_INDICES_COUNT);
    cbor_item_t* indices_count_val = cbor_map_get(item, indices_count_key);
    cbor_decref(&indices_count_key);
    
    if (!indices_count_val || !cbor_isa_uint(indices_count_val)) {
        free(proof);
        if (indices_count_val) cbor_decref(&indices_count_val);
        return NULL;
    }
    
    proof->indices_count = cbor_get_uint32(indices_count_val);
    cbor_decref(&indices_count_val);
    
    // Extract indices array
    cbor_item_t* indices_key = cbor_build_uint8(CBOR_KEY_INDICES);
    cbor_item_t* indices_val = cbor_map_get(item, indices_key);
    cbor_decref(&indices_key);
    
    if (!indices_val || !cbor_isa_array(indices_val)) {
        free(proof);
        if (indices_val) cbor_decref(&indices_val);
        return NULL;
    }
    
    proof->indices = malloc(proof->indices_count * sizeof(uint32_t));
    if (!proof->indices) {
        free(proof);
        cbor_decref(&indices_val);
        return NULL;
    }
    
    for (size_t i = 0; i < proof->indices_count; i++) {
        cbor_item_t* index_item = cbor_array_get(indices_val, i);
        if (!index_item || !cbor_isa_uint(index_item)) {
            free(proof->indices);
            free(proof);
            cbor_decref(&indices_val);
            if (index_item) cbor_decref(&index_item);
            return NULL;
        }
        proof->indices[i] = cbor_get_uint32(index_item);
        cbor_decref(&index_item);
    }
    cbor_decref(&indices_val);
    
    // Extract lemmas array
    cbor_item_t* lemmas_key = cbor_build_uint8(CBOR_KEY_LEMMAS);
    cbor_item_t* lemmas_val = cbor_map_get(item, lemmas_key);
    cbor_decref(&lemmas_key);
    
    if (!lemmas_val) {
        free(proof->indices);
        free(proof);
        return NULL;
    }
    
    size_t lemmas_count;
    proof->lemmas = cbor_to_hash_array(lemmas_val, &lemmas_count);
    proof->lemmas_count = (uint32_t)lemmas_count;
    cbor_decref(&lemmas_val);
    
    if (!proof->lemmas) {
        free(proof->indices);
        free(proof);
        return NULL;
    }
    
    // Extract expected depth (optional)
    cbor_item_t* expected_depth_key = cbor_build_uint8(CBOR_KEY_EXPECTED_DEPTH);
    cbor_item_t* expected_depth_val = cbor_map_get(item, expected_depth_key);
    cbor_decref(&expected_depth_key);
    
    if (expected_depth_val && cbor_isa_uint(expected_depth_val)) {
        proof->expected_depth = cbor_get_uint8(expected_depth_val);
        cbor_decref(&expected_depth_val);
    } else {
        proof->expected_depth = 0;  // Default value if not present
        if (expected_depth_val) cbor_decref(&expected_depth_val);
    }
    
    return proof;
}

#endif // WITH_CBOR