#include "merkle_tree_shake256_backend.h"
#include "xmss_shake256_wrapper.h"

void merkle_tree_shake256_digest(const uint8_t* data, size_t len, uint8_t* result)
{
    xmss_shake256_wrapper(data, (unsigned long long)len, result);
}
