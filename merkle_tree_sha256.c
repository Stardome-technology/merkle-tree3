#include "merkle_tree.h"
#include "merkle_tree_sha256_backend.h"

void sha256_hash(const uint8_t* left, const uint8_t* right, uint8_t* result) {
    size_t len = (size_t)(right - left);
    merkle_tree_sha256_digest(left, len, result);
}

const hash_algo_t sha256_algo = {
    .hash_func = sha256_hash,
    .algo_name = "sha256",
    .hash_size = HASH_SIZE
};

const secure_hash_algo_t secure_sha256_max = {
    .hash_func = sha256_hash,
    .algo_name = "sha256-secure-max",
    .hash_size = HASH_SIZE,
    .use_double_leaf_hash = true,
    .use_depth_prefix = true,
    .use_node_prefix = true
};

const secure_hash_algo_t secure_sha256_moderate = {
    .hash_func = sha256_hash,
    .algo_name = "sha256-secure-moderate",
    .hash_size = HASH_SIZE,
    .use_double_leaf_hash = false,
    .use_depth_prefix = true,
    .use_node_prefix = true
};