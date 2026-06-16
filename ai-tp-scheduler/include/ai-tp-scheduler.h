/**
 * @file ai-tp-scheduler.h
 * @brief AI-TP OS AI Task Scheduler API
 * @version 1.0.0
 * @date 2026-06-16
 *
 * Compute Layer Core: AI Task Scheduling and Resource Management
 *
 * "Nodes cannot claim they did work they actually didn't"
 *
 * 1. Task submission with resource requirements
 * 2. Resource matching to available compute nodes
 * 3. Task distribution with dependency resolution (DAG)
 * 4. Execution monitoring with timeout detection
 * 5. Result collection and verification
 * 6. Work verification integrating with ai-tp-consensus
 *
 * Key design principles:
 * - 20% resources for network/compute, 80% for self-use
 * - Verification code / prompt handshake mechanism
 * - Integration with ai-tp-consensus for work proof
 * - Anti-Sybil via integration with ai-tp-pow on task submission
 * - DAG-based task dependencies
 * - Priority-based and resource-fit scheduling
 *
 * Depends on: ai-tp-consensus, ai-tp-pow, ai-tp-gateway
 */

#ifndef AI_TP_SCHEDULER_H
#define AI_TP_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Version ========== */

#define AI_TP_SCHEDULER_VERSION_MAJOR 1
#define AI_TP_SCHEDULER_VERSION_MINOR 0
#define AI_TP_SCHEDULER_VERSION_PATCH 0

/* ========== Constants ========== */

#define AI_TP_SCHEDULER_MAX_TASKS           256
#define AI_TP_SCHEDULER_MAX_NODES           128
#define AI_TP_SCHEDULER_MAX_DEPENDENCIES    16
#define AI_TP_SCHEDULER_NODE_ID_BYTES       32
#define AI_TP_SCHEDULER_MAX_ERROR_MSG       256
#define AI_TP_SCHEDULER_RESULT_HASH_BYTES   32
#define AI_TP_SCHEDULER_PROOF_BYTES         128
#define AI_TP_SCHEDULER_HANDSHAKE_BYTES     64
#define AI_TP_SCHEDULER_DEFAULT_RESOURCE_RATIO  20
#define AI_TP_SCHEDULER_DEFAULT_TIMEOUT_MS      60000
#define AI_TP_SCHEDULER_DEFAULT_MAX_RETRIES     3
#define AI_TP_SCHEDULER_MAX_QUEUE_SIZE          256
#define AI_TP_SCHEDULER_MAX_PRIORITY            100

/* ========== Error Codes ========== */

#define AI_TP_SCHEDULER_OK              0
#define AI_TP_SCHEDULER_ERR_INIT       -1
#define AI_TP_SCHEDULER_ERR_INVALID    -2
#define AI_TP_SCHEDULER_ERR_FULL       -3
#define AI_TP_SCHEDULER_ERR_NOT_FOUND  -4
#define AI_TP_SCHEDULER_ERR_DEPENDENCY -5
#define AI_TP_SCHEDULER_ERR_RESOURCE   -6
#define AI_TP_SCHEDULER_ERR_TIMEOUT    -7
#define AI_TP_SCHEDULER_ERR_VERIFY     -8
#define AI_TP_SCHEDULER_ERR_STATE      -9
#define AI_TP_SCHEDULER_ERR_NO_NODE    -10
#define AI_TP_SCHEDULER_ERR_RATIO      -11

/* ========== Enums ========== */

typedef enum {
    AI_TP_TASK_PENDING    = 0,
    AI_TP_TASK_QUEUED     = 1,
    AI_TP_TASK_SCHEDULED  = 2,
    AI_TP_TASK_RUNNING    = 3,
    AI_TP_TASK_COMPLETED  = 4,
    AI_TP_TASK_FAILED     = 5,
    AI_TP_TASK_CANCELLED  = 6,
    AI_TP_TASK_TIMEOUT    = 7
} ai_tp_task_state_t;

typedef enum {
    AI_TP_TASK_TYPE_TRAINING     = 0,
    AI_TP_TASK_TYPE_INFERENCE    = 1,
    AI_TP_TASK_TYPE_FINE_TUNE    = 2,
    AI_TP_TASK_TYPE_DATA_PROCESS = 3,
    AI_TP_TASK_TYPE_CUSTOM       = 4
} ai_tp_task_type_t;

typedef enum {
    AI_TP_SCHEDULE_FIFO          = 0,
    AI_TP_SCHEDULE_PRIORITY      = 1,
    AI_TP_SCHEDULE_RESOURCE_FIT  = 2,
    AI_TP_SCHEDULE_ROUND_ROBIN   = 3
} ai_tp_schedule_strategy_t;

typedef enum {
    AI_TP_VERIFY_PENDING   = 0,
    AI_TP_VERIFY_PASSED    = 1,
    AI_TP_VERIFY_FAILED    = 2,
    AI_TP_VERIFY_FRAUD     = 3
} ai_tp_verification_status_t;

/* ========== Data Structures ========== */

typedef struct {
    uint32_t gpu_count;
    uint64_t gpu_mem_mb;
    uint32_t cpu_cores;
    uint64_t ram_mb;
    uint64_t disk_mb;
    uint64_t network_mbps;
} ai_tp_resource_spec_t;

typedef struct {
    uint64_t    task_id;
    ai_tp_task_type_t type;
    ai_tp_task_state_t state;
    uint32_t    priority;
    ai_tp_resource_spec_t resource_req;
    uint8_t     submitter_id[AI_TP_SCHEDULER_NODE_ID_BYTES];
    uint8_t     assigned_node[AI_TP_SCHEDULER_NODE_ID_BYTES];
    time_t      submit_time;
    time_t      start_time;
    time_t      end_time;
    uint64_t    timeout_ms;
    uint64_t    dependency_ids[AI_TP_SCHEDULER_MAX_DEPENDENCIES];
    uint32_t    dependency_count;
    uint8_t     result_hash[AI_TP_SCHEDULER_RESULT_HASH_BYTES];
    char        error_msg[AI_TP_SCHEDULER_MAX_ERROR_MSG];
    uint32_t    retry_count;
    uint8_t     handshake_code[AI_TP_SCHEDULER_HANDSHAKE_BYTES];
} ai_tp_task_t;

typedef struct {
    uint8_t     node_id[AI_TP_SCHEDULER_NODE_ID_BYTES];
    ai_tp_resource_spec_t available_resources;
    bool        supported_types[5];
    double      reliability_score;
    uint32_t    current_load;
    uint64_t    tasks_completed;
    uint64_t    tasks_failed;
    uint64_t    tasks_timed_out;
    uint64_t    total_compute_time;
    bool        online;
    time_t      last_seen;
} ai_tp_node_capability_t;

typedef struct {
    uint32_t    max_queue_size;
    uint64_t    default_timeout_ms;
    uint32_t    max_retries;
    ai_tp_schedule_strategy_t strategy;
    uint8_t     resource_ratio;
    bool        require_verification;
    bool        require_handshake;
} ai_tp_scheduler_config_t;

typedef struct {
    uint64_t    task_id;
    uint8_t     output_data_hash[AI_TP_SCHEDULER_RESULT_HASH_BYTES];
    uint64_t    output_size;
    uint8_t     compute_proof[AI_TP_SCHEDULER_PROOF_BYTES];
    size_t      proof_len;
    ai_tp_verification_status_t verification_status;
} ai_tp_task_result_t;

typedef struct {
    ai_tp_scheduler_config_t config;
    ai_tp_task_t             tasks[AI_TP_SCHEDULER_MAX_TASKS];
    uint32_t                 task_count;
    uint64_t                 next_task_id;
    ai_tp_node_capability_t  nodes[AI_TP_SCHEDULER_MAX_NODES];
    uint32_t                 node_count;
    uint32_t                 round_robin_index;
    bool                     initialized;
} ai_tp_scheduler_context_t;

/* ============================================================
 * Core API
 * ============================================================ */

/* Initialize / Destroy */
int  ai_tp_scheduler_init(ai_tp_scheduler_context_t *ctx);
void ai_tp_scheduler_destroy(ai_tp_scheduler_context_t *ctx);

/* ---- Task Management ---- */

uint64_t ai_tp_scheduler_submit_task(ai_tp_scheduler_context_t *ctx,
                                      ai_tp_task_type_t type,
                                      const ai_tp_resource_spec_t *req,
                                      const uint8_t submitter_id[AI_TP_SCHEDULER_NODE_ID_BYTES],
                                      uint32_t priority);

int ai_tp_scheduler_cancel_task(ai_tp_scheduler_context_t *ctx, uint64_t task_id);

int ai_tp_scheduler_set_task_dependencies(ai_tp_scheduler_context_t *ctx,
                                           uint64_t task_id,
                                           const uint64_t *dep_ids, uint32_t dep_count);

const ai_tp_task_t *ai_tp_scheduler_get_task(ai_tp_scheduler_context_t *ctx,
                                               uint64_t task_id);

ai_tp_task_state_t ai_tp_scheduler_get_task_state(ai_tp_scheduler_context_t *ctx,
                                                    uint64_t task_id);

/* ---- Node Management ---- */

int ai_tp_scheduler_register_node(ai_tp_scheduler_context_t *ctx,
                                   const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES],
                                   const ai_tp_resource_spec_t *resources,
                                   const bool supported_types[5]);

int ai_tp_scheduler_unregister_node(ai_tp_scheduler_context_t *ctx,
                                     const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES]);

int ai_tp_scheduler_update_node_resources(ai_tp_scheduler_context_t *ctx,
                                           const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES],
                                           const ai_tp_resource_spec_t *resources);

const ai_tp_node_capability_t *ai_tp_scheduler_get_node(ai_tp_scheduler_context_t *ctx,
                                                          const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES]);

/* ---- Scheduling ---- */

int ai_tp_scheduler_schedule_next(ai_tp_scheduler_context_t *ctx);

int ai_tp_scheduler_assign_task(ai_tp_scheduler_context_t *ctx,
                                 uint64_t task_id,
                                 const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES]);

int ai_tp_scheduler_run_cycle(ai_tp_scheduler_context_t *ctx);

/* ---- Result Management ---- */

int ai_tp_scheduler_report_result(ai_tp_scheduler_context_t *ctx,
                                   const ai_tp_task_result_t *result);

bool ai_tp_scheduler_verify_result(ai_tp_scheduler_context_t *ctx,
                                    const ai_tp_task_result_t *result);

/* ---- Timeout and Retry ---- */

int ai_tp_scheduler_check_timeouts(ai_tp_scheduler_context_t *ctx);

int ai_tp_scheduler_retry_task(ai_tp_scheduler_context_t *ctx, uint64_t task_id);

/* ---- Configuration ---- */

void ai_tp_scheduler_set_config(ai_tp_scheduler_context_t *ctx,
                                 const ai_tp_scheduler_config_t *config);

const ai_tp_scheduler_config_t *ai_tp_scheduler_get_config(
    const ai_tp_scheduler_context_t *ctx);

void ai_tp_scheduler_set_strategy(ai_tp_scheduler_context_t *ctx,
                                   ai_tp_schedule_strategy_t strategy);

void ai_tp_scheduler_set_resource_ratio(ai_tp_scheduler_context_t *ctx,
                                         uint8_t ratio);

/* ---- Utility ---- */

uint32_t ai_tp_scheduler_get_queue_size(const ai_tp_scheduler_context_t *ctx);

bool ai_tp_scheduler_is_node_available(const ai_tp_scheduler_context_t *ctx,
                                        const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES]);

const char *ai_tp_scheduler_task_state_name(ai_tp_task_state_t state);
const char *ai_tp_scheduler_task_type_name(ai_tp_task_type_t type);
const char *ai_tp_scheduler_strategy_name(ai_tp_schedule_strategy_t strategy);
const char *ai_tp_scheduler_verify_status_name(ai_tp_verification_status_t status);
const char *ai_tp_scheduler_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_SCHEDULER_H */
