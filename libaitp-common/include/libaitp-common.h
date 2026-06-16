/**
 * libaitp-common.h - Common utilities for AI-TP modules
 *
 * Shared functions extracted from duplicate implementations across modules.
 * Currently provides stub_hash() used by crypto/pow/zkp/consensus/scheduler.
 * Will be replaced with real libsodium/openssl in production.
 */

#ifndef LIBAITP_COMMON_H
#define LIBAITP_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stub hash function (placeholder until real crypto backend is linked)
 *
 * Produces deterministic output of out_len bytes from input data.
 * NOT cryptographically secure - for development/testing only.
 * Must be replaced with SHA-256/SHA-3 when libsodium or OpenSSL is integrated.
 *
 * @param data     Input data
 * @param len      Input data length
 * @param out      Output buffer
 * @param out_len  Output buffer size (must be > 0)
 */
void aitp_stub_hash(const uint8_t *data, size_t len, uint8_t *out, size_t out_len);

/**
 * @brief Constant-time memory comparison
 *
 * Returns 0 if equal, non-zero if different.
 * Execution time does not depend on the position of the first mismatch.
 *
 * @param a        First buffer
 * @param b        Second buffer
 * @param len      Number of bytes to compare
 * @return int     0 if equal, 1 if different
 */
int aitp_const_memcmp(const uint8_t *a, const uint8_t *b, size_t len);

/**
 * @brief Securely zero memory (compiler barrier)
 *
 * Uses volatile pointer to prevent compiler from optimizing away the memset.
 * For clearing sensitive data like private keys and session keys.
 *
 * @param buf  Buffer to clear
 * @param len  Number of bytes to clear
 */
void aitp_secure_zero(uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* LIBAITP_COMMON_H */
