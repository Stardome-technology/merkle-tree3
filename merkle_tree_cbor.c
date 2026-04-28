#ifdef WITH_CBOR

#include "merkle_tree_cbor.h"
#include "qcbor/qcbor.h"
#include "qcbor/qcbor_spiffy_decode.h"

#include <stdlib.h>
#include <string.h>

/*
 * Decode a secure_merkle_tree_t from a CBOR map already entered via
 * QCBORDecode_EnterMap / QCBORDecode_EnterMapFromMapN.
 */
static secure_merkle_tree_t *decode_secure_merkle_tree_from_map(
        QCBORDecodeContext *dc)
{
    uint64_t nodes_count_u64 = 0u;
    uint64_t tree_depth_u64 = 0u;

    QCBORDecode_GetUInt64InMapN(dc, CBOR_KEY_NODES_COUNT, &nodes_count_u64);
    QCBORDecode_GetUInt64InMapN(dc, CBOR_KEY_TREE_DEPTH, &tree_depth_u64);

    if (QCBORDecode_GetError(dc) != QCBOR_SUCCESS) {
        return NULL;
    }

    UsefulBufC algo_str = NULLUsefulBufC;
    QCBORDecode_GetTextStringInMapN(dc, CBOR_KEY_ALGORITHM, &algo_str);
    QCBORError algo_err = QCBORDecode_GetAndResetError(dc);
    if (algo_err != QCBOR_SUCCESS && algo_err != QCBOR_ERR_LABEL_NOT_FOUND) {
        QCBORDecode_SetError(dc, algo_err);
        return NULL;
    }

    if (nodes_count_u64 == 0u || nodes_count_u64 > 65535u) {
        QCBORDecode_SetError(dc, QCBOR_ERR_FIRST_USER_DEFINED);
        return NULL;
    }

    size_t nodes_count = (size_t)nodes_count_u64;

    secure_merkle_tree_t *tree = malloc(sizeof(secure_merkle_tree_t));
    if (tree == NULL) {
        QCBORDecode_SetError(dc, QCBOR_ERR_FIRST_USER_DEFINED);
        return NULL;
    }
    memset(tree, 0, sizeof(*tree));

    tree->nodes = malloc(nodes_count * sizeof(hash_t));
    if (tree->nodes == NULL) {
        free(tree);
        QCBORDecode_SetError(dc, QCBOR_ERR_FIRST_USER_DEFINED);
        return NULL;
    }

    tree->nodes_count = nodes_count;
    tree->tree_depth = (uint8_t)tree_depth_u64;

    if (algo_str.ptr != NULL && algo_str.len > 0u) {
        const char *sha256_max = "sha256-secure-max";
        const char *sha256_mod = "sha256-secure-moderate";

        if (algo_str.len == strlen(sha256_max) &&
            memcmp(algo_str.ptr, sha256_max, algo_str.len) == 0) {
            tree->algo = (secure_hash_algo_t *)&secure_sha256_max;
        } else if (algo_str.len == strlen(sha256_mod) &&
                   memcmp(algo_str.ptr, sha256_mod, algo_str.len) == 0) {
            tree->algo = (secure_hash_algo_t *)&secure_sha256_moderate;
        }
    }

    if (tree->algo == NULL) {
        tree->algo = (secure_hash_algo_t *)&secure_sha256_moderate;
    }

    QCBORDecode_EnterArrayFromMapN(dc, CBOR_KEY_NODES);
    if (QCBORDecode_GetError(dc) != QCBOR_SUCCESS) {
        free(tree->nodes);
        free(tree);
        return NULL;
    }

    size_t filled = 0u;
    while (filled < nodes_count) {
        QCBORItem node_item;
        QCBORDecode_VGetNext(dc, &node_item);
        if (QCBORDecode_GetError(dc) != QCBOR_SUCCESS) {
            break;
        }

        if (node_item.uDataType != QCBOR_TYPE_BYTE_STRING) {
            QCBORDecode_SetError(dc, QCBOR_ERR_UNEXPECTED_TYPE);
            break;
        }

        if (node_item.val.string.len != HASH_SIZE) {
            QCBORDecode_SetError(dc, QCBOR_ERR_UNEXPECTED_TYPE);
            break;
        }

        memcpy(tree->nodes[filled], node_item.val.string.ptr, HASH_SIZE);
        filled++;
    }
    QCBORDecode_ExitArray(dc);

    if (QCBORDecode_GetError(dc) != QCBOR_SUCCESS || filled != nodes_count) {
        if (QCBORDecode_GetError(dc) == QCBOR_SUCCESS) {
            QCBORDecode_SetError(dc, QCBOR_ERR_FIRST_USER_DEFINED);
        }
        free(tree->nodes);
        free(tree);
        return NULL;
    }

    QCBORDecode_EnterMapFromMapN(dc, CBOR_KEY_SECURITY_FLAGS);
    QCBORError flags_err = QCBORDecode_GetAndResetError(dc);
    if (flags_err == QCBOR_SUCCESS) {
        bool use_double_leaf = false;
        bool use_depth_prefix = false;
        bool use_node_prefix = false;

        QCBORDecode_GetBoolInMapN(dc, 1, &use_double_leaf);
        QCBORDecode_GetAndResetError(dc);
        QCBORDecode_GetBoolInMapN(dc, 2, &use_depth_prefix);
        QCBORDecode_GetAndResetError(dc);
        QCBORDecode_GetBoolInMapN(dc, 3, &use_node_prefix);
        QCBORDecode_GetAndResetError(dc);
        QCBORDecode_ExitMap(dc);

        tree->security_enabled = true;

        (void)use_double_leaf;
        (void)use_depth_prefix;
        (void)use_node_prefix;
    } else if (flags_err == QCBOR_ERR_LABEL_NOT_FOUND) {
        tree->security_enabled = false;
    } else {
        QCBORDecode_SetError(dc, flags_err);
        free(tree->nodes);
        free(tree);
        return NULL;
    }

    return tree;
}

secure_merkle_tree_t *secure_merkle_tree_decode(const uint8_t *buf,
                                                size_t buf_size)
{
    if (buf == NULL || buf_size == 0u) {
        return NULL;
    }

    QCBORDecodeContext dc;
    QCBORDecode_Init(&dc,
                     (UsefulBufC){.ptr = (const void *)buf, .len = buf_size},
                     QCBOR_DECODE_MODE_NORMAL);

    QCBORDecode_EnterMap(&dc, NULL);
    if (QCBORDecode_GetError(&dc) != QCBOR_SUCCESS) {
        return NULL;
    }

    secure_merkle_tree_t *tree = decode_secure_merkle_tree_from_map(&dc);

    QCBORDecode_ExitMap(&dc);
    QCBORError finish_err = QCBORDecode_Finish(&dc);
    if (tree != NULL && finish_err != QCBOR_SUCCESS) {
        free(tree->nodes);
        free(tree);
        return NULL;
    }

    return tree;
}

size_t secure_merkle_tree_encode(const secure_merkle_tree_t *tree,
                                 uint8_t *buf,
                                 size_t buf_size)
{
    if (tree == NULL || buf == NULL || buf_size == 0u) {
        return 0u;
    }

    QCBOREncodeContext enc;
    QCBOREncode_Init(&enc, (UsefulBuf){.ptr = buf, .len = buf_size});

    QCBOREncode_OpenMap(&enc);

    QCBOREncode_AddUInt64ToMapN(&enc, CBOR_KEY_VERSION,
                                (uint64_t)CBOR_MERKLE_TREE_VERSION_SECURE);
    QCBOREncode_AddUInt64ToMapN(&enc, CBOR_KEY_NODES_COUNT,
                                (uint64_t)tree->nodes_count);

    QCBOREncode_OpenArrayInMapN(&enc, CBOR_KEY_NODES);
    for (size_t i = 0u; i < tree->nodes_count; i++) {
        QCBOREncode_AddBytes(&enc,
                             (UsefulBufC){.ptr = tree->nodes[i],
                                          .len = HASH_SIZE});
    }
    QCBOREncode_CloseArray(&enc);

    const char *algo_name =
        (tree->algo != NULL && tree->algo->algo_name != NULL)
            ? tree->algo->algo_name
            : "unknown";
    QCBOREncode_AddSZStringToMapN(&enc, CBOR_KEY_ALGORITHM, algo_name);

    const uint32_t hash_size =
        (tree->algo != NULL) ? tree->algo->hash_size : (uint32_t)HASH_SIZE;
    QCBOREncode_AddUInt64ToMapN(&enc, CBOR_KEY_HASH_SIZE,
                                (uint64_t)hash_size);
    QCBOREncode_AddUInt64ToMapN(&enc, CBOR_KEY_TREE_DEPTH,
                                (uint64_t)tree->tree_depth);

    if (tree->security_enabled && tree->algo != NULL) {
        QCBOREncode_OpenMapInMapN(&enc, CBOR_KEY_SECURITY_FLAGS);
        QCBOREncode_AddBoolToMapN(&enc, 1, tree->algo->use_double_leaf_hash);
        QCBOREncode_AddBoolToMapN(&enc, 2, tree->algo->use_depth_prefix);
        QCBOREncode_AddBoolToMapN(&enc, 3, tree->algo->use_node_prefix);
        QCBOREncode_CloseMap(&enc);
    }

    QCBOREncode_CloseMap(&enc);

    UsefulBufC encoded;
    const QCBORError err = QCBOREncode_Finish(&enc, &encoded);
    return (err == QCBOR_SUCCESS) ? encoded.len : 0u;
}

size_t secure_merkle_proof_encode(const secure_merkle_proof_t *proof,
                                  uint8_t *buf,
                                  size_t buf_size)
{
    if (proof == NULL || buf == NULL || buf_size == 0u) {
        return 0u;
    }

    QCBOREncodeContext enc;
    QCBOREncode_Init(&enc, (UsefulBuf){.ptr = buf, .len = buf_size});

    QCBOREncode_OpenMap(&enc);

    QCBOREncode_AddUInt64ToMapN(&enc, CBOR_KEY_VERSION,
                                (uint64_t)CBOR_MERKLE_PROOF_VERSION_SECURE);

    QCBOREncode_OpenArrayInMapN(&enc, CBOR_KEY_INDICES);
    for (size_t i = 0u; i < proof->indices_count; i++) {
        QCBOREncode_AddUInt64(&enc, (uint64_t)proof->indices[i]);
    }
    QCBOREncode_CloseArray(&enc);

    QCBOREncode_OpenArrayInMapN(&enc, CBOR_KEY_LEMMAS);
    for (size_t i = 0u; i < proof->lemmas_count; i++) {
        QCBOREncode_AddBytes(&enc,
                             (UsefulBufC){.ptr = proof->lemmas[i],
                                          .len = HASH_SIZE});
    }
    QCBOREncode_CloseArray(&enc);

    QCBOREncode_AddUInt64ToMapN(&enc, CBOR_KEY_INDICES_COUNT,
                                (uint64_t)proof->indices_count);
    QCBOREncode_AddUInt64ToMapN(&enc, CBOR_KEY_LEMMAS_COUNT,
                                (uint64_t)proof->lemmas_count);
    QCBOREncode_AddUInt64ToMapN(&enc, CBOR_KEY_EXPECTED_DEPTH,
                                (uint64_t)proof->expected_depth);

    QCBOREncode_CloseMap(&enc);

    UsefulBufC encoded;
    const QCBORError err = QCBOREncode_Finish(&enc, &encoded);
    return (err == QCBOR_SUCCESS) ? encoded.len : 0u;
}

#endif // WITH_CBOR