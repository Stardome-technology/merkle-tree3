#include "merkle_tree_shake256_backend.h"
#include "fips202.h"

void merkle_tree_shake256_digest(const uint8_t* data, size_t len, uint8_t* result)
{
    shake256(result, 32, data, (unsigned long long)len);
}
