/**
 * @file ai-tp-pow.c
 * @brief AI-TP OS Proof of Work implementation
 * @version 1.0.0
 *
 * Challenge-Response PoW for anti-Sybil protection.
 * Stub hash functions - production should use libsodium/argon2.
 */

#include "ai-tp-pow.h"
#include "libaitp-common.h"
#include <string.h>
#include <stdio.h>

/* ---- Internal helpers ---- */

static void compute_pow_hash(uint8_t *out, size_t out_len,
                              const uint8_t *challenge, size_t challenge_len,
                              uint64_t nonce,
                              ai_tp_pow_alg_t algorithm,
                              const ai_tp_pow_argon2_params_t *argon2,
                              const ai_tp_pow_scrypt_params_t *scrypt) {
    /* Combine challenge + nonce into input buffer */
    uint8_t input[AI_TP_POW_CHALLENGE_BYTES + 8];
    memcpy(input, challenge, challenge_len > AI_TP_POW_CHALLENGE_BYTES ? AI_TP_POW_CHALLENGE_BYTES : challenge_len);
    for (int i = 0; i < 8; i++) {
        input[challenge_len + i] = (uint8_t)(nonce >> (i * 8));
    }
    size_t input_len = AI_TP_POW_CHALLENGE_BYTES + 8;

    switch (algorithm) {
    case AI_TP_POW_ALG_ARGON2ID: {
        /* Stub: iterated hashing to simulate memory-hardness */
        uint8_t tmp[64];
        memcpy(tmp, input, input_len < 64 ? input_len : 64);
        uint32_t iterations = argon2 ? argon2->time_cost : AI_TP_POW_ARGON2_TIME_COST_DEFAULT;
        for (uint32_t iter = 0; iter < iterations; iter++) {
            aitp_stub_hash(tmp, 64, tmp, 64);
        }
        memcpy(out, tmp, out_len < 32 ? out_len : 32);
        break;
    }
    case AI_TP_POW_ALG_SCRYPT: {
        /* Stub: similar iterated approach */
        uint8_t tmp[64];
        memcpy(tmp, input, input_len < 64 ? input_len : 64);
        uint32_t N = scrypt ? scrypt->N : AI_TP_POW_SCRYPT_N_DEFAULT;
        for (uint32_t iter = 0; iter < (N / 4096); iter++) {
            aitp_stub_hash(tmp, 64, tmp, 64);
        }
        memcpy(out, tmp, out_len < 32 ? out_len : 32);
        break;
    }
    case AI_TP_POW_ALG_SHA256:
    default:
        aitp_stub_hash(input, input_len, out, out_len);
        break;
    }
}

/* ---- Init / Destroy ---- */

int ai_tp_pow_init(ai_tp_pow_context_t *ctx, ai_tp_pow_alg_t algorithm) {
    if (!ctx) return AI_TP_POW_ERR_INVALID;
    memset(ctx, 0, sizeof(*ctx));
    ctx->default_algorithm = algorithm;
    ctx->initialized = true;

    /* Set default memory-hard params */
    ctx->argon2_params.time_cost = AI_TP_POW_ARGON2_TIME_COST_DEFAULT;
    ctx->argon2_params.memory_cost = AI_TP_POW_ARGON2_MEMORY_COST_DEFAULT;
    ctx->argon2_params.parallelism = AI_TP_POW_ARGON2_PARALLELISM_DEFAULT;

    ctx->scrypt_params.N = AI_TP_POW_SCRYPT_N_DEFAULT;
    ctx->scrypt_params.r = AI_TP_POW_SCRYPT_R_DEFAULT;
    ctx->scrypt_params.p = AI_TP_POW_SCRYPT_P_DEFAULT;

    /* Default difficulty state */
    ai_tp_pow_difficulty_init(&ctx->diff_state, AI_TP_POW_DIFFICULTY_DEFAULT, 5000, 10);

    return AI_TP_POW_OK;
}

void ai_tp_pow_destroy(ai_tp_pow_context_t *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
}

/* ---- Challenge Generation ---- */

int ai_tp_pow_create_challenge(ai_tp_pow_context_t *ctx,
                                ai_tp_pow_challenge_t *challenge,
                                uint8_t difficulty) {
    if (!ctx || !challenge) return AI_TP_POW_ERR_INVALID;
    if (!ctx->initialized) return AI_TP_POW_ERR_INIT;
    if (difficulty < AI_TP_POW_DIFFICULTY_MIN || difficulty > AI_TP_POW_DIFFICULTY_MAX)
        return AI_TP_POW_ERR_DIFFICULTY;

    /* Generate random challenge bytes (stub: use simple PRNG) */
    for (int i = 0; i < AI_TP_POW_CHALLENGE_BYTES; i++) {
        challenge->challenge[i] = (uint8_t)(rand() & 0xFF);
    }
    challenge->difficulty = difficulty;
    challenge->algorithm = ctx->default_algorithm;
    challenge->max_iterations = AI_TP_POW_MAX_ITERATIONS;
    challenge->timestamp = 0;
    return AI_TP_POW_OK;
}

int ai_tp_pow_create_challenge_for_identity(ai_tp_pow_context_t *ctx,
                                             ai_tp_pow_challenge_t *challenge) {
    return ai_tp_pow_create_challenge(ctx, challenge, AI_TP_POW_DIFFICULTY_IDENTITY);
}

bool ai_tp_pow_validate_challenge(const ai_tp_pow_challenge_t *challenge) {
    if (!challenge) return false;
    if (challenge->difficulty < AI_TP_POW_DIFFICULTY_MIN ||
        challenge->difficulty > AI_TP_POW_DIFFICULTY_MAX)
        return false;
    return true;
}

/* ---- Solve ---- */

int ai_tp_pow_solve(ai_tp_pow_context_t *ctx,
                     const ai_tp_pow_challenge_t *challenge,
                     ai_tp_pow_proof_t *proof,
                     uint64_t max_iterations) {
    if (!ctx || !challenge || !proof) return AI_TP_POW_ERR_INVALID;
    if (!ctx->initialized) return AI_TP_POW_ERR_INIT;
    if (!ai_tp_pow_validate_challenge(challenge)) return AI_TP_POW_ERR_DIFFICULTY;

    uint64_t limit = max_iterations > 0 ? max_iterations : challenge->max_iterations;
    /* For testing: cap at reasonable number */
    if (limit > 1000000) limit = 1000000;

    memset(proof, 0, sizeof(*proof));
    memcpy(proof->challenge, challenge->challenge, AI_TP_POW_CHALLENGE_BYTES);

    uint8_t hash[AI_TP_POW_PROOF_BYTES];
    for (uint64_t nonce = 0; nonce < limit; nonce++) {
        compute_pow_hash(hash, AI_TP_POW_PROOF_BYTES,
                         challenge->challenge, AI_TP_POW_CHALLENGE_BYTES,
                         nonce, challenge->algorithm,
                         &ctx->argon2_params, &ctx->scrypt_params);

        if (ai_tp_pow_check_difficulty(hash, AI_TP_POW_PROOF_BYTES, challenge->difficulty)) {
            proof->nonce = nonce;
            memcpy(proof->proof_hash, hash, AI_TP_POW_PROOF_BYTES);
            proof->iterations = nonce + 1;
            return AI_TP_POW_OK;
        }
    }

    return AI_TP_POW_ERR_TIMEOUT;
}

/* ---- Verify ---- */

bool ai_tp_pow_verify(const ai_tp_pow_challenge_t *challenge,
                       const ai_tp_pow_proof_t *proof) {
    if (!challenge || !proof) return false;
    if (memcmp(challenge->challenge, proof->challenge, AI_TP_POW_CHALLENGE_BYTES) != 0)
        return false;

    uint8_t hash[AI_TP_POW_PROOF_BYTES];
    /* We need default params for verification - use defaults */
    ai_tp_pow_argon2_params_t argon2 = {
        AI_TP_POW_ARGON2_TIME_COST_DEFAULT,
        AI_TP_POW_ARGON2_MEMORY_COST_DEFAULT,
        AI_TP_POW_ARGON2_PARALLELISM_DEFAULT
    };
    ai_tp_pow_scrypt_params_t scrypt = {
        AI_TP_POW_SCRYPT_N_DEFAULT,
        AI_TP_POW_SCRYPT_R_DEFAULT,
        AI_TP_POW_SCRYPT_P_DEFAULT
    };

    compute_pow_hash(hash, AI_TP_POW_PROOF_BYTES,
                     challenge->challenge, AI_TP_POW_CHALLENGE_BYTES,
                     proof->nonce, challenge->algorithm,
                     &argon2, &scrypt);

    if (!ai_tp_pow_check_difficulty(hash, AI_TP_POW_PROOF_BYTES, challenge->difficulty))
        return false;

    /* Hash must match claimed proof hash */
    return memcmp(hash, proof->proof_hash, AI_TP_POW_PROOF_BYTES) == 0;
}

/* ---- Hash API ---- */

int ai_tp_pow_hash(uint8_t *out, size_t out_len,
                    const uint8_t *challenge, size_t challenge_len,
                    uint64_t nonce,
                    ai_tp_pow_alg_t algorithm,
                    const ai_tp_pow_argon2_params_t *argon2,
                    const ai_tp_pow_scrypt_params_t *scrypt) {
    if (!out || !challenge) return AI_TP_POW_ERR_INVALID;
    compute_pow_hash(out, out_len, challenge, challenge_len, nonce, algorithm, argon2, scrypt);
    return AI_TP_POW_OK;
}

bool ai_tp_pow_check_difficulty(const uint8_t *hash, size_t hash_len, uint8_t difficulty) {
    return ai_tp_pow_count_leading_zeros(hash, hash_len) >= difficulty;
}

uint8_t ai_tp_pow_count_leading_zeros(const uint8_t *hash, size_t len) {
    uint8_t count = 0;
    for (size_t i = 0; i < len; i++) {
        if (hash[i] == 0) {
            count += 8;
        } else {
            /* Count leading zero bits in this byte */
            uint8_t byte = hash[i];
            while ((byte & 0x80) == 0) {
                count++;
                byte <<= 1;
            }
            break;
        }
    }
    return count;
}

/* ---- Dynamic Difficulty ---- */

int ai_tp_pow_difficulty_init(ai_tp_pow_difficulty_state_t *state,
                               uint8_t initial_difficulty,
                               uint64_t target_solve_time_ms,
                               uint16_t adjustment_interval) {
    if (!state) return AI_TP_POW_ERR_INVALID;
    memset(state, 0, sizeof(*state));
    state->current_difficulty = initial_difficulty;
    state->target_solve_time_ms = target_solve_time_ms;
    state->adjustment_interval = adjustment_interval > 0 ? adjustment_interval : 10;
    return AI_TP_POW_OK;
}

int ai_tp_pow_difficulty_update(ai_tp_pow_difficulty_state_t *state,
                                 uint64_t solve_time_ms) {
    if (!state) return AI_TP_POW_ERR_INVALID;
    state->window_count++;
    state->window_total_time_ms += solve_time_ms;

    if (state->window_count >= state->adjustment_interval) {
        uint64_t avg_time = state->window_total_time_ms / state->window_count;
        if (avg_time < state->target_solve_time_ms / 2) {
            /* Solving too fast - increase difficulty */
            if (state->current_difficulty < AI_TP_POW_DIFFICULTY_MAX)
                state->current_difficulty++;
        } else if (avg_time > state->target_solve_time_ms * 2) {
            /* Solving too slow - decrease difficulty */
            if (state->current_difficulty > AI_TP_POW_DIFFICULTY_MIN)
                state->current_difficulty--;
        }
        /* Reset window */
        state->window_count = 0;
        state->window_total_time_ms = 0;
    }
    return AI_TP_POW_OK;
}

uint8_t ai_tp_pow_difficulty_get(const ai_tp_pow_difficulty_state_t *state) {
    if (!state) return AI_TP_POW_DIFFICULTY_DEFAULT;
    return state->current_difficulty;
}

/* ---- Parameters ---- */

const ai_tp_pow_argon2_params_t *ai_tp_pow_get_argon2_params(const ai_tp_pow_context_t *ctx) {
    if (!ctx) return NULL;
    return &ctx->argon2_params;
}

const ai_tp_pow_scrypt_params_t *ai_tp_pow_get_scrypt_params(const ai_tp_pow_context_t *ctx) {
    if (!ctx) return NULL;
    return &ctx->scrypt_params;
}

void ai_tp_pow_set_argon2_params(ai_tp_pow_context_t *ctx,
                                  uint32_t time_cost, uint32_t memory_cost, uint32_t parallelism) {
    if (!ctx) return;
    ctx->argon2_params.time_cost = time_cost;
    ctx->argon2_params.memory_cost = memory_cost;
    ctx->argon2_params.parallelism = parallelism;
}

void ai_tp_pow_set_scrypt_params(ai_tp_pow_context_t *ctx,
                                  uint32_t N, uint32_t r, uint32_t p) {
    if (!ctx) return;
    ctx->scrypt_params.N = N;
    ctx->scrypt_params.r = r;
    ctx->scrypt_params.p = p;
}

/* ---- Utility ---- */

const char *ai_tp_pow_alg_name(ai_tp_pow_alg_t alg) {
    switch (alg) {
    case AI_TP_POW_ALG_SHA256:    return "SHA-256";
    case AI_TP_POW_ALG_ARGON2ID:  return "Argon2id";
    case AI_TP_POW_ALG_SCRYPT:    return "Scrypt";
    default:                      return "Unknown";
    }
}

const char *ai_tp_pow_get_version(void) {
    return "1.0.0";
}
