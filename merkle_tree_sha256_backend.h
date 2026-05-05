#ifndef MERKLE_TREE_SHA256_BACKEND_H
#define MERKLE_TREE_SHA256_BACKEND_H

#include <stddef.h>
#include <stdint.h>

// Consumer-provided SHA-256 digest backend for the optional adapter layer.
void merkle_tree_sha256_digest(const uint8_t* data, size_t len, uint8_t* result);

#endif // MERKLE_TREE_SHA256_BACKEND_H