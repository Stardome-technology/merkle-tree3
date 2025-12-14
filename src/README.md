# Merkle Tree C Implementation (Hash-Based)

This is a C implementation of a hash-based Complete Binary Merkle Tree (CBMT), designed specifically for cryptographic hash values.

## Features

- **Hash-Based Values Only**: Works exclusively with fixed-size hash values (32 bytes)
- **Complete Binary Merkle Tree**: Efficient binary tree structure for merkle proofs
- **Configurable Hash Algorithms**: Support for different hash functions via function pointers
- **Security-Enhanced Implementation**: Protection against second pre-image attacks
- **Merkle Proofs**: Generate and verify proofs for sets of hash leaves with depth validation
- **CBOR Serialization**: Binary serialization with raw bytes for efficient communication
- **Memory Safe**: Proper memory management with cleanup functions
- **No Dependencies**: Pure C implementation with standard library only (CBOR support optional)

## Hash Support

This implementation is restricted to working with hash values only:
- Fixed 32-byte hash size (configurable via `HASH_SIZE`)
- Hex string representation support (64 hex characters)
- Built-in SHA256 algorithm (simplified for demonstration)
- Extensible to other hash algorithms via function pointers

## Files

- `merkle_tree.h` - Header file with all public APIs and structures
- `merkle_tree.c` - Implementation of all merkle tree functionality
- `merkle_tree_secure.c` - Security-enhanced implementation with attack protection
- `test_merkle_tree.c` - Example usage and test cases with hash values
- `merkle_tree_cbor.h` - CBOR serialization header (optional)
- `merkle_tree_cbor.c` - CBOR serialization implementation (optional)
- `merkle-tree.cddl` - CBOR schema documentation (CDDL format)
- `CBOR_INSTALL.md` - Instructions for installing libcbor dependency
- `Makefile` - Build configuration with optional CBOR support
- `CMakeLists.txt` - CMake build configuration with CBOR auto-detection

## Building

### Quick Setup
1. **Install libcbor** (optional): See [CBOR_INSTALL.md](CBOR_INSTALL.md)
2. **Build**: Choose your preferred method below

### Windows Batch Script (Easy)
```cmd
# Build and run (detects available compiler automatically)
build.bat
```

### Using Make (Linux/macOS/MinGW)
```bash
# Build with CBOR support (default if libcbor found)
make test

# Build without CBOR
make WITH_CBOR=0

# Build static library
make lib

# Clean build artifacts
make clean
```

### Using CMake (Cross-platform)
```bash
mkdir build && cd build

# Auto-detect CBOR support
cmake ..

# Force enable CBOR
cmake .. -DWITH_CBOR=ON

# Disable CBOR
cmake .. -DWITH_CBOR=OFF

make
```
# Create build directory
mkdir build && cd build

# Configure (Release build)
cmake ..

# Configure (Debug build)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build
cmake --build .

# Run tests
ctest

# Install (optional)
cmake --install .
```

### Manual compilation
```bash
# GCC/Clang
gcc -Wall -Wextra -std=c99 -O2 -o test_merkle_tree merkle_tree.c test_merkle_tree.c

# Microsoft Visual C++
cl /W4 /O2 /Fe:test_merkle_tree.exe merkle_tree.c test_merkle_tree.c

# Run tests
./test_merkle_tree        # Linux/macOS
test_merkle_tree.exe      # Windows
```

## Usage

### Basic Example

```c
#include "merkle_tree.h"
#include <stdio.h>

// Helper function to create hash from integer (for testing)
void int_to_hash(int32_t value, hash_t hash) {
    hash_zero(hash);
    memcpy(hash, &value, sizeof(int32_t));
}

int main() {
    // Create hash values from sample data
    hash_t leaves[5];
    int32_t values[] = {2, 3, 5, 7, 11};
    
    for (int i = 0; i < 5; i++) {
        int_to_hash(values[i], leaves[i]);
    }
    
    // Build merkle tree
    merkle_tree_t* tree = cbmt_build_merkle_tree(leaves, 5, &sha256_algo);
    
    // Get root
    hash_t root;
    merkle_tree_root(tree, root);
    
    // Convert to hex string
    char hex_str[HASH_HEX_SIZE];
    hash_to_hex(root, hex_str);
    printf("Merkle root: %s\n", hex_str);
    
    // Build proof for leaves at indices 0 and 3
    uint32_t leaf_indices[] = {0, 3};
    merkle_result_t proof_result = merkle_tree_build_proof(tree, leaf_indices, 2);
    
    if (proof_result.success) {
        merkle_proof_t* proof = proof_result.proof;
        
        // Verify proof
        hash_t proof_leaves[] = {leaves[0], leaves[3]};
        bool verified = merkle_proof_verify(proof, root, proof_leaves, 2);
        printf("Proof verified: %s\n", verified ? "true" : "false");
        
        merkle_proof_free(proof);
    }
    
    merkle_tree_free(tree);
    return 0;
}
```

### Security-Enhanced Usage

For production use, use the secure implementation that protects against second pre-image attacks:

```c
#include "merkle_tree.h"

int main() {
    // Create hash values
    hash_t leaves[3];
    // ... populate leaves ...
    
    // Build secure merkle tree (recommended for production)
    merkle_result_t result = secure_cbmt_build_merkle_tree(
        leaves, 3, &secure_sha256_moderate
    );
    
    if (result.success) {
        secure_merkle_tree_t* tree = result.secure_tree;
        
        // Build secure proof
        uint32_t indices[] = {0};
        merkle_result_t proof_result = secure_merkle_tree_build_proof(tree, indices, 1);
        
        if (proof_result.success) {
            secure_merkle_proof_t* proof = proof_result.secure_proof;
            
            // Verify with depth validation
            hash_t root;
            // ... get root from tree ...
            bool verified = secure_merkle_proof_verify(proof, root, &leaves[0], 1);
            
            secure_merkle_proof_free(proof);
        }
        
        secure_merkle_tree_free(tree);
    }
    
    return 0;
}
```

### Security Configurations

Two built-in security levels are available:

```c
// Maximum security - all protections enabled
const secure_hash_algo_t secure_sha256_max = {
    .use_double_leaf_hash = true,   // Double hash leaves
    .use_depth_prefix = true,       // Add depth to each hash
    .use_node_prefix = true         // Distinguish leaf/internal nodes
};

// Moderate security - recommended for most use cases
const secure_hash_algo_t secure_sha256_moderate = {
    .use_double_leaf_hash = false,  // Single hash (sufficient)
    .use_depth_prefix = true,       // Depth prefixing (crucial)
    .use_node_prefix = true         // Node type prefixing (crucial)
};
```

### Custom Hash Algorithms

To use custom hash algorithms, implement the hash function:

```c
// Example for custom hash function
void my_hash_function(const uint8_t* left, const uint8_t* right, uint8_t* result) {
    // Implement your hash function here
    // Must combine left and right into result (32 bytes)
    for (int i = 0; i < HASH_SIZE; i++) {
        result[i] = left[i] ^ right[i]; // Simple XOR example
    }
}

const hash_algo_t my_algo = {
    .hash_func = my_hash_function,
    .algo_name = "custom_xor",
    .hash_size = HASH_SIZE
};

// For secure version
const secure_hash_algo_t my_secure_algo = {
    .hash_func = my_hash_function,
    .algo_name = "custom_xor_secure",
    .hash_size = HASH_SIZE,
    .use_double_leaf_hash = false,
    .use_depth_prefix = true,
    .use_node_prefix = true
};
```

## Security Features

### Attack Protection
- **Second Pre-image Attack Mitigation**: Node type prefixing prevents internal nodes from being used as leaves
- **Depth Validation**: Depth prefixing ensures proof chains follow correct tree traversal
- **Hash Chain Protection**: Prevents malicious manipulation of merkle proof paths
- **Tree Depth Limits**: Maximum depth validation prevents resource exhaustion attacks

### Security Levels
- **Maximum Security**: Double leaf hashing + depth prefixing + node type prefixing
- **Moderate Security**: Depth prefixing + node type prefixing (recommended)
- **Legacy Mode**: Standard implementation without security enhancements (compatibility)

## Core Features

- **Hash-based Values**: Designed specifically for cryptographic hash values (32-byte fixed size)
- **Complete Binary Merkle Tree (CBMT)**: Implements the CBMT algorithm for efficient tree construction
- **Memory Management**: Automatic memory allocation and cleanup with proper error handling  
- **Proof Generation**: Build proofs for any subset of leaves with security validation
- **Proof Verification**: Verify proofs against merkle roots with depth validation
- **Flexible Hash Algorithms**: Support for custom hash functions (SHA256 included by default)
- **CBOR Serialization**: Binary serialization with raw bytes for efficient serial communication
- **Cross-platform**: Works on Windows, Linux, and macOS with multiple build systems
- **Optional Dependencies**: Core functionality works without external libraries

### CBOR Serialization

The implementation supports efficient CBOR serialization with raw bytes:

```c
#ifdef WITH_CBOR
#include "merkle_tree_cbor.h"

// Serialize tree to CBOR buffer
cbor_buffer_t buffer = merkle_tree_to_cbor_buffer(tree);
if (buffer.success) {
    // Send buffer.data over serial communication
    // buffer.size contains the data length
    cbor_buffer_free(&buffer);
}

// Deserialize tree from CBOR buffer
merkle_tree_t* tree = merkle_tree_from_cbor_buffer(data, data_size);
#endif
```

**CBOR Schema**: See `merkle-tree.cddl` for complete CBOR schema documentation.

## API Reference

### Core Structures

- `hash_t` - Fixed 32-byte hash value type
- `secure_merkle_tree_t` - Security-enhanced merkle tree with attack protection
- `secure_merkle_proof_t` - Security-enhanced merkle proof with depth validation
- `merkle_tree_t` - Represents a complete binary merkle tree
- `merkle_proof_t` - Represents a merkle proof for a set of leaves  
- `hash_algo_t` - Hash algorithm specification

### Main Functions

#### Tree Operations
- `cbmt_build_merkle_tree()` - Build tree from hash array and hash algorithm
- `merkle_tree_free()` - Free tree memory
- `merkle_tree_root()` - Get root hash
- `merkle_tree_leaves_count()` - Get number of leaves
- `cbmt_retrieve_leaves()` - Retrieve leaf hashes by indices

#### Merkle Proof Operations
- `merkle_tree_build_proof()` - Build proof for specific leaf indices
- `merkle_proof_free()` - Free proof memory
- `merkle_proof_verify()` - Verify proof against root and leaves
- `merkle_proof_root()` - Compute root from proof and leaves

#### Hash Utilities
- `hash_zero()` - Zero out a hash
- `hash_copy()` - Copy hash value
- `hash_equal()` - Compare two hashes
- `hash_to_hex()` - Convert hash to hex string
- `hex_to_hash()` - Convert hex string to hash

#### CBOR Serialization (Optional)
When compiled with `-DWITH_CBOR`:
- `merkle_tree_to_cbor_buffer()` - Serialize tree to binary buffer
- `merkle_tree_from_cbor_buffer()` - Deserialize tree from buffer
- `merkle_proof_to_cbor_buffer()` - Serialize proof to binary buffer
- `merkle_proof_from_cbor_buffer()` - Deserialize proof from buffer
- `cbor_buffer_free()` - Free serialization buffers

### Hash Algorithms

The library includes SHA256 by default:
- `sha256_algo` - Built-in SHA256 hash algorithm
- Hash size is fixed at 32 bytes (`HASH_SIZE`)
- Custom algorithms can be implemented via `hash_algo_t`

## Memory Management

The library manages memory carefully:
- All `*_new()` and `*_build()` functions allocate memory
- All `*_free()` functions must be called to prevent leaks
- `merkle_result_t` may contain allocated data that needs freeing
- Retrieved leaves from `cbmt_retrieve_leaves()` must be freed
- CBOR buffers from `*_to_cbor_buffer()` must be freed with `cbor_buffer_free()`

## CBOR Serialization Benefits

When libcbor is available, you get:
- **Binary Efficiency**: Hashes stored as raw bytes (~50% smaller than hex)
- **Type Safety**: Native support for binary data and integers
- **Standards Compliance**: RFC 7049 CBOR specification
- **Faster Parsing**: Binary format is faster than text-based formats

## Compatibility

This C implementation focuses on cryptographic hash values:
- Fixed 32-byte hash size for security and consistency
- SHA256 hash algorithm for robust cryptographic properties
- CBMT algorithm for efficient tree construction
- Optional CBOR serialization for efficient data exchange

## Testing

Run the test suite:
```bash
make test
```

The tests verify:
- Empty tree handling
- Single leaf trees  
- Multi-leaf trees
- Proof generation and verification
- Root computation
- Leaf retrieval
- Hash utility functions

All tests should pass, demonstrating proper hash-based operation.
- Root computation
- Leaf retrieval

All tests should pass, demonstrating compatibility with the Rust implementation.