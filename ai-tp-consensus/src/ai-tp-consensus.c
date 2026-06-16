/**
 * @file ai-tp-consensus.c
 * @brief AI-TP OS Consensus Mechanism implementation
 * @version 1.0.0
 */

#include "ai-tp-consensus.h"
#include "libaitp-common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ---- Internal helpers ---- */

static int find_validator_index(ai_tp_consensus_context_t *ctx,
                                 const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES]) {
    for (uint32_t i = 0; i < ctx->validator_count; i++) {
        if (memcmp(ctx->validators[i].node_id, node_id, AI_TP_CONSENSUS_NODE_ID_BYTES) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/* ---- Init / Destroy ---- */

int ai_tp_consensus_init(ai_tp_consensus_context_t *ctx, ai_tp_consensus_type_t type) {
    if (!ctx) return AI_TP_CONSENSUS_ERR_INVALID;
    memset(ctx, 0, sizeof(*ctx));
    ctx->type = type;
    ctx->current_epoch = 1;
    ctx->min_stake = 1000;
    ctx->slash_rate_pct = 5;    /* 5% slash per offense */
    ctx->reward_rate_pct = 2;   /* 2% reward per epoch */
    ctx->max_downtime_epochs = 10;
    ctx->initialized = true;
    return AI_TP_CONSENSUS_OK;
}

void ai_tp_consensus_destroy(ai_tp_consensus_context_t *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
}

/* ---- Validator Management ---- */

int ai_tp_consensus_register_validator(ai_tp_consensus_context_t *ctx,
                                        const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES],
                                        uint64_t stake) {
    if (!ctx || !node_id) return AI_TP_CONSENSUS_ERR_INVALID;
    if (!ctx->initialized) return AI_TP_CONSENSUS_ERR_INIT;
    if (stake < ctx->min_stake) return AI_TP_CONSENSUS_ERR_STAKE;
    if (ctx->validator_count >= AI_TP_CONSENSUS_MAX_VALIDATORS) return AI_TP_CONSENSUS_ERR_FULL;
    if (find_validator_index(ctx, node_id) >= 0) return AI_TP_CONSENSUS_ERR_INVALID;

    ai_tp_consensus_validator_t *v = &ctx->validators[ctx->validator_count];
    memset(v, 0, sizeof(*v));
    memcpy(v->node_id, node_id, AI_TP_CONSENSUS_NODE_ID_BYTES);
    v->stake = stake;
    v->status = AI_TP_CONSENSUS_NODE_CANDIDATE;
    v->last_active_epoch = ctx->current_epoch;
    ctx->validator_count++;
    ctx->total_stake += stake;
    return AI_TP_CONSENSUS_OK;
}

int ai_tp_consensus_unregister_validator(ai_tp_consensus_context_t *ctx,
                                          const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES]) {
    if (!ctx || !node_id) return AI_TP_CONSENSUS_ERR_INVALID;
    int idx = find_validator_index(ctx, node_id);
    if (idx < 0) return AI_TP_CONSENSUS_ERR_INVALID;
    ctx->total_stake -= ctx->validators[idx].stake;
    uint32_t last = ctx->validator_count - 1;
    if ((uint32_t)idx != last) {
        ctx->validators[idx] = ctx->validators[last];
    }
    ctx->validator_count--;
    return AI_TP_CONSENSUS_OK;
}

const ai_tp_consensus_validator_t *ai_tp_consensus_get_validator(
    ai_tp_consensus_context_t *ctx,
    const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES]) {
    if (!ctx || !node_id) return NULL;
    int idx = find_validator_index(ctx, node_id);
    if (idx < 0) return NULL;
    return &ctx->validators[idx];
}

int ai_tp_consensus_select_validators(ai_tp_consensus_context_t *ctx,
                                       uint8_t seed[AI_TP_CONSENSUS_SEED_BYTES]) {
    if (!ctx) return AI_TP_CONSENSUS_ERR_INVALID;
    /* Stub: promote all candidates with sufficient stake to validators */
    for (uint32_t i = 0; i < ctx->validator_count; i++) {
        if (ctx->validators[i].status == AI_TP_CONSENSUS_NODE_CANDIDATE &&
            ctx->validators[i].stake >= ctx->min_stake) {
            ctx->validators[i].status = AI_TP_CONSENSUS_NODE_VALIDATOR;
        }
    }
    return AI_TP_CONSENSUS_OK;
}

/* ---- Epoch Management ---- */

int ai_tp_consensus_advance_epoch(ai_tp_consensus_context_t *ctx) {
    if (!ctx) return AI_TP_CONSENSUS_ERR_INVALID;
    ctx->current_epoch++;
    /* Check for downtime violations */
    for (uint32_t i = 0; i < ctx->validator_count; i++) {
        ai_tp_consensus_validator_t *v = &ctx->validators[i];
        if (v->status == AI_TP_CONSENSUS_NODE_VALIDATOR) {
            uint64_t downtime = ctx->current_epoch - v->last_active_epoch;
            if (downtime > ctx->max_downtime_epochs) {
                v->status = AI_TP_CONSENSUS_NODE_SLASHED;
                uint64_t penalty = v->stake * ctx->slash_rate_pct / 100;
                v->penalties += penalty;
                v->stake -= penalty;
                ctx->total_stake -= penalty;
            }
        }
    }
    return AI_TP_CONSENSUS_OK;
}

uint64_t ai_tp_consensus_get_epoch(const ai_tp_consensus_context_t *ctx) {
    if (!ctx) return 0;
    return ctx->current_epoch;
}

/* ---- Proof of Storage ---- */

int ai_tp_consensus_create_storage_challenge(ai_tp_consensus_context_t *ctx,
    ai_tp_consensus_storage_challenge_t *challenge,
    const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES]) {
    if (!ctx || !challenge || !node_id) return AI_TP_CONSENSUS_ERR_INVALID;
    /* Generate random challenge seed */
    for (int i = 0; i < AI_TP_CONSENSUS_SEED_BYTES; i++) {
        challenge->challenge_seed[i] = (uint8_t)(rand() & 0xFF);
    }
    /* Data root = H(node_id || epoch) */
    uint8_t input[AI_TP_CONSENSUS_NODE_ID_BYTES + 8];
    memcpy(input, node_id, AI_TP_CONSENSUS_NODE_ID_BYTES);
    for (int i = 0; i < 8; i++) input[AI_TP_CONSENSUS_NODE_ID_BYTES + i] = (uint8_t)(ctx->current_epoch >> (i * 8));
    aitp_stub_hash(input, sizeof(input), challenge->data_root, AI_TP_CONSENSUS_BLOCK_HASH_BYTES);
    challenge->size = 1024;
    return AI_TP_CONSENSUS_OK;
}

int ai_tp_consensus_submit_storage_proof(ai_tp_consensus_context_t *ctx,
    const ai_tp_consensus_storage_proof_t *proof) {
    if (!ctx || !proof) return AI_TP_CONSENSUS_ERR_INVALID;
    /* Mark validator as active */
    int idx = find_validator_index(ctx, proof->node_id);
    if (idx < 0) return AI_TP_CONSENSUS_ERR_INVALID;
    ctx->validators[idx].last_active_epoch = ctx->current_epoch;
    ctx->validators[idx].blocks_validated++;
    return AI_TP_CONSENSUS_OK;
}

bool ai_tp_consensus_verify_storage_proof(ai_tp_consensus_context_t *ctx,
    const ai_tp_consensus_storage_proof_t *proof,
    const ai_tp_consensus_storage_challenge_t *challenge) {
    if (!ctx || !proof || !challenge) return false;
    /* Stub: verify proof hash matches expected */
    if (memcmp(proof->data_root, challenge->data_root, AI_TP_CONSENSUS_BLOCK_HASH_BYTES) != 0) return false;
    if (proof->proof_len == 0) return false;
    return true;
}

/* ---- Block Validation ---- */

int ai_tp_consensus_propose_block(ai_tp_consensus_context_t *ctx,
    ai_tp_consensus_block_t *block,
    const uint8_t proposer_id[AI_TP_CONSENSUS_NODE_ID_BYTES]) {
    if (!ctx || !block || !proposer_id) return AI_TP_CONSENSUS_ERR_INVALID;
    /* Only validators can propose */
    int idx = find_validator_index(ctx, proposer_id);
    if (idx < 0 || ctx->validators[idx].status != AI_TP_CONSENSUS_NODE_VALIDATOR)
        return AI_TP_CONSENSUS_ERR_INVALID;

    block->epoch = ctx->current_epoch;
    block->timestamp = ctx->current_epoch * AI_TP_CONSENSUS_EPOCH_DURATION_S;
    memcpy(block->proposer, proposer_id, AI_TP_CONSENSUS_NODE_ID_BYTES);
    block->validator_count = 0;
    for (uint32_t i = 0; i < ctx->validator_count; i++) {
        if (ctx->validators[i].status == AI_TP_CONSENSUS_NODE_VALIDATOR)
            block->validator_count++;
    }
    /* Block hash = H(epoch || proposer || validator_count) */
    uint8_t input[8 + AI_TP_CONSENSUS_NODE_ID_BYTES + 4];
    for (int i = 0; i < 8; i++) input[i] = (uint8_t)(block->epoch >> (i * 8));
    memcpy(input + 8, block->proposer, AI_TP_CONSENSUS_NODE_ID_BYTES);
    for (int i = 0; i < 4; i++) input[8 + AI_TP_CONSENSUS_NODE_ID_BYTES + i] = (uint8_t)(block->validator_count >> (i * 8));
    aitp_stub_hash(input, sizeof(input), block->hash, AI_TP_CONSENSUS_BLOCK_HASH_BYTES);
    return AI_TP_CONSENSUS_OK;
}

bool ai_tp_consensus_validate_block(ai_tp_consensus_context_t *ctx,
    const ai_tp_consensus_block_t *block) {
    if (!ctx || !block) return false;
    if (block->epoch != ctx->current_epoch) return false;
    /* Verify proposer is a validator */
    int idx = find_validator_index(ctx, block->proposer);
    if (idx < 0 || ctx->validators[idx].status != AI_TP_CONSENSUS_NODE_VALIDATOR) return false;
    /* Verify block hash */
    uint8_t expected[AI_TP_CONSENSUS_BLOCK_HASH_BYTES];
    uint8_t input[8 + AI_TP_CONSENSUS_NODE_ID_BYTES + 4];
    for (int i = 0; i < 8; i++) input[i] = (uint8_t)(block->epoch >> (i * 8));
    memcpy(input + 8, block->proposer, AI_TP_CONSENSUS_NODE_ID_BYTES);
    for (int i = 0; i < 4; i++) input[8 + AI_TP_CONSENSUS_NODE_ID_BYTES + i] = (uint8_t)(block->validator_count >> (i * 8));
    aitp_stub_hash(input, sizeof(input), expected, AI_TP_CONSENSUS_BLOCK_HASH_BYTES);
    return memcmp(block->hash, expected, AI_TP_CONSENSUS_BLOCK_HASH_BYTES) == 0;
}

/* ---- Slashing and Rewards ---- */

int ai_tp_consensus_slash(ai_tp_consensus_context_t *ctx,
    const ai_tp_consensus_slash_event_t *event) {
    if (!ctx || !event) return AI_TP_CONSENSUS_ERR_SLASH;
    int idx = find_validator_index(ctx, event->node_id);
    if (idx < 0) return AI_TP_CONSENSUS_ERR_INVALID;

    ai_tp_consensus_validator_t *v = &ctx->validators[idx];
    uint64_t slash_amount = v->stake * ctx->slash_rate_pct / 100;
    if (slash_amount > v->stake) slash_amount = v->stake;

    v->stake -= slash_amount;
    v->penalties += slash_amount;
    ctx->total_stake -= slash_amount;

    if (v->stake < ctx->min_stake) {
        v->status = AI_TP_CONSENSUS_NODE_EJECTED;
        v->stake = 0;
    } else {
        v->status = AI_TP_CONSENSUS_NODE_SLASHED;
    }
    return AI_TP_CONSENSUS_OK;
}

int ai_tp_consensus_distribute_rewards(ai_tp_consensus_context_t *ctx) {
    if (!ctx) return AI_TP_CONSENSUS_ERR_INVALID;
    for (uint32_t i = 0; i < ctx->validator_count; i++) {
        ai_tp_consensus_validator_t *v = &ctx->validators[i];
        if (v->status == AI_TP_CONSENSUS_NODE_VALIDATOR) {
            uint64_t reward = v->stake * ctx->reward_rate_pct / 100;
            v->rewards_earned += reward;
            v->stake += reward;
            ctx->total_stake += reward;
            v->epochs_active++;
        }
    }
    return AI_TP_CONSENSUS_OK;
}

uint64_t ai_tp_consensus_get_validator_stake(ai_tp_consensus_context_t *ctx,
    const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES]) {
    if (!ctx || !node_id) return 0;
    int idx = find_validator_index(ctx, node_id);
    if (idx < 0) return 0;
    return ctx->validators[idx].stake;
}

/* ---- Cheat Detection ---- */

ai_tp_consensus_cheat_type_t ai_tp_consensus_detect_cheat(
    ai_tp_consensus_context_t *ctx,
    const uint8_t node_id[AI_TP_CONSENSUS_NODE_ID_BYTES],
    const uint8_t *evidence, size_t evidence_len) {
    if (!ctx || !node_id || !evidence) return AI_TP_CONSENSUS_CHEAT_NONE;
    int idx = find_validator_index(ctx, node_id);
    if (idx < 0) return AI_TP_CONSENSUS_CHEAT_NONE;

    /* Stub: detect based on validator state */
    ai_tp_consensus_validator_t *v = &ctx->validators[idx];
    uint64_t downtime = ctx->current_epoch - v->last_active_epoch;
    if (downtime > ctx->max_downtime_epochs) return AI_TP_CONSENSUS_CHEAT_DOWNTIME;
    return AI_TP_CONSENSUS_CHEAT_NONE;
}

/* ---- Configuration ---- */

void ai_tp_consensus_set_min_stake(ai_tp_consensus_context_t *ctx, uint64_t min_stake) {
    if (!ctx) return;
    ctx->min_stake = min_stake;
}

void ai_tp_consensus_set_slash_rate(ai_tp_consensus_context_t *ctx, uint64_t pct) {
    if (!ctx) return;
    ctx->slash_rate_pct = pct > 100 ? 100 : pct;
}

void ai_tp_consensus_set_reward_rate(ai_tp_consensus_context_t *ctx, uint64_t pct) {
    if (!ctx) return;
    ctx->reward_rate_pct = pct > 100 ? 100 : pct;
}

void ai_tp_consensus_set_max_downtime(ai_tp_consensus_context_t *ctx, uint32_t epochs) {
    if (!ctx) return;
    ctx->max_downtime_epochs = epochs;
}

/* ---- Utility ---- */

const char *ai_tp_consensus_type_name(ai_tp_consensus_type_t type) {
    switch (type) {
    case AI_TP_CONSENSUS_TYPE_POST:   return "PoSt";
    case AI_TP_CONSENSUS_TYPE_POS:    return "PoS";
    case AI_TP_CONSENSUS_TYPE_POW:    return "PoW";
    case AI_TP_CONSENSUS_TYPE_HYBRID: return "Hybrid";
    default:                          return "Unknown";
    }
}

const char *ai_tp_consensus_cheat_name(ai_tp_consensus_cheat_type_t cheat) {
    switch (cheat) {
    case AI_TP_CONSENSUS_CHEAT_NONE:          return "None";
    case AI_TP_CONSENSUS_CHEAT_DOUBLE_SIGN:   return "DoubleSign";
    case AI_TP_CONSENSUS_CHEAT_INVALID_PROOF: return "InvalidProof";
    case AI_TP_CONSENSUS_CHEAT_STORAGE_FRAUD: return "StorageFraud";
    case AI_TP_CONSENSUS_CHEAT_DOWNTIME:      return "Downtime";
    default:                                   return "Unknown";
    }
}

const char *ai_tp_consensus_node_status_name(ai_tp_consensus_node_status_t status) {
    switch (status) {
    case AI_TP_CONSENSUS_NODE_INACTIVE:  return "Inactive";
    case AI_TP_CONSENSUS_NODE_CANDIDATE: return "Candidate";
    case AI_TP_CONSENSUS_NODE_VALIDATOR: return "Validator";
    case AI_TP_CONSENSUS_NODE_SLASHED:   return "Slashed";
    case AI_TP_CONSENSUS_NODE_EJECTED:   return "Ejected";
    default:                             return "Unknown";
    }
}

const char *ai_tp_consensus_get_version(void) {
    return "1.0.0";
}
