/**
 * @file ai-tp-consensus.h
 * @brief AI-TP OS Consensus Mechanism API
 * @version 1.0.0
 * @date 2026-06-16
 *
 * Network Layer Security Stack - Layer 5: Consensus
 *
 * "Nodes cannot claim they did work they actually didn't"
 *
 * 1. Proof of Storage (PoSt): prove you stored data without cheating
 * 2. Proof of Stake (PoS): validator selection by stake weight
 * 3. Slashing / Rewards: penalize cheating, reward honest behavior
 * 4. Cheat Detection: identify and isolate dishonest nodes
 * 5. Epoch-based validation: time-windowed consensus rounds
 *
 * Depends on: ai-tp-crypto, ai-tp-pow
 */

#ifndef AI_TP_CONSENSUS_H
#define AI_TP_CONSENSUS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AI_TP_CONSENSUS_VERSION_MAJOR 1
#define AI_TP_CONSENSUS_VERSION_MINOR 0
#define AI_TP_CONSENSUS_VERSION_PATCH 0

/* Sizes */
#define AI_TP_CONSENSUS_NODE_ID_BYTES     32
#define AI_TP_CONSENSUS_BLOCK_HASH_BYTES  32
#define AI_TP_CONSENSUS_SIGNATURE_BYTES   64
#define AI_TP_CONSENSUS_SEED_BYTES        32
#define AI_TP_CONSENSUS_MAX_VALIDATORS    256
#define AI_TP_CONSENSUS_EPOCH_DURATION_S  3600

/* Error codes */
#define AI_TP_CONSENSUS_OK              0
#define AI_TP_CONSENSUS_ERR_INIT       -1
#define AI_TP_CONSENSUS_ERR_INVALID    -2
#define AI_TP_CONSENSUS_ERR_STAKE      -3
#define AI_TP_CONSENSUS_ERR_SLASH      -4
#define AI_TP_CONSENSUS_ERR_VERIFY     -5
#define AI_TP_CONSENSUS_ERR_EPOCH      -6
#define AI_TP_CONSENSUS_ERR_CHEAT      -7
#define AI_TP_CONSENSUS_ERR_FULL       -8

/* ---- Enums ---- */

typedef enum {
    AI_TP_CONSENSUS_TYPE_POST = 0,    /* Proof of Storage */
    AI_TP_CONSENSUS_TYPE_POS  = 1,    /* Proof of Stake */
    AI_TP_CONSENSUS_TYPE_POW  = 2,    /* Proof of Work (delegated) */
    AI_TP_CONSENSUS_TYPE_HYBRID = 3   /* Hybrid: PoS + PoSt */
} ai_tp_consensus_type_t;

typedef enum {
    AI_TP_CONSENSUS_NODE_INACTIVE = 0,
    AI_TP_CONSENSUS_NODE_CANDIDATE = 1,
    AI_TP_CONSENSUS_NODE_VALIDATOR = 2,
    AI_TP_CONSENSUS_NODE_SLASHED  = 3,
    AI_TP_CONSENSUS_NODE_EJECTED  = 4
} ai_tp_consensus_node_status_t;

typedef enum {
    AI_TP_CONSENSUS_CHEAT_NONE = 0,
    AI_TP_CONSENSUS_CHEAT_DOUBLE_SIGN = 1,
    AI_TP_CONSENSUS_CHEAT_INVALID_PROOF = 2,
    AI_TP_CONSENSUS_CHEAT_STORAGE_FRAUD = 3,
    AI_TP_CONSENSUS_CHEAT_DOWNTIME = 4
} ai_tp_consensus_cheat_type_t;

/* ---- Data Structures ---- */

typedef struct {
    uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES];
    uint64_t stake;
    ai_tp_consensus_node_status_t status;
    uint64_t epochs_active;
    uint64_t blocks_validated;
    uint64_t rewards_earned;
    uint64_t penalties;
    uint64_t last_active_epoch;
} ai_tp_consensus_validator_t;

typedef struct {
    uint8_t data_root[AI_TP_CONSENSUS_BLOCK_HASH_BYTES];
    uint64_t size;
    uint8_t challenge_seed[AI_TP_CONSENSUS_SEED_BYTES];
} ai_tp_consensus_storage_challenge_t;

typedef struct {
    uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES];
    uint8_t data_root[AI_TP_CONSENSUS_BLOCK_HASH_BYTES];
    uint8_t proof[128];
    size_t  proof_len;
    uint64_t epoch;
    uint64_t timestamp;
} ai_tp_consensus_storage_proof_t;

typedef struct {
    uint8_t hash[AI_TP_CONSENSUS_BLOCK_HASH_BYTES];
    uint64_t epoch;
    uint64_t timestamp;
    uint8_t proposer[AI_TP_CONSENSUS_NODE_ID_BYTES];
    uint32_t validator_count;
    uint8_t signature[AI_TP_CONSENSUS_SIGNATURE_BYTES];
} ai_tp_consensus_block_t;

typedef struct {
    uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES];
    ai_tp_consensus_cheat_type_t cheat_type;
    uint64_t evidence_epoch;
    uint64_t slash_amount;
    uint8_t evidence_data[128];
    size_t  evidence_len;
} ai_tp_consensus_slash_event_t;

typedef struct {
    ai_tp_consensus_type_t type;
    ai_tp_consensus_validator_t validators[AI_TP_CONSENSUS_MAX_VALIDATORS];
    uint32_t validator_count;
    uint64_t current_epoch;
    uint64_t epoch_start_time;
    uint64_t total_stake;
    uint64_t min_stake;
    uint64_t slash_rate_pct;         /* Percentage of stake slashed per offense */
    uint64_t reward_rate_pct;        /* Percentage of stake rewarded per epoch */
    uint32_t max_downtime_epochs;    /* Max epochs a validator can be offline */
    bool initialized;
} ai_tp_consensus_context_t;

/* ============================================================
 * Core API
 * ============================================================ */

/* Initialize / Destroy */
int ai_tp_consensus_init(ai_tp_consensus_context_t *ctx, ai_tp_consensus_type_t type);
void ai_tp_consensus_destroy(ai_tp_consensus_context_t *ctx);

/* ---- Validator Management ---- */

int ai_tp_consensus_register_validator(ai_tp_consensus_context_t *ctx,
                                        const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES],
                                        uint64_t stake);

int ai_tp_consensus_unregister_validator(ai_tp_consensus_context_t *ctx,
                                          const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES]);

const ai_tp_consensus_validator_t *ai_tp_consensus_get_validator(
    ai_tp_consensus_context_t *ctx,
    const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES]);

/* Select validators for current epoch (weighted by stake) */
int ai_tp_consensus_select_validators(ai_tp_consensus_context_t *ctx,
                                       uint8_t seed[AI_TP_CONSENSUS_SEED_BYTES]);

/* ---- Epoch Management ---- */

int ai_tp_consensus_advance_epoch(ai_tp_consensus_context_t *ctx);

uint64_t ai_tp_consensus_get_epoch(const ai_tp_consensus_context_t *ctx);

/* ---- Proof of Storage ---- */

int ai_tp_consensus_create_storage_challenge(ai_tp_consensus_context_t *ctx,
    ai_tp_consensus_storage_challenge_t *challenge,
    const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES]);

int ai_tp_consensus_submit_storage_proof(ai_tp_consensus_context_t *ctx,
    const ai_tp_consensus_storage_proof_t *proof);

bool ai_tp_consensus_verify_storage_proof(ai_tp_consensus_context_t *ctx,
    const ai_tp_consensus_storage_proof_t *proof,
    const ai_tp_consensus_storage_challenge_t *challenge);

/* ---- Block Validation ---- */

int ai_tp_consensus_propose_block(ai_tp_consensus_context_t *ctx,
    ai_tp_consensus_block_t *block,
    const uint8_t proposer_id[AI_TP_CONSENSUS_NODE_ID_BYTES]);

bool ai_tp_consensus_validate_block(ai_tp_consensus_context_t *ctx,
    const ai_tp_consensus_block_t *block);

/* ---- Slashing and Rewards ---- */

int ai_tp_consensus_slash(ai_tp_consensus_context_t *ctx,
    const ai_tp_consensus_slash_event_t *event);

int ai_tp_consensus_distribute_rewards(ai_tp_consensus_context_t *ctx);

uint64_t ai_tp_consensus_get_validator_stake(ai_tp_consensus_context_t *ctx,
    const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES]);

/* ---- Cheat Detection ---- */

ai_tp_consensus_cheat_type_t ai_tp_consensus_detect_cheat(
    ai_tp_consensus_context_t *ctx,
    const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES],
    const uint8_t *evidence, size_t evidence_len);

/* ---- Configuration ---- */

void ai_tp_consensus_set_min_stake(ai_tp_consensus_context_t *ctx, uint64_t min_stake);
void ai_tp_consensus_set_slash_rate(ai_tp_consensus_context_t *ctx, uint64_t pct);
void ai_tp_consensus_set_reward_rate(ai_tp_consensus_context_t *ctx, uint64_t pct);
void ai_tp_consensus_set_max_downtime(ai_tp_consensus_context_t *ctx, uint32_t epochs);

/* ---- Utility ---- */

const char *ai_tp_consensus_type_name(ai_tp_consensus_type_t type);
const char *ai_tp_consensus_cheat_name(ai_tp_consensus_cheat_type_t cheat);
const char *ai_tp_consensus_node_status_name(ai_tp_consensus_node_status_t status);
const char *ai_tp_consensus_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_CONSENSUS_H */

