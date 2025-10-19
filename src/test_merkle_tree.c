#include "merkle_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Test function to demonstrate usage
void test_merkle_tree() {
    printf("Testing Merkle Tree C implementation...\n");
    
    // Test data: same as in Rust tests
    int32_t leaves[] = {2, 3, 5, 7, 11};
    size_t leaves_count = sizeof(leaves) / sizeof(leaves[0]);
    
    printf("Building merkle tree with leaves: ");
    for (size_t i = 0; i < leaves_count; i++) {
        printf("%d ", leaves[i]);
    }
    printf("\n");
    
    // Build merkle tree
    merkle_tree_t* tree = cbmt_build_merkle_tree(leaves, leaves_count, &int32_ops);
    assert(tree != NULL);
    
    // Get root
    int32_t root;
    merkle_tree_root(tree, &root);
    printf("Merkle root: %d\n", root);
    
    // Build merkle root directly (should match tree root)
    int32_t direct_root;
    cbmt_build_merkle_root(leaves, leaves_count, &int32_ops, &direct_root);
    printf("Direct merkle root: %d\n", direct_root);
    assert(root == direct_root);
    
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
    int32_t proof_leaves[] = {2, 7}; // leaves[0] and leaves[3]
    bool verified = merkle_proof_verify(proof, &root, proof_leaves, 2);
    printf("Proof verification: %s\n", verified ? "PASSED" : "FAILED");
    assert(verified);
    
    // Test root computation from proof
    int32_t computed_root;
    bool root_success = merkle_proof_root(proof, proof_leaves, 2, &computed_root);
    printf("Root from proof: %s, value: %d\n", root_success ? "SUCCESS" : "FAILED", computed_root);
    assert(root_success && computed_root == root);
    
    // Test retrieve leaves
    size_t retrieved_count;
    merkle_result_t retrieve_result = cbmt_retrieve_leaves(leaves, leaves_count, proof, &retrieved_count);
    assert(retrieve_result.success);
    
    int32_t* retrieved_leaves = (int32_t*)retrieve_result.data;
    printf("Retrieved leaves: ");
    for (size_t i = 0; i < retrieved_count; i++) {
        printf("%d ", retrieved_leaves[i]);
    }
    printf("\n");
    
    // Cleanup
    free(retrieved_leaves);
    merkle_proof_free(proof);
    merkle_tree_free(tree);
    
    printf("All tests passed!\n");
}

void test_single_leaf() {
    printf("\nTesting single leaf...\n");
    
    int32_t leaves[] = {42};
    size_t leaves_count = 1;
    
    merkle_tree_t* tree = cbmt_build_merkle_tree(leaves, leaves_count, &int32_ops);
    assert(tree != NULL);
    
    int32_t root;
    merkle_tree_root(tree, &root);
    printf("Single leaf root: %d\n", root);
    assert(root == 42);
    
    // Build proof for single leaf
    uint32_t leaf_indices[] = {0};
    merkle_result_t proof_result = merkle_tree_build_proof(tree, leaf_indices, 1);
    assert(proof_result.success);
    
    merkle_proof_t* proof = proof_result.proof;
    printf("Single leaf lemmas count: %zu\n", merkle_proof_lemmas_count(proof));
    assert(merkle_proof_lemmas_count(proof) == 0); // No lemmas needed for single leaf
    
    // Verify proof
    bool verified = merkle_proof_verify(proof, &root, leaves, 1);
    printf("Single leaf verification: %s\n", verified ? "PASSED" : "FAILED");
    assert(verified);
    
    merkle_proof_free(proof);
    merkle_tree_free(tree);
    
    printf("Single leaf test passed!\n");
}

void test_empty_tree() {
    printf("\nTesting empty tree...\n");
    
    merkle_tree_t* tree = cbmt_build_merkle_tree(NULL, 0, &int32_ops);
    assert(tree != NULL);
    
    int32_t root;
    merkle_tree_root(tree, &root);
    printf("Empty tree root: %d\n", root);
    assert(root == 0); // Default value for int32_t
    
    assert(merkle_tree_nodes_count(tree) == 0);
    
    merkle_tree_free(tree);
    
    printf("Empty tree test passed!\n");
}

int main() {
    printf("=== Merkle Tree C Implementation Tests ===\n");
    
    test_empty_tree();
    test_single_leaf();
    test_merkle_tree();
    
    printf("\n=== All tests completed successfully! ===\n");
    return 0;
}