/**
 * ai-tp-integration.h - End-to-End Integration Test Framework
 *
 * Tests the full AI-TP security stack:
 *   Layer 1: Gateway (ai-tp-gateway) - Access control, handshake
 *   Layer 2: Crypto (ai-tp-crypto) - Encryption, signatures
 *   Layer 3: PoW (ai-tp-pow) - Anti-Sybil, identity cost
 *   Layer 4: ZKP (ai-tp-zkp) - Privacy-preserving verification
 *   Layer 5: Consensus (ai-tp-consensus) - Anti-cheat, trust
 *   Compute: Scheduler (ai-tp-scheduler) - Task distribution
 *   Network: Address (ai-tp-address) - Decentralized naming
 */

#ifndef AI_TP_INTEGRATION_H
#define AI_TP_INTEGRATION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Constants ========== */

#define AITP_INT_MAX_LAYERS     7
#define AITP_INT_MAX_SCENARIOS  32
#define AITP_INT_MAX_STEPS      64
#define AITP_INT_MAX_LOG        4096

/* ========== Enumerations ========== */

typedef enum aitp_int_layer {
    AITP_INT_LAYER_GATEWAY   = 0,
    AITP_INT_LAYER_CRYPTO    = 1,
    AITP_INT_LAYER_POW       = 2,
    AITP_INT_LAYER_ZKP       = 3,
    AITP_INT_LAYER_CONSENSUS = 4,
    AITP_INT_LAYER_SCHEDULER = 5,
    AITP_INT_LAYER_ADDRESS   = 6
} aitp_int_layer_t;

typedef enum aitp_int_step_status {
    AITP_INT_STEP_PENDING  = 0,
    AITP_INT_STEP_RUNNING  = 1,
    AITP_INT_STEP_PASSED   = 2,
    AITP_INT_STEP_FAILED   = 3,
    AITP_INT_STEP_SKIPPED  = 4
} aitp_int_step_status_t;

typedef enum aitp_int_scenario_type {
    AITP_INT_SCENARIO_FULL_STACK     = 0,  /* All 7 layers */
    AITP_INT_SCENARIO_NETWORK_ONLY   = 1,  /* Gateway + Address + Crypto */
    AITP_INT_SCENARIO_SECURITY_ONLY  = 2,  /* Crypto + PoW + ZKP + Consensus */
    AITP_INT_SCENARIO_COMPUTE_FLOW   = 3,  /* Scheduler + Consensus + PoW */
    AITP_INT_SCENARIO_NODE_JOIN      = 4,  /* Full node onboarding */
    AITP_INT_SCENARIO_TASK_LIFECYCLE = 5   /* Submit → Schedule → Execute → Verify */
} aitp_int_scenario_type_t;

/* ========== Data Structures ========== */

typedef struct aitp_int_step {
    char     name[64];
    aitp_int_layer_t layer;
    aitp_int_step_status_t status;
    uint64_t start_ms;
    uint64_t end_ms;
    char     detail[256];
} aitp_int_step_t;

typedef struct aitp_int_scenario {
    char     name[64];
    aitp_int_scenario_type_t type;
    aitp_int_step_t steps[AITP_INT_MAX_STEPS];
    int      step_count;
    int      passed_count;
    int      failed_count;
    int      skipped_count;
    uint64_t total_ms;
} aitp_int_scenario_t;

typedef struct aitp_int_result {
    aitp_int_scenario_t scenarios[AITP_INT_MAX_SCENARIOS];
    int scenario_count;
    int total_passed;
    int total_failed;
    int total_skipped;
    uint64_t total_ms;
    char log[AITP_INT_MAX_LOG];
    int log_len;
} aitp_int_result_t;

typedef struct aitp_int_config {
    int verbose;
    int stop_on_failure;
    int run_security_only;
    int run_compute_only;
    int run_network_only;
} aitp_int_config_t;

typedef struct aitp_int_ctx {
    aitp_int_config_t config;
    aitp_int_result_t result;
    int initialized;
} aitp_int_ctx_t;

/* ========== API Functions ========== */

int aitp_int_init(aitp_int_ctx_t *ctx);
int aitp_int_init_with_config(aitp_int_ctx_t *ctx, const aitp_int_config_t *config);
void aitp_int_destroy(aitp_int_ctx_t *ctx);

/* Run all integration scenarios */
int aitp_int_run_all(aitp_int_ctx_t *ctx);

/* Run specific scenario type */
int aitp_int_run_scenario(aitp_int_ctx_t *ctx, aitp_int_scenario_type_t type);

/* Individual scenario runners */
int aitp_int_test_full_stack(aitp_int_ctx_t *ctx);
int aitp_int_test_network_only(aitp_int_ctx_t *ctx);
int aitp_int_test_security_only(aitp_int_ctx_t *ctx);
int aitp_int_test_compute_flow(aitp_int_ctx_t *ctx);
int aitp_int_test_node_join(aitp_int_ctx_t *ctx);
int aitp_int_test_task_lifecycle(aitp_int_ctx_t *ctx);

/* Results */
int aitp_int_get_results(aitp_int_ctx_t *ctx, aitp_int_result_t *out);
void aitp_int_print_report(aitp_int_ctx_t *ctx);

/* Utility */
const char *aitp_int_layer_name(aitp_int_layer_t layer);
const char *aitp_int_step_status_name(aitp_int_step_status_t status);
const char *aitp_int_scenario_type_name(aitp_int_scenario_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_INTEGRATION_H */
