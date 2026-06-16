/**
 * @file test_basic.c
 * @brief ai-tp-pow basic tests
 */

#include <stdio.h>
#include <string.h>
#include "../include/ai-tp-pow.h"

static int passed = 0, total = 0;

#define TEST(name) do { total++; printf("  TEST %d: %s ... ", total, name); } while(0)
#define PASS() do { passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

static void test_init_destroy(void) {
    TEST("init and destroy");
    ai_tp_pow_context_t ctx;
    if (ai_tp_pow_init(&ctx, AI_TP_POW_ALG_SHA256) != AI_TP_POW_OK) { FAIL("init"); return; }
    if (!ctx.initialized) { FAIL("flag"); return; }
    if (ctx.default_algorithm != AI_TP_POW_ALG_SHA256) { FAIL("alg"); return; }
    ai_tp_pow_destroy(&ctx);
    if (ctx.initialized) { FAIL("destroy"); return; }
    PASS();
}

static void test_challenge_create(void) {
    TEST("challenge creation");
    ai_tp_pow_context_t ctx;
    ai_tp_pow_init(&ctx, AI_TP_POW_ALG_SHA256);
    ai_tp_pow_challenge_t ch;
    if (ai_tp_pow_create_challenge(&ctx, &ch, 8) != AI_TP_POW_OK) { FAIL("create"); return; }
    if (ch.difficulty != 8) { FAIL("diff"); return; }
    if (ch.algorithm != AI_TP_POW_ALG_SHA256) { FAIL("alg"); return; }
    if (!ai_tp_pow_validate_challenge(&ch)) { FAIL("valid"); return; }
    ai_tp_pow_destroy(&ctx);
    PASS();
}

static void test_challenge_validation(void) {
    TEST("challenge validation");
    ai_tp_pow_challenge_t ch;
    memset(&ch, 0, sizeof(ch));
    ch.difficulty = 0;
    if (ai_tp_pow_validate_challenge(&ch)) { FAIL("0 diff"); return; }
    ch.difficulty = 33;
    if (ai_tp_pow_validate_challenge(&ch)) { FAIL("33 diff"); return; }
    ch.difficulty = 16;
    if (!ai_tp_pow_validate_challenge(&ch)) { FAIL("valid 16"); return; }
    PASS();
}

static void test_solve_verify_sha256(void) {
    TEST("solve and verify (SHA-256, low difficulty)");
    ai_tp_pow_context_t ctx;
    ai_tp_pow_init(&ctx, AI_TP_POW_ALG_SHA256);
    ai_tp_pow_challenge_t ch;
    /* Use very low difficulty for fast test */
    if (ai_tp_pow_create_challenge(&ctx, &ch, 4) != AI_TP_POW_OK) { FAIL("create"); return; }

    ai_tp_pow_proof_t proof;
    if (ai_tp_pow_solve(&ctx, &ch, &proof, 100000) != AI_TP_POW_OK) { FAIL("solve"); return; }
    if (proof.nonce == 0 && proof.iterations == 0) { FAIL("no solve"); return; }

    if (!ai_tp_pow_verify(&ch, &proof)) { FAIL("verify"); return; }
    ai_tp_pow_destroy(&ctx);
    PASS();
}

static void test_solve_verify_argon2(void) {
    TEST("solve and verify (Argon2id, low difficulty)");
    ai_tp_pow_context_t ctx;
    ai_tp_pow_init(&ctx, AI_TP_POW_ALG_ARGON2ID);
    /* Use minimal argon2 params for test speed */
    ai_tp_pow_set_argon2_params(&ctx, 1, 1024, 1);
    ai_tp_pow_challenge_t ch;
    if (ai_tp_pow_create_challenge(&ctx, &ch, 4) != AI_TP_POW_OK) { FAIL("create"); return; }

    ai_tp_pow_proof_t proof;
    if (ai_tp_pow_solve(&ctx, &ch, &proof, 100000) != AI_TP_POW_OK) { FAIL("solve"); return; }
    if (!ai_tp_pow_verify(&ch, &proof)) { FAIL("verify"); return; }
    ai_tp_pow_destroy(&ctx);
    PASS();
}

static void test_wrong_proof_rejected(void) {
    TEST("wrong proof rejected");
    ai_tp_pow_context_t ctx;
    ai_tp_pow_init(&ctx, AI_TP_POW_ALG_SHA256);
    ai_tp_pow_challenge_t ch;
    ai_tp_pow_create_challenge(&ctx, &ch, 4);
    ai_tp_pow_proof_t proof;
    ai_tp_pow_solve(&ctx, &ch, &proof, 100000);

    /* Tamper with proof */
    ai_tp_pow_proof_t bad = proof;
    bad.nonce += 1;
    if (ai_tp_pow_verify(&ch, &bad)) { FAIL("tamper nonce"); return; }

    /* Wrong challenge */
    ai_tp_pow_challenge_t ch2;
    ai_tp_pow_create_challenge(&ctx, &ch2, 4);
    if (ai_tp_pow_verify(&ch2, &proof)) { FAIL("wrong challenge"); return; }
    ai_tp_pow_destroy(&ctx);
    PASS();
}

static void test_leading_zeros(void) {
    TEST("leading zero bit counting");
    uint8_t h1[32]; memset(h1, 0, 32);
    if (ai_tp_pow_count_leading_zeros(h1, 32) != 32 * 8) { FAIL("all zero"); return; }
    uint8_t h2[32]; memset(h2, 0xFF, 32);
    if (ai_tp_pow_count_leading_zeros(h2, 32) != 0) { FAIL("all ones"); return; }
    uint8_t h3[32]; memset(h3, 0, 32); h3[3] = 0x0F;
    if (ai_tp_pow_count_leading_zeros(h3, 32) != 28) { FAIL("28 zeros"); return; }
    PASS();
}

static void test_difficulty_adjustment(void) {
    TEST("dynamic difficulty adjustment");
    ai_tp_pow_difficulty_state_t state;
    ai_tp_pow_difficulty_init(&state, 16, 5000, 5);

    /* Fast solves -> difficulty should increase */
    for (int i = 0; i < 5; i++) ai_tp_pow_difficulty_update(&state, 1000);
    if (state.current_difficulty <= 16) { FAIL("should increase"); return; }

    /* Slow solves -> difficulty should decrease */
    ai_tp_pow_difficulty_init(&state, 16, 5000, 5);
    for (int i = 0; i < 5; i++) ai_tp_pow_difficulty_update(&state, 20000);
    if (state.current_difficulty >= 16) { FAIL("should decrease"); return; }
    PASS();
}

static void test_identity_challenge(void) {
    TEST("identity challenge difficulty");
    ai_tp_pow_context_t ctx;
    ai_tp_pow_init(&ctx, AI_TP_POW_ALG_SHA256);
    ai_tp_pow_challenge_t ch;
    if (ai_tp_pow_create_challenge_for_identity(&ctx, &ch) != AI_TP_POW_OK) { FAIL("create"); return; }
    if (ch.difficulty != AI_TP_POW_DIFFICULTY_IDENTITY) { FAIL("diff"); return; }
    ai_tp_pow_destroy(&ctx);
    PASS();
}

static void test_params(void) {
    TEST("argon2 and scrypt params");
    ai_tp_pow_context_t ctx;
    ai_tp_pow_init(&ctx, AI_TP_POW_ALG_ARGON2ID);

    ai_tp_pow_set_argon2_params(&ctx, 5, 131072, 4);
    const ai_tp_pow_argon2_params_t *a = ai_tp_pow_get_argon2_params(&ctx);
    if (!a || a->time_cost != 5 || a->memory_cost != 131072 || a->parallelism != 4) { FAIL("argon2"); return; }

    ai_tp_pow_set_scrypt_params(&ctx, 32768, 16, 2);
    const ai_tp_pow_scrypt_params_t *s = ai_tp_pow_get_scrypt_params(&ctx);
    if (!s || s->N != 32768 || s->r != 16 || s->p != 2) { FAIL("scrypt"); return; }
    ai_tp_pow_destroy(&ctx);
    PASS();
}

static void test_alg_name(void) {
    TEST("algorithm names");
    if (strcmp(ai_tp_pow_alg_name(AI_TP_POW_ALG_SHA256), "SHA-256") != 0) { FAIL("sha"); return; }
    if (strcmp(ai_tp_pow_alg_name(AI_TP_POW_ALG_ARGON2ID), "Argon2id") != 0) { FAIL("argon"); return; }
    if (strcmp(ai_tp_pow_alg_name(AI_TP_POW_ALG_SCRYPT), "Scrypt") != 0) { FAIL("scrypt"); return; }
    PASS();
}

static void test_version(void) {
    TEST("version");
    if (strcmp(ai_tp_pow_get_version(), "1.0.0") != 0) { FAIL("ver"); return; }
    PASS();
}

int main(void) {
    printf("=== ai-tp-pow tests ===\n\n");
    test_init_destroy();
    test_challenge_create();
    test_challenge_validation();
    test_solve_verify_sha256();
    test_solve_verify_argon2();
    test_wrong_proof_rejected();
    test_leading_zeros();
    test_difficulty_adjustment();
    test_identity_challenge();
    test_params();
    test_alg_name();
    test_version();
    printf("\n=== Results: %d/%d passed ===\n", passed, total);
    return (passed == total) ? 0 : 1;
}

