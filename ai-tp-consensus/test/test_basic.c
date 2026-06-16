/**
 * @file test_basic.c
 * @brief ai-tp-consensus basic tests
 */

#include <stdio.h>
#include <string.h>
#include "../include/ai-tp-consensus.h"

static int passed = 0, total = 0;

#define TEST(name) do { total++; printf("  TEST %d: %s ... ", total, name); } while(0)
#define PASS() do { passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

static void test_init_destroy(void) {
    TEST("init and destroy");
    ai_tp_consensus_context_t ctx;
    if (ai_tp_consensus_init(&ctx, AI_TP_CONSENSUS_TYPE_HYBRID) != AI_TP_CONSENSUS_OK) { FAIL("init"); return; }
    if (!ctx.initialized) { FAIL("flag"); return; }
    if (ctx.type != AI_TP_CONSENSUS_TYPE_HYBRID) { FAIL("type"); return; }
    if (ctx.min_stake != 1000) { FAIL("stake"); return; }
    ai_tp_consensus_destroy(&ctx);
    PASS();
}

static void test_register_validator(void) {
    TEST("register and unregister validator");
    ai_tp_consensus_context_t ctx;
    ai_tp_consensus_init(&ctx, AI_TP_CONSENSUS_TYPE_POS);
    uint8_t id[32]; memset(id, 0xAA, 32);
    if (ai_tp_consensus_register_validator(&ctx, id, 5000) != AI_TP_CONSENSUS_OK) { FAIL("reg"); return; }
    if (ctx.validator_count != 1) { FAIL("cnt"); return; }
    if (ctx.total_stake != 5000) { FAIL("stake"); return; }

    const ai_tp_consensus_validator_t *v = ai_tp_consensus_get_validator(&ctx, id);
    if (!v) { FAIL("get"); return; }
    if (v->stake != 5000) { FAIL("vstake"); return; }
    if (v->status != AI_TP_CONSENSUS_NODE_CANDIDATE) { FAIL("status"); return; }

    /* Below min stake */
    uint8_t id2[32]; memset(id2, 0xBB, 32);
    if (ai_tp_consensus_register_validator(&ctx, id2, 100) == AI_TP_CONSENSUS_OK) { FAIL("low stake"); return; }

    /* Unregister */
    if (ai_tp_consensus_unregister_validator(&ctx, id) != AI_TP_CONSENSUS_OK) { FAIL("unreg"); return; }
    if (ctx.validator_count != 0) { FAIL("cnt2"); return; }
    ai_tp_consensus_destroy(&ctx);
    PASS();
}

static void test_select_validators(void) {
    TEST("select validators");
    ai_tp_consensus_context_t ctx;
    ai_tp_consensus_init(&ctx, AI_TP_CONSENSUS_TYPE_POS);
    uint8_t id1[32]; memset(id1, 0x11, 32);
    uint8_t id2[32]; memset(id2, 0x22, 32);
    ai_tp_consensus_register_validator(&ctx, id1, 5000);
    ai_tp_consensus_register_validator(&ctx, id2, 3000);

    uint8_t seed[32]; memset(seed, 0, 32);
    ai_tp_consensus_select_validators(&ctx, seed);

    const ai_tp_consensus_validator_t *v = ai_tp_consensus_get_validator(&ctx, id1);
    if (v->status != AI_TP_CONSENSUS_NODE_VALIDATOR) { FAIL("v1"); return; }
    ai_tp_consensus_destroy(&ctx);
    PASS();
}

static void test_epoch(void) {
    TEST("epoch advancement");
    ai_tp_consensus_context_t ctx;
    ai_tp_consensus_init(&ctx, AI_TP_CONSENSUS_TYPE_POS);
    if (ai_tp_consensus_get_epoch(&ctx) != 1) { FAIL("e1"); return; }
    ai_tp_consensus_advance_epoch(&ctx);
    if (ai_tp_consensus_get_epoch(&ctx) != 2) { FAIL("e2"); return; }
    ai_tp_consensus_destroy(&ctx);
    PASS();
}

static void test_storage_proof(void) {
    TEST("storage challenge and proof");
    ai_tp_consensus_context_t ctx;
    ai_tp_consensus_init(&ctx, AI_TP_CONSENSUS_TYPE_POST);
    uint8_t id[32]; memset(id, 0xAA, 32);
    ai_tp_consensus_register_validator(&ctx, id, 5000);
    uint8_t seed[32]; memset(seed, 0, 32);
    ai_tp_consensus_select_validators(&ctx, seed);

    ai_tp_consensus_storage_challenge_t challenge;
    if (ai_tp_consensus_create_storage_challenge(&ctx, &challenge, id) != AI_TP_CONSENSUS_OK) { FAIL("ch"); return; }

    ai_tp_consensus_storage_proof_t proof;
    memset(&proof, 0, sizeof(proof));
    memcpy(proof.node_id, id, 32);
    memcpy(proof.data_root, challenge.data_root, 32);
    memset(proof.proof, 0x42, 128);
    proof.proof_len = 128;
    proof.epoch = ctx.current_epoch;

    if (ai_tp_consensus_submit_storage_proof(&ctx, &proof) != AI_TP_CONSENSUS_OK) { FAIL("submit"); return; }
    if (!ai_tp_consensus_verify_storage_proof(&ctx, &proof, &challenge)) { FAIL("verify"); return; }
    ai_tp_consensus_destroy(&ctx);
    PASS();
}

static void test_block(void) {
    TEST("block proposal and validation");
    ai_tp_consensus_context_t ctx;
    ai_tp_consensus_init(&ctx, AI_TP_CONSENSUS_TYPE_HYBRID);
    uint8_t id[32]; memset(id, 0xCC, 32);
    ai_tp_consensus_register_validator(&ctx, id, 10000);
    uint8_t seed[32]; memset(seed, 0, 32);
    ai_tp_consensus_select_validators(&ctx, seed);

    ai_tp_consensus_block_t block;
    if (ai_tp_consensus_propose_block(&ctx, &block, id) != AI_TP_CONSENSUS_OK) { FAIL("propose"); return; }
    if (block.epoch != ctx.current_epoch) { FAIL("epoch"); return; }
    if (!ai_tp_consensus_validate_block(&ctx, &block)) { FAIL("validate"); return; }

    /* Non-validator cannot propose */
    uint8_t bad_id[32]; memset(bad_id, 0xDD, 32);
    ai_tp_consensus_block_t bad_block;
    if (ai_tp_consensus_propose_block(&ctx, &bad_block, bad_id) == AI_TP_CONSENSUS_OK) { FAIL("bad propose"); return; }
    ai_tp_consensus_destroy(&ctx);
    PASS();
}

static void test_slash(void) {
    TEST("slashing");
    ai_tp_consensus_context_t ctx;
    ai_tp_consensus_init(&ctx, AI_TP_CONSENSUS_TYPE_POS);
    uint8_t id[32]; memset(id, 0xEE, 32);
    ai_tp_consensus_register_validator(&ctx, id, 10000);
    uint8_t seed[32]; memset(seed, 0, 32);
    ai_tp_consensus_select_validators(&ctx, seed);

    ai_tp_consensus_slash_event_t event;
    memset(&event, 0, sizeof(event));
    memcpy(event.node_id, id, 32);
    event.cheat_type = AI_TP_CONSENSUS_CHEAT_DOUBLE_SIGN;
    event.evidence_epoch = ctx.current_epoch;

    uint64_t stake_before = ai_tp_consensus_get_validator_stake(&ctx, id);
    if (ai_tp_consensus_slash(&ctx, &event) != AI_TP_CONSENSUS_OK) { FAIL("slash"); return; }
    uint64_t stake_after = ai_tp_consensus_get_validator_stake(&ctx, id);
    if (stake_after >= stake_before) { FAIL("not slashed"); return; }

    const ai_tp_consensus_validator_t *v = ai_tp_consensus_get_validator(&ctx, id);
    if (v->status != AI_TP_CONSENSUS_NODE_SLASHED) { FAIL("status"); return; }
    ai_tp_consensus_destroy(&ctx);
    PASS();
}

static void test_rewards(void) {
    TEST("reward distribution");
    ai_tp_consensus_context_t ctx;
    ai_tp_consensus_init(&ctx, AI_TP_CONSENSUS_TYPE_POS);
    uint8_t id[32]; memset(id, 0xFF, 32);
    ai_tp_consensus_register_validator(&ctx, id, 10000);
    uint8_t seed[32]; memset(seed, 0, 32);
    ai_tp_consensus_select_validators(&ctx, seed);

    uint64_t stake_before = ai_tp_consensus_get_validator_stake(&ctx, id);
    if (ai_tp_consensus_distribute_rewards(&ctx) != AI_TP_CONSENSUS_OK) { FAIL("reward"); return; }
    uint64_t stake_after = ai_tp_consensus_get_validator_stake(&ctx, id);
    if (stake_after <= stake_before) { FAIL("no reward"); return; }
    ai_tp_consensus_destroy(&ctx);
    PASS();
}

static void test_cheat_detection(void) {
    TEST("cheat detection (downtime)");
    ai_tp_consensus_context_t ctx;
    ai_tp_consensus_init(&ctx, AI_TP_CONSENSUS_TYPE_POS);
    ai_tp_consensus_set_max_downtime(&ctx, 3);
    uint8_t id[32]; memset(id, 0x11, 32);
    ai_tp_consensus_register_validator(&ctx, id, 10000);
    uint8_t seed[32]; memset(seed, 0, 32);
    ai_tp_consensus_select_validators(&ctx, seed);

    /* Advance epochs without activity */
    for (int i = 0; i < 5; i++) ai_tp_consensus_advance_epoch(&ctx);

    uint8_t evidence[32]; memset(evidence, 0, 32);
    ai_tp_consensus_cheat_type_t cheat = ai_tp_consensus_detect_cheat(&ctx, id, evidence, 32);
    if (cheat != AI_TP_CONSENSUS_CHEAT_DOWNTIME) { FAIL("not detected"); return; }
    ai_tp_consensus_destroy(&ctx);
    PASS();
}

static void test_config(void) {
    TEST("configuration");
    ai_tp_consensus_context_t ctx;
    ai_tp_consensus_init(&ctx, AI_TP_CONSENSUS_TYPE_HYBRID);
    ai_tp_consensus_set_min_stake(&ctx, 500);
    ai_tp_consensus_set_slash_rate(&ctx, 10);
    ai_tp_consensus_set_reward_rate(&ctx, 5);
    ai_tp_consensus_set_max_downtime(&ctx, 5);
    if (ctx.min_stake != 500) { FAIL("min"); return; }
    if (ctx.slash_rate_pct != 10) { FAIL("slash"); return; }
    if (ctx.reward_rate_pct != 5) { FAIL("reward"); return; }
    if (ctx.max_downtime_epochs != 5) { FAIL("down"); return; }
    ai_tp_consensus_destroy(&ctx);
    PASS();
}

static void test_names(void) {
    TEST("type and cheat names");
    if (strcmp(ai_tp_consensus_type_name(AI_TP_CONSENSUS_TYPE_HYBRID), "Hybrid") != 0) { FAIL("hybrid"); return; }
    if (strcmp(ai_tp_consensus_cheat_name(AI_TP_CONSENSUS_CHEAT_DOUBLE_SIGN), "DoubleSign") != 0) { FAIL("ds"); return; }
    if (strcmp(ai_tp_consensus_node_status_name(AI_TP_CONSENSUS_NODE_VALIDATOR), "Validator") != 0) { FAIL("v"); return; }
    if (strcmp(ai_tp_consensus_get_version(), "1.0.0") != 0) { FAIL("ver"); return; }
    PASS();
}

int main(void) {
    printf("=== ai-tp-consensus tests ===\n\n");
    test_init_destroy();
    test_register_validator();
    test_select_validators();
    test_epoch();
    test_storage_proof();
    test_block();
    test_slash();
    test_rewards();
    test_cheat_detection();
    test_config();
    test_names();
    printf("\n=== Results: %d/%d passed ===\n", passed, total);
    return (passed == total) ? 0 : 1;
}

