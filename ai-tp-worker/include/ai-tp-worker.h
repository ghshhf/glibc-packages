/**
 * ai-tp-worker.h - Remote Compute Worker Agent for AI-TP Mesh
 *
 * Runs on every device in the mesh. Responsibilities:
 *  1. Resource monitoring (CPU/RAM/Disk/GPU/battery)
 *  2. 20% quota enforcement (max 20% of each resource for network tasks)
 *  3. Task execution sandbox (isolated process for remote tasks)
 *  4. Progress/result reporting to scheduler via ai-tp-lan messaging
 *  5. Local task prioritization (user's own tasks always preempt network tasks)
 *
 * Integration: ai-tp-lan (discovery+transport) + ai-tp-scheduler (task dispatch)
 */
#ifndef AI_TP_WORKER_H
#define AI_TP_WORKER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Constants
 * ============================================================ */

#define AITP_WORKER_VERSION_MAJOR      1
#define AITP_WORKER_VERSION_MINOR      0

#define AITP_WORKER_MAX_TASKS          64
#define AITP_WORKER_MAX_TASK_NAME      128
#define AITP_WORKER_MAX_TASK_DATA      65536
#define AITP_WORKER_MAX_RESULT_DATA    65536
#define AITP_WORKER_MAX_ERROR_MSG      256
#define AITP_WORKER_MAX_NODE_ID        32
#define AITP_WORKER_DEFAULT_PORT       49100

/* Resource quota: 20% default */
#define AITP_WORKER_QUOTA_RATIO        20
#define AITP_WORKER_QUOTA_DENOM        100

/* Monitoring intervals */
#define AITP_WORKER_MONITOR_INTERVAL_MS 2000
#define AITP_WORKER_REPORT_INTERVAL_MS  5000

/* ============================================================
 * Enums
 * ============================================================ */

typedef enum aitp_worker_task_state {
    AITP_WORK_TASK_PENDING    = 0,
    AITP_WORK_TASK_RUNNING    = 1,
    AITP_WORK_TASK_COMPLETED  = 2,
    AITP_WORK_TASK_FAILED     = 3,
    AITP_WORK_TASK_CANCELLED  = 4,
    AITP_WORK_TASK_QUEUED     = 5,   /* queued due to local task priority */
} aitp_worker_task_state_t;

typedef enum aitp_worker_resource_type {
    AITP_WORK_RES_CPU      = 0,
    AITP_WORK_RES_RAM      = 1,
    AITP_WORK_RES_DISK     = 2,
    AITP_WORK_RES_GPU      = 3,
    AITP_WORK_RES_NETWORK  = 4,
    AITP_WORK_RES_BATTERY  = 5,
} aitp_worker_resource_type_t;

/* ============================================================
 * Data Structures
 * ============================================================ */

/**
 * Real-time resource snapshot of the local device.
 * Monitor collects this periodically.
 */
typedef struct aitp_worker_resources {
    /* CPU */
    uint32_t cpu_cores_total;
    uint32_t cpu_cores_available;       /* after reserving for local use */
    float    cpu_usage_pct;             /* 0.0 - 100.0 */
    float    cpu_temp_celsius;

    /* RAM */
    uint64_t ram_total_mb;
    uint64_t ram_available_mb;          /* after 20% reduction for quota */
    uint64_t ram_used_by_network_mb;

    /* Disk (storage contribution) */
    uint64_t disk_total_mb;
    uint64_t disk_contributed_mb;       /* 20% of total */
    uint64_t disk_used_by_network_mb;

    /* GPU */
    int      gpu_count;
    float    gpu_usage_pct;

    /* Battery (important for mobile devices) */
    int      battery_pct;               /* -1 if not applicable */
    bool     is_charging;

    /* Network */
    uint64_t bandwidth_up_bps;
    uint64_t bandwidth_down_bps;

    /* Quota enforcement */
    uint32_t quota_pct;                 /* 0-100, default 20 */
    uint32_t local_task_priority;       /* higher = more reserved for local */
} aitp_worker_resources_t;

/**
 * A single task received from the network scheduler.
 */
typedef struct aitp_worker_task {
    char     task_id[64];
    char     task_name[AITP_WORKER_MAX_TASK_NAME];
    aitp_worker_task_state_t state;
    uint8_t  submitter_node[AITP_WORKER_MAX_NODE_ID];

    /* Execution */
    uint8_t  input_data[AITP_WORKER_MAX_TASK_DATA];
    size_t   input_len;
    uint8_t  result_data[AITP_WORKER_MAX_RESULT_DATA];
    size_t   result_len;
    char     error_msg[AITP_WORKER_MAX_ERROR_MSG];

    /* Resource requirements (from scheduler) */
    uint32_t required_cpu_cores;
    uint64_t required_ram_mb;
    uint64_t required_disk_mb;
    uint32_t timeout_secs;

    /* Timing */
    uint64_t received_ms;
    uint64_t started_ms;
    uint64_t completed_ms;
    uint64_t last_progress_ms;

    /* Progress 0.0 - 1.0 */
    float    progress;
    char     progress_msg[128];

    /* Flags */
    bool     is_local;                  /* true = user's own task, preempts */
    bool     is_network;                /* true = contributed via 20% */
} aitp_worker_task_t;

/**
 * Callbacks for worker events.
 */
typedef struct aitp_worker_callbacks {
    void (*on_task_received)(const char *task_id);
    void (*on_task_started)(const char *task_id);
    void (*on_task_progress)(const char *task_id, float progress, const char *msg);
    void (*on_task_completed)(const char *task_id, const uint8_t *result, size_t len);
    void (*on_task_failed)(const char *task_id, const char *error);
    void (*on_resources_updated)(const aitp_worker_resources_t *res);
    void (*on_quota_exceeded)(aitp_worker_resource_type_t type, float requested, float available);
} aitp_worker_callbacks_t;

/** Main worker context */
typedef struct aitp_worker_ctx {
    aitp_worker_task_t    tasks[AITP_WORKER_MAX_TASKS];
    int                   task_count;
    aitp_worker_resources_t resources;
    aitp_worker_callbacks_t callbacks;
    uint8_t               node_id[AITP_WORKER_MAX_NODE_ID];
    char                  listen_ip[64];
    uint16_t              listen_port;
    int                   listen_sock;
    bool                  running;
    bool                  initialized;
    uint64_t              last_monitor_ms;
    uint64_t              last_report_ms;
    /* Network task counter for quota tracking */
    int                   active_network_tasks;
} aitp_worker_ctx_t;

/* ============================================================
 * API Functions
 * ============================================================ */

/* Lifecycle */
int  aitp_worker_init(aitp_worker_ctx_t *ctx, const uint8_t node_id[AITP_WORKER_MAX_NODE_ID]);
void aitp_worker_destroy(aitp_worker_ctx_t *ctx);
int  aitp_worker_start(aitp_worker_ctx_t *ctx);
void aitp_worker_stop(aitp_worker_ctx_t *ctx);

/* Resource management */
int  aitp_worker_monitor_resources(aitp_worker_ctx_t *ctx);
int  aitp_worker_set_quota(aitp_worker_ctx_t *ctx, uint32_t quota_pct);
int  aitp_worker_get_quota(aitp_worker_ctx_t *ctx);
int  aitp_worker_get_resources(aitp_worker_ctx_t *ctx, aitp_worker_resources_t *out_res);
bool aitp_worker_can_accept_task(aitp_worker_ctx_t *ctx, uint32_t cpu, uint64_t ram, uint64_t disk);

/* Task management */
int  aitp_worker_submit_task(aitp_worker_ctx_t *ctx, const aitp_worker_task_t *task);
int  aitp_worker_cancel_task(aitp_worker_ctx_t *ctx, const char *task_id);
int  aitp_worker_get_task(aitp_worker_ctx_t *ctx, const char *task_id, aitp_worker_task_t *out_task);
int  aitp_worker_get_tasks(aitp_worker_ctx_t *ctx, aitp_worker_task_t *out_tasks, int max);
int  aitp_worker_get_active_count(aitp_worker_ctx_t *ctx);

/* Execution simulation (stub: logs execution) */
int  aitp_worker_execute_task(aitp_worker_ctx_t *ctx, const char *task_id);
int  aitp_worker_report_progress(aitp_worker_ctx_t *ctx, const char *task_id,
                                  float progress, const char *msg);
int  aitp_worker_complete_task(aitp_worker_ctx_t *ctx, const char *task_id,
                                const uint8_t *result, size_t len);
int  aitp_worker_fail_task(aitp_worker_ctx_t *ctx, const char *task_id, const char *error);

/* Tick (call periodically for resource monitoring + timeout detection) */
int  aitp_worker_tick(aitp_worker_ctx_t *ctx);

/* Callbacks */
int  aitp_worker_set_callbacks(aitp_worker_ctx_t *ctx, const aitp_worker_callbacks_t *cbs);

/* Utility */
const char* aitp_worker_state_name(aitp_worker_task_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_WORKER_H */
