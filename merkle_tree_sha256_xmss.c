#include "merkle_tree_sha256_backend.h"
#include "xmss-reference/core/sha256.h"

void merkle_tree_sha256_digest(const uint8_t* data, size_t len, uint8_t* result) {
    xmss_sha256_wrapper(data, (unsigned long long)len, result);
}