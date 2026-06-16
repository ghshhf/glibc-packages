/**
 * test_basic.c - Integration tests for AI-TP full stack
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ai-tp-integration.h"

static int passed = 0, failed = 0;
#define TEST(n) printf("  Testing: %s ... ", n);
#define PASS() do { printf("PASS\n"); passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); failed++; } while(0)
#define ASSERT(c,m) do { if(!(c)){FAIL(m);return;} } while(0)

static void test_init_destroy(void) {
    TEST("init and destroy");
    aitp_int_ctx_t ctx;
    int r = aitp_int_init(&ctx);
    ASSERT(r == 0, "init failed");
    ASSERT(ctx.initialized == 1, "not initialized");
    ASSERT(ctx.config.verbose == 1, "verbose should be on");
    aitp_int_destroy(&ctx);
    ASSERT(ctx.initialized == 0, "not destroyed");
    PASS();
}

static void test_full_stack(void) {
    TEST("full stack scenario");
    aitp_int_ctx_t ctx;
    aitp_int_init(&ctx);
    ctx.config.verbose = 0; /* quiet for test */
    int r = aitp_int_test_full_stack(&ctx);
    ASSERT(r == 0, "full stack failed");
    ASSERT(ctx.result.scenario_count == 1, "should have 1 scenario");
    ASSERT(ctx.result.scenarios[0].passed_count == 7, "should pass 7 steps");
    ASSERT(ctx.result.scenarios[0].failed_count == 0, "should have 0 failures");
    aitp_int_destroy(&ctx);
    PASS();
}

static void test_network_only(void) {
    TEST("network only scenario");
    aitp_int_ctx_t ctx;
    aitp_int_init(&ctx);
    ctx.config.verbose = 0;
    int r = aitp_int_test_network_only(&ctx);
    ASSERT(r == 0, "network test failed");
    ASSERT(ctx.result.scenarios[0].passed_count == 3, "should pass 3 steps");
    aitp_int_destroy(&ctx);
    PASS();
}

static void test_security_only(void) {
    TEST("security stack scenario");
    aitp_int_ctx_t ctx;
    aitp_int_init(&ctx);
    ctx.config.verbose = 0;
    int r = aitp_int_test_security_only(&ctx);
    ASSERT(r == 0, "security test failed");
    ASSERT(ctx.result.scenarios[0].passed_count == 5, "should pass 5 steps");
    aitp_int_destroy(&ctx);
    PASS();
}

static void test_compute_flow(void) {
    TEST("compute flow scenario");
    aitp_int_ctx_t ctx;
    aitp_int_init(&ctx);
    ctx.config.verbose = 0;
    int r = aitp_int_test_compute_flow(&ctx);
    ASSERT(r == 0, "compute test failed");
    ASSERT(ctx.result.scenarios[0].passed_count == 4, "should pass 4 steps");
    aitp_int_destroy(&ctx);
    PASS();
}

static void test_node_join(void) {
    TEST("node onboarding scenario");
    aitp_int_ctx_t ctx;
    aitp_int_init(&ctx);
    ctx.config.verbose = 0;
    int r = aitp_int_test_node_join(&ctx);
    ASSERT(r == 0, "node join failed");
    ASSERT(ctx.result.scenarios[0].passed_count == 6, "should pass 6 steps");
    aitp_int_destroy(&ctx);
    PASS();
}

static void test_task_lifecycle(void) {
    TEST("task lifecycle scenario");
    aitp_int_ctx_t ctx;
    aitp_int_init(&ctx);
    ctx.config.verbose = 0;
    int r = aitp_int_test_task_lifecycle(&ctx);
    ASSERT(r == 0, "lifecycle test failed");
    ASSERT(ctx.result.scenarios[0].passed_count == 8, "should pass 8 steps");
    aitp_int_destroy(&ctx);
    PASS();
}

static void test_run_all(void) {
    TEST("run all scenarios");
    aitp_int_ctx_t ctx;
    aitp_int_init(&ctx);
    ctx.config.verbose = 0;
    int r = aitp_int_run_all(&ctx);
    ASSERT(r == 0, "run all failed");
    ASSERT(ctx.result.scenario_count == 6, "should have 6 scenarios");
    ASSERT(ctx.result.total_failed == 0, "should have 0 failures");
    ASSERT(ctx.result.total_passed > 0, "should have passes");
    aitp_int_destroy(&ctx);
    PASS();
}

static void test_results_api(void) {
    TEST("results API");
    aitp_int_ctx_t ctx;
    aitp_int_init(&ctx);
    ctx.config.verbose = 0;
    aitp_int_run_all(&ctx);
    aitp_int_result_t result;
    int r = aitp_int_get_results(&ctx, &result);
    ASSERT(r == 0, "get_results failed");
    ASSERT(result.scenario_count == 6, "6 scenarios");
    ASSERT(result.total_passed > 0, "has passes");
    aitp_int_destroy(&ctx);
    PASS();
}

static void test_utility_functions(void) {
    TEST("utility functions");
    ASSERT(strcmp(aitp_int_layer_name(AITP_INT_LAYER_GATEWAY), "Gateway") == 0, "Gateway");
    ASSERT(strcmp(aitp_int_layer_name(AITP_INT_LAYER_CRYPTO), "Crypto") == 0, "Crypto");
    ASSERT(strcmp(aitp_int_layer_name(AITP_INT_LAYER_POW), "PoW") == 0, "PoW");
    ASSERT(strcmp(aitp_int_layer_name(AITP_INT_LAYER_ZKP), "ZKP") == 0, "ZKP");
    ASSERT(strcmp(aitp_int_layer_name(AITP_INT_LAYER_CONSENSUS), "Consensus") == 0, "Consensus");
    ASSERT(strcmp(aitp_int_layer_name(AITP_INT_LAYER_SCHEDULER), "Scheduler") == 0, "Scheduler");
    ASSERT(strcmp(aitp_int_layer_name(AITP_INT_LAYER_ADDRESS), "Address") == 0, "Address");
    ASSERT(strcmp(aitp_int_step_status_name(AITP_INT_STEP_PASSED), "PASS") == 0, "PASS");
    ASSERT(strcmp(aitp_int_step_status_name(AITP_INT_STEP_FAILED), "FAIL") == 0, "FAIL");
    ASSERT(strcmp(aitp_int_scenario_type_name(AITP_INT_SCENARIO_FULL_STACK), "Full Stack") == 0, "FullStack");
    PASS();
}

int main(void) {
    printf("=== ai-tp-integration Basic Tests ===\n\n");
    test_init_destroy();
    test_full_stack();
    test_network_only();
    test_security_only();
    test_compute_flow();
    test_node_join();
    test_task_lifecycle();
    test_run_all();
    test_results_api();
    test_utility_functions();
    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
