#include "merkle_tree.h"
#include <stdlib.h>
#include <string.h>

// Built-in secure hash algorithm configurations
const secure_hash_algo_t secure_sha256_max = {
    .hash_func = sha256_hash,
    .algo_name = "sha256-secure-max",
    .hash_size = HASH_SIZE,
    .use_double_leaf_hash = true,   // Double hash leaves
    .use_depth_prefix = true,       // Add depth to each hash
    .use_node_prefix = true         // Distinguish leaf/internal nodes
};

const secure_hash_algo_t secure_sha256_moderate = {
    .hash_func = sha256_hash,
    .algo_name = "sha256-secure-moderate",
    .hash_size = HASH_SIZE,
    .use_double_leaf_hash = false,  // Single hash is usually sufficient
    .use_depth_prefix = true,       // Depth prefixing is crucial
    .use_node_prefix = true         // Always distinguish node types
};

// Security-enhanced leaf hashing
void secure_leaf_hash(const uint8_t* data, size_t data_len, uint8_t depth, 
                     const secure_hash_algo_t* algo, uint8_t* result) {
    if (!data || !algo || !result) return;
    
    uint8_t temp_hash[HASH_SIZE];
    size_t prefix_len = (algo->use_node_prefix ? 1 : 0) + (algo->use_depth_prefix ? 1 : 0);
    uint8_t* prefixed_data = malloc(data_len + prefix_len);
    
    if (!prefixed_data) return;
    
    size_t offset = 0;
    
    // Add node type prefix
    if (algo->use_node_prefix) {
        prefixed_data[offset++] = LEAF_PREFIX;
    }
    
    // Add depth prefix
    if (algo->use_depth_prefix) {
        prefixed_data[offset++] = depth;
    }
    
    // Add original data
    memcpy(prefixed_data + offset, data, data_len);
    
    // First hash
    algo->hash_func(prefixed_data, prefixed_data + data_len + prefix_len, temp_hash);
    
    // Double hash for leaves if enabled
    if (algo->use_double_leaf_hash) {
        algo->hash_func(temp_hash, temp_hash + HASH_SIZE, result);
    } else {
        memcpy(result, temp_hash, HASH_SIZE);
    }
    
    free(prefixed_data);
}

// Security-enhanced internal node hashing
void secure_internal_hash(const uint8_t* left, const uint8_t* right, uint8_t depth,
                         const secure_hash_algo_t* algo, uint8_t* result) {
    if (!left || !right || !algo || !result) return;
    
    size_t prefix_len = (algo->use_node_prefix ? 1 : 0) + (algo->use_depth_prefix ? 1 : 0);
    uint8_t prefixed_data[HASH_SIZE * 2 + 2]; // +2 for prefix and depth
    
    size_t offset = 0;
    
    // Add node type prefix
    if (algo->use_node_prefix) {
        prefixed_data[offset++] = INTERNAL_PREFIX;
    }
    
    // Add depth prefix
    if (algo->use_depth_prefix) {
        prefixed_data[offset++] = depth;
    }
    
    // Add left and right hashes
    memcpy(prefixed_data + offset, left, HASH_SIZE);
    memcpy(prefixed_data + offset + HASH_SIZE, right, HASH_SIZE);
    
    // Single hash for internal nodes
    algo->hash_func(prefixed_data, prefixed_data + HASH_SIZE * 2 + prefix_len, result);
}

// Create new secure merkle tree
secure_merkle_tree_t* secure_merkle_tree_new(const secure_hash_algo_t* algo) {
    if (!algo) return NULL;
    
    secure_merkle_tree_t* tree = malloc(sizeof(secure_merkle_tree_t));
    if (!tree) return NULL;
    
    memset(tree, 0, sizeof(secure_merkle_tree_t));
    tree->algo = (secure_hash_algo_t*)algo;
    tree->security_enabled = true;
    
    return tree;
}

// Free secure merkle tree
void secure_merkle_tree_free(secure_merkle_tree_t* tree) {
    if (!tree) return;
    
    if (tree->nodes) {
        free(tree->nodes);
    }
    
    free(tree);
}

// Build secure merkle tree with depth validation
merkle_result_t secure_cbmt_build_merkle_tree(const hash_t* leaves, size_t leaves_count, 
                                             const secure_hash_algo_t* algo) {
    merkle_result_t result = {false, {NULL}};
    
    if (!leaves || leaves_count == 0 || !algo) {
        return result;
    }
    
    // Calculate and validate tree depth
    uint8_t depth = 0;
    size_t temp_count = leaves_count;
    while (temp_count > 1) {
        temp_count = (temp_count + 1) / 2;
        depth++;
    }
    
    if (depth > MAX_TREE_DEPTH) {
        return result; // Tree too deep
    }
    
    secure_merkle_tree_t* tree = secure_merkle_tree_new(algo);
    if (!tree) return result;
    
    tree->tree_depth = depth;
    
    // Calculate total nodes needed
    size_t total_nodes = leaves_count;
    temp_count = leaves_count;
    while (temp_count > 1) {
        temp_count = (temp_count + 1) / 2;
        total_nodes += temp_count;
    }
    
    tree->nodes = malloc(total_nodes * sizeof(hash_t));
    if (!tree->nodes) {
        secure_merkle_tree_free(tree);
        return result;
    }
    
    tree->nodes_count = total_nodes;
    
    // Copy leaf nodes with secure hashing
    for (size_t i = 0; i < leaves_count; i++) {
        secure_leaf_hash(leaves[i], HASH_SIZE, depth, algo, tree->nodes[i]);
    }
    
    // Build tree bottom-up with secure internal hashing
    size_t current_level_start = 0;
    size_t current_level_count = leaves_count;
    uint8_t current_depth = depth;
    
    while (current_level_count > 1) {
        size_t next_level_start = current_level_start + current_level_count;
        size_t next_level_count = (current_level_count + 1) / 2;
        current_depth--;
        
        for (size_t i = 0; i < next_level_count; i++) {
            size_t left_idx = current_level_start + (i * 2);
            size_t right_idx = left_idx + 1;
            
            if (right_idx < current_level_start + current_level_count) {
                // Both children exist
                secure_internal_hash(tree->nodes[left_idx], tree->nodes[right_idx], 
                                   current_depth, algo, tree->nodes[next_level_start + i]);
            } else {
                // Only left child exists, copy it up
                memcpy(tree->nodes[next_level_start + i], tree->nodes[left_idx], HASH_SIZE);
            }
        }
        
        current_level_start = next_level_start;
        current_level_count = next_level_count;
    }
    
    result.success = true;
    result.secure_tree = tree;
    return result;
}

// Create new secure merkle proof
secure_merkle_proof_t* secure_merkle_proof_new(const uint32_t* indices, size_t indices_count,
                                              const hash_t* lemmas, size_t lemmas_count,
                                              uint8_t expected_depth, const secure_hash_algo_t* algo) {
    if (!indices || !lemmas || indices_count == 0 || lemmas_count == 0 || !algo) {
        return NULL;
    }
    
    secure_merkle_proof_t* proof = malloc(sizeof(secure_merkle_proof_t));
    if (!proof) return NULL;
    
    memset(proof, 0, sizeof(secure_merkle_proof_t));
    
    // Copy indices
    proof->indices = malloc(indices_count * sizeof(uint32_t));
    if (!proof->indices) {
        free(proof);
        return NULL;
    }
    memcpy(proof->indices, indices, indices_count * sizeof(uint32_t));
    proof->indices_count = indices_count;
    
    // Copy lemmas
    proof->lemmas = malloc(lemmas_count * sizeof(hash_t));
    if (!proof->lemmas) {
        free(proof->indices);
        free(proof);
        return NULL;
    }
    memcpy(proof->lemmas, lemmas, lemmas_count * sizeof(hash_t));
    proof->lemmas_count = lemmas_count;
    
    proof->expected_depth = expected_depth;
    proof->algo = (secure_hash_algo_t*)algo;
    
    return proof;
}

// Free secure merkle proof
void secure_merkle_proof_free(secure_merkle_proof_t* proof) {
    if (!proof) return;
    
    if (proof->indices) {
        free(proof->indices);
    }
    
    if (proof->lemmas) {
        free(proof->lemmas);
    }
    
    free(proof);
}

// Verify secure merkle proof with depth validation
bool secure_merkle_proof_verify(const secure_merkle_proof_t* proof, const hash_t root, 
                               const hash_t* leaves, size_t leaves_count) {
    if (!proof || !leaves || leaves_count == 0 || proof->indices_count != leaves_count) {
        return false;
    }
    
    // This implementation currently supports proofs for a single leaf only.
    if (leaves_count != 1 || proof->indices_count != 1) {
        return false;
    }

    return secure_merkle_proof_verify_single(proof, root, leaves[0]);
}

bool secure_merkle_proof_verify_single(const secure_merkle_proof_t* proof, const hash_t root,
                                      const hash_t leaf) {
    if (!proof || !leaf) {
        return false;
    }

    if (proof->indices_count != 1) {
        return false;
    }
    
    // Validate expected depth against lemmas count
    if (proof->lemmas_count != proof->expected_depth) {
        return false;
    }
    
        hash_t current_hash;
    
    // Start with secure leaf hash
    secure_leaf_hash(leaf, HASH_SIZE, proof->expected_depth, proof->algo, current_hash);
    
    // Walk up the tree using lemmas with depth validation
    uint8_t current_depth = proof->expected_depth;
    uint32_t current_index = proof->indices[0];
    
    for (size_t i = 0; i < proof->lemmas_count; i++) {
        if (current_depth == 0) {
            return false; // Depth went to zero before reaching root
        }
        
        current_depth--;
        
        // Determine if current node is left or right child
        if (current_index % 2 == 0) {
            // Current is left child, lemma is right sibling
            secure_internal_hash(current_hash, proof->lemmas[i], current_depth, 
                               proof->algo, current_hash);
        } else {
            // Current is right child, lemma is left sibling
            secure_internal_hash(proof->lemmas[i], current_hash, current_depth,
                               proof->algo, current_hash);
        }
        
        current_index /= 2; // Move up one level
    }
    
    if (current_depth != 0) {
        return false; // Didn't reach expected root level
    }
    
    // Compare computed root with expected root
    return memcmp(current_hash, root, HASH_SIZE) == 0;
}

// Build secure merkle proof from secure tree
merkle_result_t secure_merkle_tree_build_proof(const secure_merkle_tree_t* tree, 
                                              const uint32_t* leaf_indices, size_t indices_count) {
    merkle_result_t result = {false, {NULL}};
    
    if (!tree || !leaf_indices || indices_count == 0) {
        return result;
    }
    
    // For simplicity, implement proof for single leaf
    // Full implementation would handle multiple leaves
    if (indices_count != 1) {
        return result; // Multi-leaf proofs not implemented in this example
    }
    
    uint32_t leaf_index = leaf_indices[0];
    
    // Calculate leaf count from tree depth
    size_t leaf_count = 1;
    for (int i = 0; i < tree->tree_depth; i++) {
        leaf_count *= 2;
    }
    
    if (leaf_index >= leaf_count) {
        return result; // Index out of bounds
    }
    
    // Build proof path
    hash_t* lemmas = malloc(tree->tree_depth * sizeof(hash_t));
    uint32_t* indices = malloc(sizeof(uint32_t));
    
    if (!lemmas || !indices) {
        if (lemmas) free(lemmas);
        if (indices) free(indices);
        return result;
    }
    
    indices[0] = leaf_index;
    
    // Walk up the tree collecting sibling hashes
    uint32_t current_index = leaf_index;
    size_t current_level_start = 0;
    size_t current_level_count = leaf_count;
    
    for (uint8_t depth = 0; depth < tree->tree_depth; depth++) {
        uint32_t sibling_index = current_index % 2 == 0 ? current_index + 1 : current_index - 1;
        
        if (sibling_index < current_level_count) {
            memcpy(lemmas[depth], tree->nodes[current_level_start + sibling_index], HASH_SIZE);
        } else {
            // No sibling, this shouldn't happen in a complete tree
            free(lemmas);
            free(indices);
            return result;
        }
        
        current_index /= 2;
        current_level_start += current_level_count;
        current_level_count = (current_level_count + 1) / 2;
    }
    
    secure_merkle_proof_t* proof = secure_merkle_proof_new(indices, 1, lemmas, tree->tree_depth,
                                                          tree->tree_depth, tree->algo);
    
    free(lemmas);
    free(indices);
    
    if (!proof) return result;
    
    result.success = true;
    result.secure_proof = proof;
    return result;
}