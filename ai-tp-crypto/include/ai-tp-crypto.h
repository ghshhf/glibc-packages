/**
 * @file ai-tp-crypto.h
 * @brief AI-TP OS cryptographic primitives API
 * @version 1.0.0
 * @date 2026-06-16
 *
 * Network Layer Security Stack - Layer 2: Encryption
 *
 * 1. Key pair generation (Ed25519 signing, X25519 key exchange)
 * 2. ECDH key agreement -> symmetric session key
 * 3. Symmetric encryption (AES-256-GCM / ChaCha20-Poly1305)
 * 4. Forward secrecy (simplified ratchet)
 * 5. Digital signature / verification (Ed25519)
 * 6. HMAC message authentication
 * 7. Hash functions (SHA-256, SHA-512)
 * 8. Secure random number generation
 *
 * Production: link against libsodium or OpenSSL
 * This header provides a stable ABI; backends are swappable
 */

#ifndef AI_TP_CRYPTO_H
#define AI_TP_CRYPTO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AI_TP_CRYPTO_VERSION_MAJOR 1
#define AI_TP_CRYPTO_VERSION_MINOR 0
#define AI_TP_CRYPTO_VERSION_PATCH 0

/* Key sizes */
#define AI_TP_CRYPTO_PUBLIC_KEY_BYTES   32
#define AI_TP_CRYPTO_SECRET_KEY_BYTES   32
#define AI_TP_CRYPTO_SEED_BYTES         32

/* Signature sizes */
#define AI_TP_CRYPTO_SIGN_BYTES         64

/* Symmetric key sizes */
#define AI_TP_CRYPTO_SESSION_KEY_BYTES  32
#define AI_TP_CRYPTO_SYM_KEY_BYTES      32

/* Nonce / IV sizes */
#define AI_TP_CRYPTO_NONCE_BYTES        12
#define AI_TP_CRYPTO_SALT_BYTES         16

/* Hash sizes */
#define AI_TP_CRYPTO_SHA256_BYTES       32
#define AI_TP_CRYPTO_SHA512_BYTES       64
#define AI_TP_CRYPTO_HASH_MAX_BYTES     64

/* HMAC sizes */
#define AI_TP_CRYPTO_HMAC_KEY_BYTES     32
#define AI_TP_CRYPTO_HMAC_BYTES         32

/* AEAD tag sizes */
#define AI_TP_CRYPTO_AEAD_TAG_BYTES     16

/* Ratchet */
#define AI_TP_CRYPTO_RATCHET_CHAIN_KEY_BYTES  32
#define AI_TP_CRYPTO_RATCHET_MAX_SKIP         64

/* Error codes */
#define AI_TP_CRYPTO_OK              0
#define AI_TP_CRYPTO_ERR_INIT       -1
#define AI_TP_CRYPTO_ERR_KEYGEN     -2
#define AI_TP_CRYPTO_ERR_ENCRYPT    -3
#define AI_TP_CRYPTO_ERR_DECRYPT    -4
#define AI_TP_CRYPTO_ERR_SIGN       -5
#define AI_TP_CRYPTO_ERR_VERIFY     -6
#define AI_TP_CRYPTO_ERR_RNG        -7
#define AI_TP_CRYPTO_ERR_INVALID    -8
#define AI_TP_CRYPTO_ERR_BUFFER     -9
#define AI_TP_CRYPTO_ERR_RATCHET   -10

typedef enum {
    AI_TP_CRYPTO_SYM_AES256GCM = 0,
    AI_TP_CRYPTO_SYM_CHACHA20POLY1305 = 1
} ai_tp_crypto_sym_alg_t;

typedef enum {
    AI_TP_CRYPTO_HASH_SHA256 = 0,
    AI_TP_CRYPTO_HASH_SHA512 = 1
} ai_tp_crypto_hash_alg_t;

typedef struct {
    uint8_t pk[AI_TP_CRYPTO_PUBLIC_KEY_BYTES];
} ai_tp_crypto_public_key_t;

typedef struct {
    uint8_t sk[AI_TP_CRYPTO_SECRET_KEY_BYTES];
} ai_tp_crypto_secret_key_t;

typedef struct {
    ai_tp_crypto_public_key_t  pub;
    ai_tp_crypto_secret_key_t  sec;
} ai_tp_crypto_keypair_t;

typedef struct {
    uint8_t data[AI_TP_CRYPTO_SIGN_BYTES];
} ai_tp_crypto_signature_t;

typedef struct {
    uint8_t bytes[AI_TP_CRYPTO_NONCE_BYTES];
} ai_tp_crypto_nonce_t;

typedef struct {
    uint8_t bytes[AI_TP_CRYPTO_SALT_BYTES];
} ai_tp_crypto_salt_t;

typedef struct {
    uint8_t bytes[AI_TP_CRYPTO_SESSION_KEY_BYTES];
} ai_tp_crypto_session_key_t;

typedef struct {
    uint8_t bytes[AI_TP_CRYPTO_SYM_KEY_BYTES];
} ai_tp_crypto_sym_key_t;

typedef struct {
    uint8_t bytes[AI_TP_CRYPTO_HMAC_KEY_BYTES];
} ai_tp_crypto_hmac_key_t;

typedef struct {
    uint8_t bytes[AI_TP_CRYPTO_HMAC_BYTES];
} ai_tp_crypto_hmac_t;

typedef struct {
    uint8_t bytes[AI_TP_CRYPTO_SHA256_BYTES];
} ai_tp_crypto_sha256_t;

typedef struct {
    uint8_t bytes[AI_TP_CRYPTO_SHA512_BYTES];
} ai_tp_crypto_sha512_t;

typedef struct {
    uint8_t bytes[AI_TP_CRYPTO_AEAD_TAG_BYTES];
} ai_tp_crypto_aead_tag_t;

/* Simplified Double Ratchet state for forward secrecy */
typedef struct {
    ai_tp_crypto_keypair_t     identity;
    ai_tp_crypto_keypair_t     ephemeral;
    ai_tp_crypto_public_key_t  remote_identity;
    ai_tp_crypto_public_key_t  remote_ephemeral;
    uint8_t root_key[AI_TP_CRYPTO_RATCHET_CHAIN_KEY_BYTES];
    uint8_t send_chain[AI_TP_CRYPTO_RATCHET_CHAIN_KEY_BYTES];
    uint8_t recv_chain[AI_TP_CRYPTO_RATCHET_CHAIN_KEY_BYTES];
    uint64_t send_count;
    uint64_t recv_count;
    uint32_t skipped_keys;
    bool initialized;
} ai_tp_crypto_ratchet_t;

/* ============================================================
 * Core API
 * ============================================================ */

/* Initialization / Cleanup */
int ai_tp_crypto_init(void);
void ai_tp_crypto_shutdown(void);

/* Random Number Generation */
int ai_tp_crypto_random_bytes(uint8_t *buf, size_t len);
int ai_tp_crypto_random_nonce(ai_tp_crypto_nonce_t *nonce);
int ai_tp_crypto_random_salt(ai_tp_crypto_salt_t *salt);

/* Key Generation */
int ai_tp_crypto_sign_keypair(ai_tp_crypto_keypair_t *kp);
int ai_tp_crypto_sign_keypair_from_seed(ai_tp_crypto_keypair_t *kp,
                                         const uint8_t seed[AI_TP_CRYPTO_SEED_BYTES]);
int ai_tp_crypto_kx_keypair(ai_tp_crypto_keypair_t *kp);
int ai_tp_crypto_kx_keypair_from_sign(ai_tp_crypto_keypair_t *kx_kp,
                                        const ai_tp_crypto_keypair_t *sign_kp);

/* Key Exchange (ECDH) */
int ai_tp_crypto_kx_agree(ai_tp_crypto_session_key_t *shared,
                            const ai_tp_crypto_secret_key_t *my_sec,
                            const ai_tp_crypto_public_key_t *their_pub);
int ai_tp_crypto_derive_session_key(ai_tp_crypto_session_key_t *session_key,
                                     const ai_tp_crypto_salt_t *salt,
                                     const ai_tp_crypto_session_key_t *dh_identity,
                                     const ai_tp_crypto_session_key_t *dh_ephemeral);

/* Symmetric Encryption (AEAD) */
int ai_tp_crypto_aead_encrypt(uint8_t *ciphertext, size_t *ct_len,
                               const uint8_t *plaintext, size_t pt_len,
                               const uint8_t *aad, size_t aad_len,
                               const ai_tp_crypto_nonce_t *nonce,
                               const ai_tp_crypto_sym_key_t *key,
                               ai_tp_crypto_sym_alg_t alg,
                               ai_tp_crypto_aead_tag_t *tag);
int ai_tp_crypto_aead_decrypt(uint8_t *plaintext, size_t *pt_len,
                               const uint8_t *ciphertext, size_t ct_len,
                               const uint8_t *aad, size_t aad_len,
                               const ai_tp_crypto_nonce_t *nonce,
                               const ai_tp_crypto_sym_key_t *key,
                               const ai_tp_crypto_aead_tag_t *tag,
                               ai_tp_crypto_sym_alg_t alg);

/* Digital Signatures (Ed25519) */
int ai_tp_crypto_sign(ai_tp_crypto_signature_t *sig,
                       const uint8_t *message, size_t msg_len,
                       const ai_tp_crypto_secret_key_t *sk);
bool ai_tp_crypto_sign_verify(const ai_tp_crypto_signature_t *sig,
                               const uint8_t *message, size_t msg_len,
                               const ai_tp_crypto_public_key_t *pk);

/* Hash Functions */
int ai_tp_crypto_sha256(ai_tp_crypto_sha256_t *out, const uint8_t *data, size_t len);
int ai_tp_crypto_sha512(ai_tp_crypto_sha512_t *out, const uint8_t *data, size_t len);
int ai_tp_crypto_hash(uint8_t *out, size_t out_len, const uint8_t *data, size_t data_len,
                       ai_tp_crypto_hash_alg_t alg);

/* HMAC */
int ai_tp_crypto_hmac(ai_tp_crypto_hmac_t *out, const ai_tp_crypto_hmac_key_t *key,
                       const uint8_t *data, size_t data_len);
bool ai_tp_crypto_hmac_verify(const ai_tp_crypto_hmac_t *expected, const ai_tp_crypto_hmac_t *actual);

/* Forward Secrecy (Simplified Ratchet) */
int ai_tp_crypto_ratchet_init_initiator(ai_tp_crypto_ratchet_t *ratchet,
    const ai_tp_crypto_keypair_t *my_identity,
    const ai_tp_crypto_keypair_t *my_ephemeral,
    const ai_tp_crypto_public_key_t *remote_identity);
int ai_tp_crypto_ratchet_init_responder(ai_tp_crypto_ratchet_t *ratchet,
    const ai_tp_crypto_keypair_t *my_identity,
    const ai_tp_crypto_public_key_t *remote_identity,
    const ai_tp_crypto_public_key_t *remote_ephemeral);
int ai_tp_crypto_ratchet_send(ai_tp_crypto_sym_key_t *msg_key, ai_tp_crypto_ratchet_t *ratchet);
int ai_tp_crypto_ratchet_recv(ai_tp_crypto_sym_key_t *msg_key, ai_tp_crypto_ratchet_t *ratchet);
void ai_tp_crypto_ratchet_destroy(ai_tp_crypto_ratchet_t *ratchet);

/* Utility */
bool ai_tp_crypto_mem_eq(const void *a, const void *b, size_t len);
void ai_tp_crypto_mem_zero(void *buf, size_t len);
int ai_tp_crypto_pubkey_to_address(char *buf, size_t buf_len, const ai_tp_crypto_public_key_t *pk);
const char *ai_tp_crypto_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_CRYPTO_H */

