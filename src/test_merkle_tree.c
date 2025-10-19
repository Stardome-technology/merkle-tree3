#include "merkle_tree.h"
#ifdef WITH_CBOR
#include "merkle_tree_cbor.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Helper function to create a hash from an integer (for testing)
void int_to_hash(int32_t value, hash_t hash) {
    hash_zero(hash);
    memcpy(hash, &value, sizeof(int32_t));
}

// Helper function to extract integer from hash (for testing)
int32_t hash_to_int(const hash_t hash) {
    int32_t value;
    memcpy(&value, hash, sizeof(int32_t));
    return value;
}

// Helper function to print hash
void print_hash(const hash_t hash) {
    for (int i = 0; i < 4; i++) { // Print first 4 bytes as int32
        printf("%02x", hash[i]);
    }
}

// Test function to demonstrate usage
void test_merkle_tree() {
    printf("Testing Hash-based Merkle Tree C implementation...\n");
    
    // Test data: convert integers to hashes
    int32_t test_values[] = {2, 3, 5, 7, 11};
    size_t leaves_count = sizeof(test_values) / sizeof(test_values[0]);
    
    hash_t* leaves = malloc(leaves_count * sizeof(hash_t));
    assert(leaves != NULL);
    
    printf("Building merkle tree with leaf values: ");
    for (size_t i = 0; i < leaves_count; i++) {
        int_to_hash(test_values[i], leaves[i]);
        printf("%d ", test_values[i]);
    }
    printf("\n");
    
    // Build merkle tree
    merkle_tree_t* tree = cbmt_build_merkle_tree(leaves, leaves_count, &sha256_algo);
    assert(tree != NULL);
    
    // Get root
    hash_t root;
    merkle_tree_root(tree, root);
    printf("Merkle root: ");
    print_hash(root);
    printf("\n");
    
    // Build merkle root directly (should match tree root)
    hash_t direct_root;
    cbmt_build_merkle_root(leaves, leaves_count, &sha256_algo, direct_root);
    printf("Direct merkle root: ");
    print_hash(direct_root);
    printf("\n");
    assert(hash_compare(root, direct_root) == 0);
    
    // Build proof for leaves at indices 0 and 3
    uint32_t leaf_indices[] = {0, 3};
    size_t indices_count = sizeof(leaf_indices) / sizeof(leaf_indices[0]);
    
    merkle_result_t proof_result = merkle_tree_build_proof(tree, leaf_indices, indices_count);
    assert(proof_result.success);
    
    merkle_proof_t* proof = proof_result.proof;
    printf("Proof built successfully\n");
    printf("Lemmas count: %zu\n", merkle_proof_lemmas_count(proof));
    printf("Indices count: %zu\n", merkle_proof_indices_count(proof));
    
    // Verify proof
    hash_t proof_leaves[2];
    hash_copy(leaves[0], proof_leaves[0]); // leaves[0] = 2
    hash_copy(leaves[3], proof_leaves[1]); // leaves[3] = 7
    
    bool verified = merkle_proof_verify(proof, root, proof_leaves, 2);
    printf("Proof verification: %s\n", verified ? "PASSED" : "FAILED");
    assert(verified);
    
    // Test root computation from proof
    hash_t computed_root;
    bool root_success = merkle_proof_root(proof, proof_leaves, 2, computed_root);
    printf("Root from proof: %s\n", root_success ? "SUCCESS" : "FAILED");
    if (root_success) {
        printf("Computed root: ");
        print_hash(computed_root);
        printf("\n");
    }
    assert(root_success);
    
    // Test retrieve leaves
    size_t retrieved_count;
    merkle_result_t retrieve_result = cbmt_retrieve_leaves(leaves, leaves_count, proof, &retrieved_count);
    assert(retrieve_result.success);
    
    hash_t* retrieved_leaves = (hash_t*)retrieve_result.data;
    printf("Retrieved %zu leaves: ", retrieved_count);
    for (size_t i = 0; i < retrieved_count; i++) {
        int32_t value = hash_to_int(retrieved_leaves[i]);
        printf("%d ", value);
    }
    printf("\n");
    
    // Cleanup
    free(retrieved_leaves);
    free(leaves);
    merkle_proof_free(proof);
    merkle_tree_free(tree);
    
    printf("All tests passed!\n");
}

void test_single_leaf() {
    printf("\nTesting single leaf...\n");
    
    hash_t leaves[1];
    int_to_hash(42, leaves[0]);
    
    merkle_tree_t* tree = cbmt_build_merkle_tree(leaves, 1, &sha256_algo);
    assert(tree != NULL);
    
    hash_t root;
    merkle_tree_root(tree, root);
    printf("Single leaf root: ");
    print_hash(root);
    printf(" (value: %d)\n", hash_to_int(root));
    assert(hash_compare(root, leaves[0]) == 0);
    
    // Build proof for single leaf
    uint32_t leaf_indices[] = {0};
    merkle_result_t proof_result = merkle_tree_build_proof(tree, leaf_indices, 1);
    assert(proof_result.success);
    
    merkle_proof_t* proof = proof_result.proof;
    printf("Single leaf lemmas count: %zu\n", merkle_proof_lemmas_count(proof));
    
    // Verify proof
    bool verified = merkle_proof_verify(proof, root, leaves, 1);
    printf("Single leaf verification: %s\n", verified ? "PASSED" : "FAILED");
    assert(verified);
    
    merkle_proof_free(proof);
    merkle_tree_free(tree);
    
    printf("Single leaf test passed!\n");
}

void test_empty_tree() {
    printf("\nTesting empty tree...\n");
    
    merkle_tree_t* tree = cbmt_build_merkle_tree(NULL, 0, &sha256_algo);
    assert(tree != NULL);
    
    hash_t root;
    merkle_tree_root(tree, root);
    printf("Empty tree root: ");
    print_hash(root);
    printf("\n");
    
    // Check that root is zero hash
    hash_t zero_hash;
    hash_zero(zero_hash);
    assert(hash_compare(root, zero_hash) == 0);
    
    assert(merkle_tree_nodes_count(tree) == 0);
    
    merkle_tree_free(tree);
    
    printf("Empty tree test passed!\n");
}

void test_hash_utilities() {
    printf("\nTesting hash utilities...\n");
    
    hash_t hash1, hash2;
    int_to_hash(12345, hash1);
    
    // Test hex conversion
    char hex_str[HASH_HEX_SIZE];
    hash_to_hex(hash1, hex_str);
    printf("Hash as hex: %s\n", hex_str);
    
    // Test hex parsing
    bool parsed = hash_from_hex(hex_str, hash2);
    assert(parsed);
    assert(hash_compare(hash1, hash2) == 0);
    
    // Test hash copy
    hash_t hash3;
    hash_copy(hash1, hash3);
    assert(hash_compare(hash1, hash3) == 0);
    
    printf("Hash utilities test passed!\n");
}

#ifdef WITH_CBOR
// Test CBOR serialization/deserialization
void test_cbor_serialization() {
    printf("\n--- Testing CBOR Serialization ---\n");
    
    // Create test data
    int32_t test_values[] = {100, 200, 300};
    size_t leaves_count = 3;
    
    hash_t* leaves = malloc(leaves_count * sizeof(hash_t));
    assert(leaves != NULL);
    
    for (size_t i = 0; i < leaves_count; i++) {
        int_to_hash(test_values[i], leaves[i]);
    }
    
    // Build tree
    merkle_tree_t* original_tree = cbmt_build_merkle_tree(leaves, leaves_count, &sha256_algo);
    assert(original_tree != NULL);
    
    // Test tree serialization
    cbor_buffer_t tree_buffer = merkle_tree_to_cbor_buffer(original_tree);
    assert(tree_buffer.success);
    printf("Serialized tree to %zu bytes\n", tree_buffer.size);
    
    // Test tree deserialization
    merkle_tree_t* restored_tree = merkle_tree_from_cbor_buffer(tree_buffer.data, tree_buffer.size);
    assert(restored_tree != NULL);
    
    // Compare trees
    assert(restored_tree->nodes_count == original_tree->nodes_count);
    
    hash_t orig_root, restored_root;
    merkle_tree_root(original_tree, orig_root);
    merkle_tree_root(restored_tree, restored_root);
    assert(hash_equal(orig_root, restored_root));
    
    printf("Tree CBOR serialization test passed!\n");
    
    // Test proof serialization
    uint32_t proof_indices[] = {0, 2};
    merkle_result_t proof_result = merkle_tree_build_proof(original_tree, proof_indices, 2);
    assert(proof_result.success);
    
    cbor_buffer_t proof_buffer = merkle_proof_to_cbor_buffer(proof_result.proof);
    assert(proof_buffer.success);
    printf("Serialized proof to %zu bytes\n", proof_buffer.size);
    
    // Test proof deserialization
    merkle_proof_t* restored_proof = merkle_proof_from_cbor_buffer(proof_buffer.data, proof_buffer.size);
    assert(restored_proof != NULL);
    
    // Verify restored proof works
    hash_t proof_leaves[] = {leaves[0], leaves[2]};
    bool verified = merkle_proof_verify(restored_proof, orig_root, proof_leaves, 2);
    assert(verified);
    
    printf("Proof CBOR serialization test passed!\n");
    
    // Cleanup
    merkle_tree_free(original_tree);
    merkle_tree_free(restored_tree);
    merkle_proof_free(proof_result.proof);
    merkle_proof_free(restored_proof);
    cbor_buffer_free(&tree_buffer);
    cbor_buffer_free(&proof_buffer);
    free(leaves);
}
#endif // WITH_CBOR

int main() {
    printf("=== Hash-based Merkle Tree C Implementation Tests ===\n");
    
    test_empty_tree();
    test_single_leaf();
    test_merkle_tree();
    test_hash_utilities();
    
#ifdef WITH_CBOR
    test_cbor_serialization();
#else
    printf("\nCBOR support disabled - skipping CBOR tests\n");
#endif
    
    printf("\n=== All tests completed successfully! ===\n");
    return 0;
}