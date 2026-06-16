/**
 * @file ai-tp-zkp.h
 * @brief AI-TP OS Zero-Knowledge Proof API
 * @version 1.0.0
 * @date 2026-06-16
 *
 * Network Layer Security Stack - Layer 4: Zero-Knowledge Proofs
 *
 * "I can prove I have access, but you don't know who I am"
 *
 * 1. Pedersen Commitment: commit to a value without revealing it
 * 2. Sigma Protocol: prove knowledge of a secret without exposing it
 * 3. Range Proof: prove a value is within a range without revealing it
 * 4. Membership Proof: prove membership in a set without revealing which element
 * 5. Simplified zk-SNARK interface: placeholder for production SNARK libs
 *
 * Depends on: ai-tp-crypto (for hash, random, key operations)
 */

#ifndef AI_TP_ZKP_H
#define AI_TP_ZKP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AI_TP_ZKP_VERSION_MAJOR 1
#define AI_TP_ZKP_VERSION_MINOR 0
#define AI_TP_ZKP_VERSION_PATCH 0

/* Sizes */
#define AI_TP_ZKP_FIELD_BYTES        32
#define AI_TP_ZKP_SCALAR_BYTES       32
#define AI_TP_ZKP_COMMITMENT_BYTES   32
#define AI_TP_ZKP_CHALLENGE_BYTES    32
#define AI_TP_ZKP_RESPONSE_BYTES     32
#define AI_TP_ZKP_PROOF_MAX_BYTES    256
#define AI_TP_ZKP_SNARK_PROOF_BYTES  192

/* Error codes */
#define AI_TP_ZKP_OK              0
#define AI_TP_ZKP_ERR_INIT       -1
#define AI_TP_ZKP_ERR_INVALID    -2
#define AI_TP_ZKP_ERR_VERIFY     -3
#define AI_TP_ZKP_ERR_COMMIT     -4
#define AI_TP_ZKP_ERR_PROOF      -5
#define AI_TP_ZKP_ERR_BUFFER     -6
#define AI_TP_ZKP_ERR_RANGE      -7

/* ---- Pedersen Commitment ---- */

typedef struct {
    uint8_t value[AI_TP_ZKP_COMMITMENT_BYTES];
} ai_tp_zkp_commitment_t;

typedef struct {
    uint8_t blinding[AI_TP_ZKP_SCALAR_BYTES];
} ai_tp_zkp_blinding_t;

/* ---- Sigma Protocol (Schnorr-like) ---- */

typedef struct {
    uint8_t commitment[AI_TP_ZKP_COMMITMENT_BYTES];   /* R = g^r */
    uint8_t challenge[AI_TP_ZKP_CHALLENGE_BYTES];     /* c = H(g, y, R) */
    uint8_t response[AI_TP_ZKP_RESPONSE_BYTES];        /* s = r + c*x */
} ai_tp_zkp_sigma_proof_t;

typedef struct {
    uint8_t public_key[AI_TP_ZKP_FIELD_BYTES];     /* y = g^x (the statement) */
} ai_tp_zkp_sigma_statement_t;

/* ---- Range Proof ---- */

typedef struct {
    uint8_t commitment[AI_TP_ZKP_COMMITMENT_BYTES];
    uint8_t proof_data[AI_TP_ZKP_PROOF_MAX_BYTES];
    size_t  proof_len;
    uint64_t min_value;
    uint64_t max_value;
} ai_tp_zkp_range_proof_t;

/* ---- Membership Proof ---- */

typedef struct {
    uint8_t root_hash[AI_TP_ZKP_COMMITMENT_BYTES];   /* Merkle root */
    uint8_t proof_data[AI_TP_ZKP_PROOF_MAX_BYTES];
    size_t  proof_len;
    uint32_t set_size;
} ai_tp_zkp_membership_proof_t;

/* ---- zk-SNARK (simplified interface) ---- */

typedef struct {
    uint8_t proof[AI_TP_ZKP_SNARK_PROOF_BYTES];
    uint8_t public_inputs[64];   /* Hash of public inputs */
} ai_tp_zkp_snark_proof_t;

typedef struct {
    uint8_t verifying_key[128];  /* Circuit-specific verification key */
    uint8_t proving_key[128];    /* Circuit-specific proving key */
} ai_tp_zkp_snark_keys_t;

/* ---- ZKP Context ---- */

typedef struct {
    uint8_t generator_g[AI_TP_ZKP_FIELD_BYTES];  /* Base generator */
    uint8_t generator_h[AI_TP_ZKP_FIELD_BYTES];  /* Second generator (for Pedersen) */
    bool initialized;
} ai_tp_zkp_context_t;

/* ============================================================
 * Core API
 * ============================================================ */

/* Initialize / Destroy */
int ai_tp_zkp_init(ai_tp_zkp_context_t *ctx);
void ai_tp_zkp_destroy(ai_tp_zkp_context_t *ctx);

/* ---- Pedersen Commitment ---- */

/**
 * Create a Pedersen commitment: C = g^value * h^blinding
 * If blinding is NULL, a random one is generated.
 */
int ai_tp_zkp_commit(ai_tp_zkp_context_t *ctx,
                      ai_tp_zkp_commitment_t *commitment,
                      uint64_t value,
                      ai_tp_zkp_blinding_t *blinding);

/**
 * Verify a Pedersen commitment opens to a given value.
 * (Requires knowing the blinding factor - used internally.)
 */
bool ai_tp_zkp_commit_verify(const ai_tp_zkp_commitment_t *commitment,
                              uint64_t value,
                              const ai_tp_zkp_blinding_t *blinding,
                              const uint8_t generator_g[AI_TP_ZKP_FIELD_BYTES],
                              const uint8_t generator_h[AI_TP_ZKP_FIELD_BYTES]);

/* ---- Sigma Protocol (Schnorr) ---- */

/**
 * Prove knowledge of secret x such that y = g^x.
 * The prover knows x; the verifier only sees y.
 */
int ai_tp_zkp_sigma_prove(ai_tp_zkp_context_t *ctx,
                            ai_tp_zkp_sigma_proof_t *proof,
                            const uint8_t secret[AI_TP_ZKP_SCALAR_BYTES],
                            const ai_tp_zkp_sigma_statement_t *statement);

/**
 * Verify a Sigma protocol proof.
 * Returns true if the prover knows the secret without revealing it.
 */
bool ai_tp_zkp_sigma_verify(ai_tp_zkp_context_t *ctx,
                              const ai_tp_zkp_sigma_proof_t *proof,
                              const ai_tp_zkp_sigma_statement_t *statement);

/* ---- Range Proof ---- */

/**
 * Prove that a committed value v satisfies min_value <= v <= max_value
 * without revealing v.
 */
int ai_tp_zkp_range_prove(ai_tp_zkp_context_t *ctx,
                            ai_tp_zkp_range_proof_t *proof,
                            uint64_t value,
                            const ai_tp_zkp_blinding_t *blinding,
                            uint64_t min_value,
                            uint64_t max_value);

/**
 * Verify a range proof.
 */
bool ai_tp_zkp_range_verify(ai_tp_zkp_context_t *ctx,
                              const ai_tp_zkp_range_proof_t *proof);

/* ---- Membership Proof ---- */

/**
 * Prove that a value is a member of a set (identified by Merkle root)
 * without revealing which element.
 */
int ai_tp_zkp_membership_prove(ai_tp_zkp_context_t *ctx,
                                ai_tp_zkp_membership_proof_t *proof,
                                const uint8_t *element, size_t element_len,
                                const uint8_t *merkle_proof, size_t merkle_proof_len,
                                const uint8_t root_hash[AI_TP_ZKP_COMMITMENT_BYTES],
                                uint32_t set_size);

/**
 * Verify a membership proof.
 */
bool ai_tp_zkp_membership_verify(ai_tp_zkp_context_t *ctx,
                                  const ai_tp_zkp_membership_proof_t *proof);

/* ---- zk-SNARK (simplified interface) ---- */

/**
 * Generate proving/verifying keys for a circuit.
 * Stub: in production, this calls a SNARK library setup.
 */
int ai_tp_zkp_snark_setup(ai_tp_zkp_snark_keys_t *keys,
                            const uint8_t *circuit_desc, size_t desc_len);

/**
 * Create a zk-SNARK proof.
 * private_inputs are NOT revealed to the verifier.
 * public_inputs_hash is the hash of public inputs (revealed).
 */
int ai_tp_zkp_snark_prove(ai_tp_zkp_snark_keys_t *keys,
                            ai_tp_zkp_snark_proof_t *proof,
                            const uint8_t *private_inputs, size_t private_len,
                            const uint8_t *public_inputs, size_t public_len);

/**
 * Verify a zk-SNARK proof.
 * Only needs the verifying key, proof, and public inputs hash.
 */
bool ai_tp_zkp_snark_verify(const ai_tp_zkp_snark_keys_t *keys,
                              const ai_tp_zkp_snark_proof_t *proof);

/* ---- Utility ---- */

const char *ai_tp_zkp_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_ZKP_H */

