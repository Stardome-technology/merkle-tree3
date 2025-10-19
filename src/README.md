# Merkle Tree C Implementation (Hash-Based)

This is a C implementation of a hash-based Complete Binary Merkle Tree (CBMT), designed specifically for cryptographic hash values.

## Features

- **Hash-Based Values Only**: Works exclusively with fixed-size hash values (32 bytes)
- **Complete Binary Merkle Tree**: Efficient binary tree structure for merkle proofs
- **Configurable Hash Algorithms**: Support for different hash functions via function pointers
- **Merkle Proofs**: Generate and verify proofs for sets of hash leaves
- **Memory Safe**: Proper memory management with cleanup functions
- **No Dependencies**: Pure C implementation with standard library only

## Hash Support

This implementation is restricted to working with hash values only:
- Fixed 32-byte hash size (configurable via `HASH_SIZE`)
- Hex string representation support (64 hex characters)
- Built-in SHA256 algorithm (simplified for demonstration)
- Extensible to other hash algorithms via function pointers

## Files

- `merkle_tree.h` - Header file with all public APIs and structures
- `merkle_tree.c` - Implementation of all merkle tree functionality
- `test_merkle_tree.c` - Example usage and test cases with hash values
- `merkle_tree_schema.json` - JSON schema for serializing hash-based trees
- `Makefile` - Build configuration

## Building

### Windows Batch Script (Easy)
```cmd
# Build and run (detects available compiler automatically)
build.bat
```

### Using Make (Linux/macOS/MinGW)
```bash
# Build and run tests
make test

# Build only
make all

# Build static library
make lib

# Clean build artifacts
make clean

# Debug build
make debug
```

### Using CMake (Cross-platform)
```bash
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

```

## Features

- **Hash-based Values**: Designed specifically for cryptographic hash values (32-byte fixed size)
- **Complete Binary Merkle Tree (CBMT)**: Implements the CBMT algorithm for efficient tree construction
- **Memory Management**: Automatic memory allocation and cleanup with proper error handling  
- **Proof Generation**: Build proofs for any subset of leaves
- **Proof Verification**: Verify proofs against merkle roots
- **Flexible Hash Algorithms**: Support for custom hash functions (SHA256 included by default)
- **JSON Schema**: Structured data format for serialization and interoperability
- **Cross-platform**: Works on Windows, Linux, and macOS with multiple build systems

## API Reference

### Core Structures

- `hash_t` - Fixed 32-byte hash value type
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

## Compatibility

This C implementation focuses on cryptographic hash values:
- Fixed 32-byte hash size for security and consistency
- SHA256 hash algorithm for robust cryptographic properties
- CBMT algorithm for efficient tree construction
- JSON schema support for data exchange

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