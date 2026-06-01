# Merkle Tree for Static Data

This repository contains a stripped-down C port/adaptation of the Nervos Network Complete Binary Merkle Tree (CBMT) implementation.

It is not a fork or port of QCBOR itself. Instead, this codebase adds optional QCBOR-based CBOR serialization on top of the C Merkle tree implementation when `WITH_CBOR` is enabled.

## Scope and Provenance

This repository is intentionally narrower than the original Rust upstream:

- The core CBMT algorithm is ported to C and exposed through function-pointer based hashing APIs.
- The code is adapted for embedded-oriented use, including caller-provided buffers for CBOR encoding.
- The repository includes security-hardened tree/proof variants in `merkle_tree_secure.c`.
- The repository includes QCBOR integration in `merkle_tree_cbor.c` and `merkle_tree_cbor.h`.
- It is stripped down compared with the Rust upstream: there is no trait-based generic API, fewer examples/tests, and manual memory management is used throughout.

## Complete Binary Merkle Tree

Complete Binary Merkle Tree (CBMT) is used to generate a Merkle root and Merkle proofs for a static list of items. A CBMT is a complete binary tree, in which every level, except possibly the last, is completely filled and all nodes are as far left as possible. It is also a full binary tree, in which every non-leaf node has two children. Compared with other Merkle tree layouts, CBMT minimizes hash computation and proof size.

## Node Organization

For illustration, tree nodes are ordered from top to bottom and left to right starting at zero. In a CBMT with `n` leaves, the root is node `0`, and leaf `i` is stored at index `i + n - 1` in the implicit array representation.

For example, CBMT with 6 items (hashes `[T0, T1, T2, T3, T4, T5]`) and CBMT with 7 items (`[T0, T1, T2, T3, T4, T5, T6]`) are shown below:

```text
        with 6 items                       with 7 items

              B0 -- node 0                       B0 -- node 0
             /  \                               /  \
           /      \                           /      \
         /          \                       /          \
       /              \                   /              \
      B1 -- node 1    B2 -- node 2       B1 -- node 1    B2 -- node 2
     /  \            /  \               /  \            /  \
    /    \          /    \             /    \          /    \
   /      \        /      \           /      \        /      \
  B3(3)   B4(4)  T0(5)    T1(6)      B3(3)   B4(4)   B5(5)   T0(6)
 /  \    /  \                       /  \    /  \    /  \
T2  T3  T4  T5                     T1  T2  T3  T4  T5  T6
(7) (8) (9) (10)                   (7) (8) (9)(10)(11) (12)
```

For a tree with `n` items, the array size is `2n - 1`. For a node at index `i`:

- Parent index: `(i - 1) / 2`
- Sibling index: `((i + 1) ^ 1) - 1`
- Children indices: `[2i + 1, 2i + 2]`

## Tree Layout in Memory

CBMT is represented compactly using an array of hashes stored in ascending node order.

```text
[B0, B1, B2, B3, B4, T0, T1, T2, T3, T4, T5]
[B0, B1, B2, B3, B4, B5, T0, T1, T2, T3, T4, T5, T6]
```

In this C port, hash values are fixed-size 32-byte arrays (`hash_t`), and tree/proof APIs are declared in `merkle_tree.h`.

## Merkle Proofs

Merkle proofs provide proof of inclusion for one or more items. Only siblings along the leaf-to-root paths that are not already on the proof path are included. Proof lemmas are stored in descending tree order.

The codebase exposes two proof models:

- Legacy proofs use CBMT node indices.
- Secure proofs use leaf-layer indices and carry an `expected_depth` value for validation.

This distinction is part of the versioned CBOR schema and is documented in `merkle_tree.h`.

## Optional SHA-256 Adapter

The core library is hash-agnostic and does not depend on any specific SHA-256 implementation.

If you want the convenience symbols `sha256_algo`, `secure_sha256_max`, and `secure_sha256_moderate`, compile `merkle_tree_sha256.c` and provide a project-local implementation of `merkle_tree_sha256_digest()` declared in `merkle_tree_sha256_backend.h`.

## CBOR Serialization and CDDL Compliance

The normative schema for serialized trees and proofs is defined in `stardome-merkle-tree.cddl`.

This README summarizes that schema, but the CDDL file remains the source of truth for field-level requirements.

### Merkle Tree Objects

Serialized Merkle tree objects use integer CBOR keys:

- `1`: version
- `2`: nodes array
- `3`: nodes count
- `4`: algorithm name
- `5`: hash size
- `6`: tree depth
- `7`: security flags

Schema versions:

- Version `1` is the legacy tree format.
- Version `2` and later identify secure trees.
- In secure tree objects, `tree_depth` is required by the schema.
- `security_flags` is optional and records whether double leaf hashing, depth prefixing, and node prefixing are enabled.

Hashes are serialized as raw 32-byte byte strings, not hex strings.

### Merkle Proof Objects

Serialized Merkle proof objects also use integer keys, with overlapping key numbers in a different object type:

- `1`: version
- `2`: indices array
- `3`: lemmas array
- `4`: indices count
- `5`: lemmas count
- `8`: expected depth

Schema versions:

- Version `1` is the legacy proof format.
- Version `2` and later identify secure proofs.
- Secure proofs are defined as single-leaf proofs in the schema.
- In secure proofs, `indices_count` must be `1` and the indices array must contain exactly one leaf index.
- In secure proofs, `expected_depth` is required by the schema.
- The verifier expects `lemmas_count == expected_depth` for secure proofs.

### Current C Implementation Status

The public CBOR API is declared in `merkle_tree_cbor.h`.

- `secure_merkle_tree_encode()` writes a secure tree CBOR map into a caller-supplied buffer.
- `secure_merkle_proof_encode()` writes a secure proof CBOR map into a caller-supplied buffer.
- `secure_merkle_tree_decode()` decodes a serialized tree into a heap-allocated `secure_merkle_tree_t`.

The current encode path emits secure-version objects and includes the secure fields used by the implementation. For schema-compliant secure proof serialization, callers should serialize single-leaf proofs only. The current decode path is implemented for secure tree objects. If you need additional legacy object handling or proof decoding, treat the CDDL as the compatibility target and the current C code as the implemented subset.

### Security Algorithms

`merkle_tree_secure.c` currently defines two built-in secure algorithm profiles:

- `sha256-secure-max`
- `sha256-secure-moderate`

These profiles control the security flags summarized in the schema:

- Double leaf hashing
- Depth prefixing
- Leaf/internal node prefixing

## C API Overview

The main public headers are:

- `merkle_tree.h` for tree, proof, and secure tree/proof types and APIs.
- `merkle_tree_cbor.h` for CBOR key definitions and encode/decode functions.

Typical operations include:

- Build a legacy tree with `cbmt_build_merkle_tree()`.
- Build a secure tree with `secure_cbmt_build_merkle_tree()`.
- Build a secure proof with `secure_merkle_tree_build_proof()`.
- Verify a secure single-leaf proof with `secure_merkle_proof_verify_single()`.

## Buffer Sizing Notes

The CBOR header documents approximate output sizing for caller-provided buffers:

- Tree encoding: about `64 + nodes_count * 34` bytes
- Proof encoding: about `64 + lemmas_count * 34` bytes

These are upper-bound planning estimates, not a substitute for validating actual encoded output sizes in your application.

## Limitations

- This repository is a C adaptation, not the original Rust library.
- The README summarizes the schema but does not replace `stardome-merkle-tree.cddl`.
- The current C implementation exposes only the CBOR operations implemented in the shipped source files; do not assume full schema coverage unless the corresponding API exists.

# License

This repository contains code derived from the Nervos Network merkle-tree implementation, which is licensed under the MIT License.

Modifications and additions in this fork are Copyright 2026 Stardome SAGL.

Unless otherwise noted, code derived from the upstream implementation remains subject to the MIT License in the repository LICENSE file.

Original Stardome-authored additions may also be made available under the Apache License, Version 2.0. Where that applies, it should be stated explicitly in the relevant files or accompanying license notice.

Stardome and related names may be trademarks of their respective owners. No trademark rights are granted by this repository license.