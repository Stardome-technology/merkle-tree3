#include "merkle_tree.h"
#include "merkle_tree_shake256_backend.h"

static void shake256_hash(const uint8_t* left, const uint8_t* right, uint8_t* result)
{
    size_t len = (size_t)(right - left);
    merkle_tree_shake256_digest(left, len, result);
}

const hash_algo_t shake256_algo = {
    .hash_func = shake256_hash,
    .algo_name = "shake256",
    .hash_size = HASH_SIZE
};
