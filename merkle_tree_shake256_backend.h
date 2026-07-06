#ifndef MERKLE_TREE_SHAKE256_BACKEND_H
#define MERKLE_TREE_SHAKE256_BACKEND_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Consumer-provided SHAKE256 digest backend for merkle tree operations.
void merkle_tree_shake256_digest(const uint8_t* data, size_t len, uint8_t* result);

#ifdef __cplusplus
}
#endif

#endif
