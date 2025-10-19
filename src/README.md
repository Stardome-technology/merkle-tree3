# Merkle Tree C Implementation

This is a C implementation of the Rust merkle-tree3 library, providing Complete Binary Merkle Tree (CBMT) functionality.

## Features

- **Complete Binary Merkle Tree**: Efficient binary tree structure for merkle proofs
- **Generic Type Support**: Configurable through function pointers for different data types
- **Merkle Proofs**: Generate and verify proofs for sets of leaves
- **Memory Safe**: Proper memory management with cleanup functions
- **No Dependencies**: Pure C implementation with standard library only

## Files

- `merkle_tree.h` - Header file with all public APIs and structures
- `merkle_tree.c` - Implementation of all merkle tree functionality
- `test_merkle_tree.c` - Example usage and test cases
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

int main() {
    // Sample data
    int32_t leaves[] = {2, 3, 5, 7, 11};
    size_t leaves_count = 5;
    
    // Build merkle tree
    merkle_tree_t* tree = cbmt_build_merkle_tree(leaves, leaves_count, &int32_ops);
    
    // Get root
    int32_t root;
    merkle_tree_root(tree, &root);
    printf("Merkle root: %d\n", root);
    
    // Build proof for leaves at indices 0 and 3
    uint32_t leaf_indices[] = {0, 3};
    merkle_result_t proof_result = merkle_tree_build_proof(tree, leaf_indices, 2);
    
    if (proof_result.success) {
        merkle_proof_t* proof = proof_result.proof;
        
        // Verify proof
        int32_t proof_leaves[] = {2, 7}; // values at indices 0 and 3
        bool verified = merkle_proof_verify(proof, &root, proof_leaves, 2);
        printf("Proof verified: %s\n", verified ? "true" : "false");
        
        merkle_proof_free(proof);
    }
    
    merkle_tree_free(tree);
    return 0;
}
```

### Custom Data Types

To use custom data types, implement the required operations:

```c
// Example for custom struct
typedef struct {
    int x, y;
} point_t;

void merge_point(const void* left, const void* right, void* result) {
    const point_t* l = (const point_t*)left;
    const point_t* r = (const point_t*)right;
    point_t* res = (point_t*)result;
    res->x = l->x + r->x;
    res->y = l->y + r->y;
}

int compare_point(const void* a, const void* b) {
    const point_t* pa = (const point_t*)a;
    const point_t* pb = (const point_t*)b;
    if (pa->x != pb->x) return (pa->x < pb->x) ? -1 : 1;
    return (pa->y < pb->y) ? -1 : (pa->y > pb->y) ? 1 : 0;
}

void copy_point(const void* src, void* dst) {
    *(point_t*)dst = *(const point_t*)src;
}

void default_point(void* item) {
    point_t* p = (point_t*)item;
    p->x = 0;
    p->y = 0;
}

const type_ops_t point_ops = {
    .merge = merge_point,
    .compare = compare_point,
    .copy = copy_point,
    .default_init = default_point,
    .item_size = sizeof(point_t)
};
```

## API Reference

### Core Structures

- `merkle_tree_t` - Represents a complete binary merkle tree
- `merkle_proof_t` - Represents a merkle proof for a set of leaves
- `type_ops_t` - Function pointers for type-specific operations

### Main Functions

#### CBMT Operations
- `cbmt_build_merkle_tree()` - Build a complete merkle tree from leaves
- `cbmt_build_merkle_root()` - Calculate merkle root directly
- `cbmt_build_merkle_proof()` - Build a proof for specific leaf indices
- `cbmt_retrieve_leaves()` - Extract original leaves from a proof

#### Merkle Tree Operations
- `merkle_tree_new()` - Create empty merkle tree
- `merkle_tree_free()` - Free merkle tree memory
- `merkle_tree_build_proof()` - Generate proof for leaf indices
- `merkle_tree_root()` - Get tree root value

#### Merkle Proof Operations
- `merkle_proof_new()` - Create new proof from indices and lemmas
- `merkle_proof_free()` - Free proof memory
- `merkle_proof_verify()` - Verify proof against root and leaves
- `merkle_proof_root()` - Compute root from proof and leaves

### Built-in Type Support

The library includes built-in support for `int32_t`:
- `int32_ops` - Type operations for 32-bit integers
- Uses subtraction merge: `right - left` (matching Rust implementation)

## Memory Management

The library manages memory carefully:
- All `*_new()` functions allocate memory
- All `*_free()` functions must be called to prevent leaks
- `merkle_result_t` may contain allocated data that needs freeing
- Retrieved leaves from `cbmt_retrieve_leaves()` must be freed

## Compatibility

This C implementation maintains behavioral compatibility with the original Rust version:
- Same merge operation (right - left for int32)
- Same tree construction algorithm
- Same proof generation and verification logic
- Same test cases and expected results

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

All tests should pass, demonstrating compatibility with the Rust implementation.