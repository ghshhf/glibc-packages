/**
 * @file ai-tp-pow.h
 * @brief AI-TP OS Proof of Work API
 * @version 1.0.0
 * @date 2026-06-16
 *
 * Network Layer Security Stack - Layer 3: Proof of Work
 *
 * Anti-Sybil: creating new identities requires computational work.
 * Challenge-Response model (not continuous mining like Bitcoin).
 * Dynamic difficulty adjustment based on network conditions.
 * Memory-hard hashing (Argon2/Scrypt interfaces) to resist ASIC/GPU.
 *
 * Depends on: ai-tp-crypto (for SHA-256, HMAC, random bytes)
 */

#ifndef AI_TP_POW_H
#define AI_TP_POW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AI_TP_POW_VERSION_MAJOR 1
#define AI_TP_POW_VERSION_MINOR 0
#define AI_TP_POW_VERSION_PATCH 0

/* Sizes */
#define AI_TP_POW_CHALLENGE_BYTES    32
#define AI_TP_POW_NONCE_BYTES        16
#define AI_TP_POW_PROOF_BYTES        32
#define AI_TP_POW_SEED_BYTES         32
#define AI_TP_POW_MAX_ITERATIONS     (1ULL << 32)

/* Difficulty: number of leading zero bits in hash */
#define AI_TP_POW_DIFFICULTY_MIN     1
#define AI_TP_POW_DIFFICULTY_MAX     32
#define AI_TP_POW_DIFFICULTY_DEFAULT 16
#define AI_TP_POW_DIFFICULTY_IDENTITY 20  /* For creating new node identities */

/* Memory-hard hash parameters */
#define AI_TP_POW_ARGON2_TIME_COST_DEFAULT    3
#define AI_TP_POW_ARGON2_MEMORY_COST_DEFAULT  65536  /* 64 MB */
#define AI_TP_POW_ARGON2_PARALLELISM_DEFAULT  2
#define AI_TP_POW_SCRYPT_N_DEFAULT            16384
#define AI_TP_POW_SCRYPT_R_DEFAULT            8
#define AI_TP_POW_SCRYPT_P_DEFAULT            1

/* Error codes */
#define AI_TP_POW_OK              0
#define AI_TP_POW_ERR_INIT       -1
#define AI_TP_POW_ERR_INVALID    -2
#define AI_TP_POW_ERR_DIFFICULTY -3
#define AI_TP_POW_ERR_TIMEOUT    -4
#define AI_TP_POW_ERR_OVERFLOW   -5
#define AI_TP_POW_ERR_VERIFY     -6
#define AI_TP_POW_ERR_NOMEM      -7

/* Algorithm types */
typedef enum {
    AI_TP_POW_ALG_SHA256 = 0,      /* Simple SHA-256 (for lightweight nodes) */
    AI_TP_POW_ALG_ARGON2ID = 1,    /* Memory-hard (recommended) */
    AI_TP_POW_ALG_SCRYPT = 2       /* Memory-hard (alternative) */
} ai_tp_pow_alg_t;

/* Challenge issued by verifier */
typedef struct {
    uint8_t challenge[AI_TP_POW_CHALLENGE_BYTES];
    uint8_t difficulty;             /* Number of leading zero bits required */
    ai_tp_pow_alg_t algorithm;
    uint64_t max_iterations;        /* Max nonce attempts allowed */
    uint64_t timestamp;             /* When challenge was issued */
} ai_tp_pow_challenge_t;

/* Proof of work solution */
typedef struct {
    uint8_t challenge[AI_TP_POW_CHALLENGE_BYTES];
    uint64_t nonce;                 /* The nonce that solved the challenge */
    uint8_t proof_hash[AI_TP_POW_PROOF_BYTES];  /* hash(challenge || nonce) */
    uint64_t iterations;            /* How many attempts it took */
    uint64_t compute_time_ms;       /* Wall clock time to solve */
} ai_tp_pow_proof_t;

/* Argon2 parameters */
typedef struct {
    uint32_t time_cost;
    uint32_t memory_cost;   /* KiB */
    uint32_t parallelism;
} ai_tp_pow_argon2_params_t;

/* Scrypt parameters */
typedef struct {
    uint32_t N;     /* CPU/Memory cost factor */
    uint32_t r;     /* Block size */
    uint32_t p;     /* Parallelism */
} ai_tp_pow_scrypt_params_t;

/* Dynamic difficulty state */
typedef struct {
    uint8_t current_difficulty;
    uint64_t target_solve_time_ms;  /* Target: how long solving should take */
    uint64_t window_count;          /* Number of solves in adjustment window */
    uint64_t window_total_time_ms;  /* Total solve time in window */
    uint64_t last_adjust_time;      /* When difficulty was last adjusted */
    uint16_t adjustment_interval;   /* Solve count between adjustments */
} ai_tp_pow_difficulty_state_t;

/* PoW context */
typedef struct {
    ai_tp_pow_alg_t default_algorithm;
    ai_tp_pow_argon2_params_t argon2_params;
    ai_tp_pow_scrypt_params_t scrypt_params;
    ai_tp_pow_difficulty_state_t diff_state;
    bool initialized;
} ai_tp_pow_context_t;

/* ============================================================
 * Core API
 * ============================================================ */

/* Initialize / Destroy */
int ai_tp_pow_init(ai_tp_pow_context_t *ctx, ai_tp_pow_alg_t algorithm);
void ai_tp_pow_destroy(ai_tp_pow_context_t *ctx);

/* Challenge Generation */
int ai_tp_pow_create_challenge(ai_tp_pow_context_t *ctx,
                                ai_tp_pow_challenge_t *challenge,
                                uint8_t difficulty);

int ai_tp_pow_create_challenge_for_identity(ai_tp_pow_context_t *ctx,
                                             ai_tp_pow_challenge_t *challenge);

/* Challenge validation (not expired, valid difficulty range) */
bool ai_tp_pow_validate_challenge(const ai_tp_pow_challenge_t *challenge);

/* Solve (Prover side) */
int ai_tp_pow_solve(ai_tp_pow_context_t *ctx,
                     const ai_tp_pow_challenge_t *challenge,
                     ai_tp_pow_proof_t *proof,
                     uint64_t max_iterations);

/* Verify (Verifier side) */
bool ai_tp_pow_verify(const ai_tp_pow_challenge_t *challenge,
                       const ai_tp_pow_proof_t *proof);

/* Hash computation (exposed for testing) */
int ai_tp_pow_hash(uint8_t *out, size_t out_len,
                    const uint8_t *challenge, size_t challenge_len,
                    uint64_t nonce,
                    ai_tp_pow_alg_t algorithm,
                    const ai_tp_pow_argon2_params_t *argon2,
                    const ai_tp_pow_scrypt_params_t *scrypt);

/* Check if hash meets difficulty requirement */
bool ai_tp_pow_check_difficulty(const uint8_t *hash, size_t hash_len, uint8_t difficulty);

/* Count leading zero bits in hash */
uint8_t ai_tp_pow_count_leading_zeros(const uint8_t *hash, size_t len);

/* Dynamic Difficulty Adjustment */
int ai_tp_pow_difficulty_init(ai_tp_pow_difficulty_state_t *state,
                               uint8_t initial_difficulty,
                               uint64_t target_solve_time_ms,
                               uint16_t adjustment_interval);

int ai_tp_pow_difficulty_update(ai_tp_pow_difficulty_state_t *state,
                                 uint64_t solve_time_ms);

uint8_t ai_tp_pow_difficulty_get(const ai_tp_pow_difficulty_state_t *state);

/* Parameter Getters */
const ai_tp_pow_argon2_params_t *ai_tp_pow_get_argon2_params(const ai_tp_pow_context_t *ctx);
const ai_tp_pow_scrypt_params_t *ai_tp_pow_get_scrypt_params(const ai_tp_pow_context_t *ctx);

/* Set custom memory-hard parameters */
void ai_tp_pow_set_argon2_params(ai_tp_pow_context_t *ctx,
                                  uint32_t time_cost, uint32_t memory_cost, uint32_t parallelism);
void ai_tp_pow_set_scrypt_params(ai_tp_pow_context_t *ctx,
                                  uint32_t N, uint32_t r, uint32_t p);

/* Utility */
const char *ai_tp_pow_alg_name(ai_tp_pow_alg_t alg);
const char *ai_tp_pow_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_POW_H */

