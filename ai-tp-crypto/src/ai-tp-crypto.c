/**
 * @file ai-tp-crypto.c
 * @brief AI-TP OS cryptographic primitives implementation
 * @version 1.0.0
 *
 * NOTE: This is a reference implementation with stub crypto operations.
 * Production deployments MUST link against libsodium or OpenSSL.
 * Stub functions use deterministic placeholders for API validation.
 */

#include "ai-tp-crypto.h"
#include "libaitp-common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ---- Internal state ---- */
static bool s_initialized = false;

/* Simple LFSR-based PRNG for stub (NOT cryptographically secure) */
static uint64_t s_prng_state = 0;

static void prng_seed(void) {
    s_prng_state = (uint64_t)time(NULL) ^ ((uint64_t)clock() << 32);
    if (s_prng_state == 0) s_prng_state = 0x123456789ABCDEF0ULL;
}

static uint8_t prng_next_byte(void) {
    /* xorshift64 */
    uint64_t x = s_prng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    s_prng_state = x;
    return (uint8_t)(x & 0xFF);
}

/* ---- Initialization / Cleanup ---- */

int ai_tp_crypto_init(void) {
    if (s_initialized) return AI_TP_CRYPTO_OK;
    prng_seed();
    s_initialized = true;
    return AI_TP_CRYPTO_OK;
}

void ai_tp_crypto_shutdown(void) {
    s_initialized = false;
    ai_tp_crypto_mem_zero(&s_prng_state, sizeof(s_prng_state));
}

/* ---- Random Number Generation ---- */

int ai_tp_crypto_random_bytes(uint8_t *buf, size_t len) {
    if (!buf || len == 0) return AI_TP_CRYPTO_ERR_INVALID;
    if (!s_initialized) return AI_TP_CRYPTO_ERR_INIT;
    for (size_t i = 0; i < len; i++) {
        buf[i] = prng_next_byte();
    }
    return AI_TP_CRYPTO_OK;
}

int ai_tp_crypto_random_nonce(ai_tp_crypto_nonce_t *nonce) {
    if (!nonce) return AI_TP_CRYPTO_ERR_INVALID;
    return ai_tp_crypto_random_bytes(nonce->bytes, AI_TP_CRYPTO_NONCE_BYTES);
}

int ai_tp_crypto_random_salt(ai_tp_crypto_salt_t *salt) {
    if (!salt) return AI_TP_CRYPTO_ERR_INVALID;
    return ai_tp_crypto_random_bytes(salt->bytes, AI_TP_CRYPTO_SALT_BYTES);
}

/* ---- Key Generation ---- */

int ai_tp_crypto_sign_keypair(ai_tp_crypto_keypair_t *kp) {
    if (!kp) return AI_TP_CRYPTO_ERR_KEYGEN;
    if (!s_initialized) return AI_TP_CRYPTO_ERR_INIT;
    /* Stub: generate deterministic-looking keys from PRNG */
    ai_tp_crypto_random_bytes(kp->sec.sk, AI_TP_CRYPTO_SECRET_KEY_BYTES);
    /* Public key = hash of secret key (stub: copy + transform) */
    for (int i = 0; i < AI_TP_CRYPTO_PUBLIC_KEY_BYTES; i++) {
        kp->pub.pk[i] = kp->sec.sk[i] ^ (uint8_t)(i * 17 + 0x5A);
    }
    return AI_TP_CRYPTO_OK;
}

int ai_tp_crypto_sign_keypair_from_seed(ai_tp_crypto_keypair_t *kp,
                                         const uint8_t seed[AI_TP_CRYPTO_SEED_BYTES]) {
    if (!kp || !seed) return AI_TP_CRYPTO_ERR_KEYGEN;
    /* Deterministic: derive from seed */
    for (int i = 0; i < AI_TP_CRYPTO_SECRET_KEY_BYTES; i++) {
        kp->sec.sk[i] = seed[i % AI_TP_CRYPTO_SEED_BYTES] ^ (uint8_t)(i * 37);
    }
    for (int i = 0; i < AI_TP_CRYPTO_PUBLIC_KEY_BYTES; i++) {
        kp->pub.pk[i] = kp->sec.sk[i] ^ (uint8_t)(i * 17 + 0x5A);
    }
    return AI_TP_CRYPTO_OK;
}

int ai_tp_crypto_kx_keypair(ai_tp_crypto_keypair_t *kp) {
    /* X25519 keypair generation (stub: same structure as signing) */
    return ai_tp_crypto_sign_keypair(kp);
}

int ai_tp_crypto_kx_keypair_from_sign(ai_tp_crypto_keypair_t *kx_kp,
                                        const ai_tp_crypto_keypair_t *sign_kp) {
    if (!kx_kp || !sign_kp) return AI_TP_CRYPTO_ERR_KEYGEN;
    /* Stub: Ed25519->X25519 conversion (in production: crypto_sign_ed25519_pk_to_curve25519) */
    for (int i = 0; i < AI_TP_CRYPTO_SECRET_KEY_BYTES; i++) {
        kx_kp->sec.sk[i] = sign_kp->sec.sk[i] ^ 0x55;
    }
    for (int i = 0; i < AI_TP_CRYPTO_PUBLIC_KEY_BYTES; i++) {
        kx_kp->pub.pk[i] = sign_kp->pub.pk[i] ^ 0xAA;
    }
    return AI_TP_CRYPTO_OK;
}

/* ---- Key Exchange (ECDH) ---- */

int ai_tp_crypto_kx_agree(ai_tp_crypto_session_key_t *shared,
                            const ai_tp_crypto_secret_key_t *my_sec,
                            const ai_tp_crypto_public_key_t *their_pub) {
    if (!shared || !my_sec || !their_pub) return AI_TP_CRYPTO_ERR_INVALID;
    /* Stub: XOR of secret and public key (NOT real ECDH!) */
    for (int i = 0; i < AI_TP_CRYPTO_SESSION_KEY_BYTES; i++) {
        shared->bytes[i] = my_sec->sk[i] ^ their_pub->pk[i];
    }
    return AI_TP_CRYPTO_OK;
}

int ai_tp_crypto_derive_session_key(ai_tp_crypto_session_key_t *session_key,
                                     const ai_tp_crypto_salt_t *salt,
                                     const ai_tp_crypto_session_key_t *dh_identity,
                                     const ai_tp_crypto_session_key_t *dh_ephemeral) {
    if (!session_key || !dh_identity || !dh_ephemeral) return AI_TP_CRYPTO_ERR_INVALID;
    /* Stub: HKDF(salt, dh1 || dh2) - simplified as XOR chain */
    for (int i = 0; i < AI_TP_CRYPTO_SESSION_KEY_BYTES; i++) {
        uint8_t mix = dh_identity->bytes[i] ^ dh_ephemeral->bytes[i];
        if (salt) mix ^= salt->bytes[i % AI_TP_CRYPTO_SALT_BYTES];
        session_key->bytes[i] = mix ^ (uint8_t)(i * 31);
    }
    return AI_TP_CRYPTO_OK;
}

/* ---- Symmetric Encryption (AEAD) ---- */

int ai_tp_crypto_aead_encrypt(uint8_t *ciphertext, size_t *ct_len,
                               const uint8_t *plaintext, size_t pt_len,
                               const uint8_t *aad, size_t aad_len,
                               const ai_tp_crypto_nonce_t *nonce,
                               const ai_tp_crypto_sym_key_t *key,
                               ai_tp_crypto_sym_alg_t alg,
                               ai_tp_crypto_aead_tag_t *tag) {
    (void)alg; (void)aad; (void)aad_len;
    if (!ciphertext || !plaintext || !nonce || !key || !tag) return AI_TP_CRYPTO_ERR_ENCRYPT;
    if (!s_initialized) return AI_TP_CRYPTO_ERR_INIT;
    /* Stub: XOR cipher with key stream from nonce+key */
    for (size_t i = 0; i < pt_len; i++) {
        uint8_t k = key->bytes[i % AI_TP_CRYPTO_SYM_KEY_BYTES] ^ nonce->bytes[i % AI_TP_CRYPTO_NONCE_BYTES];
        ciphertext[i] = plaintext[i] ^ k ^ (uint8_t)(i & 0xFF);
    }
    if (ct_len) *ct_len = pt_len;
    /* Stub tag: hash of key+nonce+plaintext_len */
    memset(tag->bytes, 0, AI_TP_CRYPTO_AEAD_TAG_BYTES);
    for (int i = 0; i < AI_TP_CRYPTO_AEAD_TAG_BYTES; i++) {
        tag->bytes[i] = key->bytes[i % AI_TP_CRYPTO_SYM_KEY_BYTES] ^ nonce->bytes[i % AI_TP_CRYPTO_NONCE_BYTES] ^ (uint8_t)(pt_len + i);
    }
    return AI_TP_CRYPTO_OK;
}

int ai_tp_crypto_aead_decrypt(uint8_t *plaintext, size_t *pt_len,
                               const uint8_t *ciphertext, size_t ct_len,
                               const uint8_t *aad, size_t aad_len,
                               const ai_tp_crypto_nonce_t *nonce,
                               const ai_tp_crypto_sym_key_t *key,
                               const ai_tp_crypto_aead_tag_t *tag,
                               ai_tp_crypto_sym_alg_t alg) {
    (void)alg; (void)aad; (void)aad_len;
    if (!plaintext || !ciphertext || !nonce || !key || !tag) return AI_TP_CRYPTO_ERR_DECRYPT;
    /* Verify tag (stub) */
    ai_tp_crypto_aead_tag_t expected;
    memset(expected.bytes, 0, AI_TP_CRYPTO_AEAD_TAG_BYTES);
    for (int i = 0; i < AI_TP_CRYPTO_AEAD_TAG_BYTES; i++) {
        expected.bytes[i] = key->bytes[i % AI_TP_CRYPTO_SYM_KEY_BYTES] ^ nonce->bytes[i % AI_TP_CRYPTO_NONCE_BYTES] ^ (uint8_t)(ct_len + i);
    }
    if (!ai_tp_crypto_mem_eq(tag->bytes, expected.bytes, AI_TP_CRYPTO_AEAD_TAG_BYTES)) {
        return AI_TP_CRYPTO_ERR_DECRYPT;
    }
    /* XOR decrypt */
    for (size_t i = 0; i < ct_len; i++) {
        uint8_t k = key->bytes[i % AI_TP_CRYPTO_SYM_KEY_BYTES] ^ nonce->bytes[i % AI_TP_CRYPTO_NONCE_BYTES];
        plaintext[i] = ciphertext[i] ^ k ^ (uint8_t)(i & 0xFF);
    }
    if (pt_len) *pt_len = ct_len;
    return AI_TP_CRYPTO_OK;
}

/* ---- Digital Signatures (Ed25519) ---- */

int ai_tp_crypto_sign(ai_tp_crypto_signature_t *sig,
                       const uint8_t *message, size_t msg_len,
                       const ai_tp_crypto_secret_key_t *sk) {
    if (!sig || !message || !sk) return AI_TP_CRYPTO_ERR_SIGN;
    /* Stub: deterministic signature from secret key + message hash */
    for (int i = 0; i < AI_TP_CRYPTO_SIGN_BYTES; i++) {
        sig->data[i] = sk->sk[i % AI_TP_CRYPTO_SECRET_KEY_BYTES] ^ (uint8_t)(msg_len + i * 7);
        if (msg_len > 0) sig->data[i] ^= message[i % msg_len];
    }
    return AI_TP_CRYPTO_OK;
}

bool ai_tp_crypto_sign_verify(const ai_tp_crypto_signature_t *sig,
                               const uint8_t *message, size_t msg_len,
                               const ai_tp_crypto_public_key_t *pk) {
    if (!sig || !message || !pk) return false;
    /* Stub: reconstruct expected signature from public key */
    uint8_t expected[AI_TP_CRYPTO_SIGN_BYTES];
    for (int i = 0; i < AI_TP_CRYPTO_SIGN_BYTES; i++) {
        expected[i] = pk->pk[i % AI_TP_CRYPTO_PUBLIC_KEY_BYTES] ^ (uint8_t)(msg_len + i * 7 + 0x5A);
        if (msg_len > 0) expected[i] ^= message[i % msg_len];
    }
    return ai_tp_crypto_mem_eq(sig->data, expected, AI_TP_CRYPTO_SIGN_BYTES);
}

/* ---- Hash Functions ---- */

int ai_tp_crypto_sha256(ai_tp_crypto_sha256_t *out, const uint8_t *data, size_t len) {
    if (!out || !data) return AI_TP_CRYPTO_ERR_INVALID;
    aitp_stub_hash(data, len, out->bytes, AI_TP_CRYPTO_SHA256_BYTES);
    return AI_TP_CRYPTO_OK;
}

int ai_tp_crypto_sha512(ai_tp_crypto_sha512_t *out, const uint8_t *data, size_t len) {
    if (!out || !data) return AI_TP_CRYPTO_ERR_INVALID;
    aitp_stub_hash(data, len, out->bytes, AI_TP_CRYPTO_SHA512_BYTES);
    return AI_TP_CRYPTO_OK;
}

int ai_tp_crypto_hash(uint8_t *out, size_t out_len, const uint8_t *data, size_t data_len,
                       ai_tp_crypto_hash_alg_t alg) {
    if (!out || !data) return AI_TP_CRYPTO_ERR_INVALID;
    if (alg == AI_TP_CRYPTO_HASH_SHA256 && out_len >= AI_TP_CRYPTO_SHA256_BYTES) {
        ai_tp_crypto_sha256_t h;
        ai_tp_crypto_sha256(&h, data, data_len);
        memcpy(out, h.bytes, AI_TP_CRYPTO_SHA256_BYTES);
        return AI_TP_CRYPTO_OK;
    }
    if (alg == AI_TP_CRYPTO_HASH_SHA512 && out_len >= AI_TP_CRYPTO_SHA512_BYTES) {
        ai_tp_crypto_sha512_t h;
        ai_tp_crypto_sha512(&h, data, data_len);
        memcpy(out, h.bytes, AI_TP_CRYPTO_SHA512_BYTES);
        return AI_TP_CRYPTO_OK;
    }
    return AI_TP_CRYPTO_ERR_INVALID;
}

/* ---- HMAC ---- */

int ai_tp_crypto_hmac(ai_tp_crypto_hmac_t *out, const ai_tp_crypto_hmac_key_t *key,
                       const uint8_t *data, size_t data_len) {
    if (!out || !key || !data) return AI_TP_CRYPTO_ERR_INVALID;
    /* Stub HMAC: hash(key || data) */
    uint8_t combined[AI_TP_CRYPTO_HMAC_KEY_BYTES + 256];
    size_t cplen = AI_TP_CRYPTO_HMAC_KEY_BYTES;
    memcpy(combined, key->bytes, AI_TP_CRYPTO_HMAC_KEY_BYTES);
    size_t copy_len = data_len < 256 ? data_len : 256;
    memcpy(combined + cplen, data, copy_len);
    cplen += copy_len;
    aitp_stub_hash(combined, cplen, out->bytes, AI_TP_CRYPTO_HMAC_BYTES);
    return AI_TP_CRYPTO_OK;
}

bool ai_tp_crypto_hmac_verify(const ai_tp_crypto_hmac_t *expected, const ai_tp_crypto_hmac_t *actual) {
    if (!expected || !actual) return false;
    return ai_tp_crypto_mem_eq(expected->bytes, actual->bytes, AI_TP_CRYPTO_HMAC_BYTES);
}

/* ---- Forward Secrecy (Simplified Ratchet) ---- */

static void ratchet_kdf(uint8_t *out, const uint8_t *key, const uint8_t *input, size_t input_len) {
    /* Stub KDF: hash(key || input) */
    uint8_t combined[32 + 256];
    memcpy(combined, key, 32);
    size_t copy_len = input_len < 256 ? input_len : 256;
    memcpy(combined + 32, input, copy_len);
    aitp_stub_hash(combined, 32 + copy_len, out, 32);
}

int ai_tp_crypto_ratchet_init_initiator(ai_tp_crypto_ratchet_t *ratchet,
    const ai_tp_crypto_keypair_t *my_identity,
    const ai_tp_crypto_keypair_t *my_ephemeral,
    const ai_tp_crypto_public_key_t *remote_identity) {
    if (!ratchet || !my_identity || !my_ephemeral || !remote_identity) return AI_TP_CRYPTO_ERR_RATCHET;
    memset(ratchet, 0, sizeof(*ratchet));
    ratchet->identity = *my_identity;
    ratchet->ephemeral = *my_ephemeral;
    ratchet->remote_identity = *remote_identity;

    /* DH: identity + ephemeral */
    ai_tp_crypto_session_key_t dh_id, dh_eph;
    ai_tp_crypto_kx_agree(&dh_id, &my_identity->sec, remote_identity);
    ai_tp_crypto_kx_agree(&dh_eph, &my_ephemeral->sec, remote_identity);

    /* Root key = KDF(dh_identity || dh_ephemeral) */
    uint8_t root_input[64];
    memcpy(root_input, dh_id.bytes, 32);
    memcpy(root_input + 32, dh_eph.bytes, 32);
    aitp_stub_hash(root_input, 64, ratchet->root_key, 32);

    /* Chain keys from root key */
    ratchet_kdf(ratchet->send_chain, ratchet->root_key, (const uint8_t *)"send", 4);
    ratchet_kdf(ratchet->recv_chain, ratchet->root_key, (const uint8_t *)"recv", 4);

    ratchet->send_count = 0;
    ratchet->recv_count = 0;
    ratchet->skipped_keys = 0;
    ratchet->initialized = true;
    return AI_TP_CRYPTO_OK;
}

int ai_tp_crypto_ratchet_init_responder(ai_tp_crypto_ratchet_t *ratchet,
    const ai_tp_crypto_keypair_t *my_identity,
    const ai_tp_crypto_public_key_t *remote_identity,
    const ai_tp_crypto_public_key_t *remote_ephemeral) {
    if (!ratchet || !my_identity || !remote_identity || !remote_ephemeral) return AI_TP_CRYPTO_ERR_RATCHET;
    memset(ratchet, 0, sizeof(*ratchet));
    ratchet->identity = *my_identity;
    ratchet->remote_identity = *remote_identity;
    ratchet->remote_ephemeral = *remote_ephemeral;

    /* Generate our ephemeral */
    ai_tp_crypto_kx_keypair(&ratchet->ephemeral);

    /* DH computations */
    ai_tp_crypto_session_key_t dh_id, dh_eph;
    ai_tp_crypto_kx_agree(&dh_id, &my_identity->sec, remote_identity);
    ai_tp_crypto_kx_agree(&dh_eph, &ratchet->ephemeral.sec, remote_ephemeral);

    uint8_t root_input[64];
    memcpy(root_input, dh_id.bytes, 32);
    memcpy(root_input + 32, dh_eph.bytes, 32);
    aitp_stub_hash(root_input, 64, ratchet->root_key, 32);

    ratchet_kdf(ratchet->send_chain, ratchet->root_key, (const uint8_t *)"send", 4);
    ratchet_kdf(ratchet->recv_chain, ratchet->root_key, (const uint8_t *)"recv", 4);

    ratchet->send_count = 0;
    ratchet->recv_count = 0;
    ratchet->initialized = true;
    return AI_TP_CRYPTO_OK;
}

int ai_tp_crypto_ratchet_send(ai_tp_crypto_sym_key_t *msg_key, ai_tp_crypto_ratchet_t *ratchet) {
    if (!msg_key || !ratchet || !ratchet->initialized) return AI_TP_CRYPTO_ERR_RATCHET;
    if (ratchet->skipped_keys >= AI_TP_CRYPTO_RATCHET_MAX_SKIP) return AI_TP_CRYPTO_ERR_RATCHET;

    /* msg_key = KDF(send_chain, send_count) */
    uint8_t count_bytes[8];
    for (int i = 0; i < 8; i++) count_bytes[i] = (uint8_t)(ratchet->send_count >> (i * 8));
    ratchet_kdf(msg_key->bytes, ratchet->send_chain, count_bytes, 8);

    /* Advance send chain */
    ratchet_kdf(ratchet->send_chain, ratchet->send_chain, (const uint8_t *)"next", 4);
    ratchet->send_count++;
    return AI_TP_CRYPTO_OK;
}

int ai_tp_crypto_ratchet_recv(ai_tp_crypto_sym_key_t *msg_key, ai_tp_crypto_ratchet_t *ratchet) {
    if (!msg_key || !ratchet || !ratchet->initialized) return AI_TP_CRYPTO_ERR_RATCHET;
    if (ratchet->skipped_keys >= AI_TP_CRYPTO_RATCHET_MAX_SKIP) return AI_TP_CRYPTO_ERR_RATCHET;

    uint8_t count_bytes[8];
    for (int i = 0; i < 8; i++) count_bytes[i] = (uint8_t)(ratchet->recv_count >> (i * 8));
    ratchet_kdf(msg_key->bytes, ratchet->recv_chain, count_bytes, 8);

    ratchet_kdf(ratchet->recv_chain, ratchet->recv_chain, (const uint8_t *)"next", 4);
    ratchet->recv_count++;
    return AI_TP_CRYPTO_OK;
}

void ai_tp_crypto_ratchet_destroy(ai_tp_crypto_ratchet_t *ratchet) {
    if (!ratchet) return;
    ai_tp_crypto_mem_zero(ratchet, sizeof(*ratchet));
}

/* ---- Utility ---- */

bool ai_tp_crypto_mem_eq(const void *a, const void *b, size_t len) {
    if (!a || !b) return false;
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff |= pa[i] ^ pb[i];
    }
    return diff == 0;
}

void ai_tp_crypto_mem_zero(void *buf, size_t len) {
    if (!buf) return;
    volatile uint8_t *p = (volatile uint8_t *)buf;
    while (len--) *p++ = 0;
}

/* Base58 alphabet */
static const char b58_alpha[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

int ai_tp_crypto_pubkey_to_address(char *buf, size_t buf_len, const ai_tp_crypto_public_key_t *pk) {
    if (!buf || !pk || buf_len < 64) return AI_TP_CRYPTO_ERR_BUFFER;
    /* Simplified base58 encoding (stub: hex-like representation) */
    memcpy(buf, "ai-tp:", 6);
    size_t pos = 6;
    for (int i = 0; i < AI_TP_CRYPTO_PUBLIC_KEY_BYTES && pos < buf_len - 2; i++) {
        uint8_t b = pk->pk[i];
        buf[pos++] = b58_alpha[b % 58];
        buf[pos++] = b58_alpha[(b / 58) % 58];
    }
    buf[pos] = '\0';
    return AI_TP_CRYPTO_OK;
}

const char *ai_tp_crypto_get_version(void) {
    return "1.0.0";
}
