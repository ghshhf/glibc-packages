/**
 * @file ai-tp-scheduler.c
 * @brief AI-TP OS AI Task Scheduler implementation
 * @version 1.0.0
 */

#include "ai-tp-scheduler.h"
#include "libaitp-common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
 * Internal Helpers
 * ============================================================ */

static time_t current_time_ms(void) {
    return (time_t)(clock() * 1000 / CLOCKS_PER_SEC);
}

static int find_task_index(ai_tp_scheduler_context_t *ctx, uint64_t task_id) {
    for (uint32_t i = 0; i < ctx->task_count; i++) {
        if (ctx->tasks[i].task_id == task_id) return (int)i;
    }
    return -1;
}

static int find_node_index(ai_tp_scheduler_context_t *ctx,
                            const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES]) {
    for (uint32_t i = 0; i < ctx->node_count; i++) {
        if (memcmp(ctx->nodes[i].node_id, node_id, AI_TP_SCHEDULER_NODE_ID_BYTES) == 0)
            return (int)i;
    }
    return -1;
}

static bool resource_fits(const ai_tp_resource_spec_t *node_res,
                           const ai_tp_resource_spec_t *task_req) {
    if (node_res->gpu_count < task_req->gpu_count) return false;
    if (node_res->gpu_mem_mb < task_req->gpu_mem_mb) return false;
    if (node_res->cpu_cores < task_req->cpu_cores) return false;
    if (node_res->ram_mb < task_req->ram_mb) return false;
    if (node_res->disk_mb < task_req->disk_mb) return false;
    if (node_res->network_mbps < task_req->network_mbps) return false;
    return true;
}

static bool node_supports_type(const ai_tp_node_capability_t *node,
                                ai_tp_task_type_t type) {
    if (type < 0 || type > 4) return false;
    return node->supported_types[type];
}

static bool dependencies_met(ai_tp_scheduler_context_t *ctx, const ai_tp_task_t *task) {
    for (uint32_t i = 0; i < task->dependency_count; i++) {
        int idx = find_task_index(ctx, task->dependency_ids[i]);
        if (idx < 0) return false;
        if (ctx->tasks[idx].state != AI_TP_TASK_COMPLETED) return false;
    }
    return true;
}

static int find_node_for_task(ai_tp_scheduler_context_t *ctx,
                               const ai_tp_task_t *task) {
    int best_idx = -1;
    double best_score = -1.0;

    for (uint32_t i = 0; i < ctx->node_count; i++) {
        const ai_tp_node_capability_t *node = &ctx->nodes[i];
        if (!node->online) continue;
        if (!node_supports_type(node, task->type)) continue;
        if (!resource_fits(&node->available_resources, &task->resource_req)) continue;

        /* Score: reliability + inverse load, lower load is better */
        double score = node->reliability_score * 10.0;
        if (node->current_load > 0)
            score /= (double)(node->current_load + 1);
        score += node->reliability_score * 5.0;

        if (score > best_score) {
            best_score = score;
            best_idx = (int)i;
        }
    }
    return best_idx;
}

static void update_node_reliability(ai_tp_scheduler_context_t *ctx,
                                     const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES],
                                     bool success) {
    int idx = find_node_index(ctx, node_id);
    if (idx < 0) return;

    ai_tp_node_capability_t *node = &ctx->nodes[idx];
    if (success) {
        node->tasks_completed++;
        double completed = (double)node->tasks_completed;
        double failed = (double)node->tasks_failed;
        double timed_out = (double)node->tasks_timed_out;
        double total = completed + failed + timed_out;
        node->reliability_score = (total > 0) ? completed / total : 0.5;
    } else {
        node->tasks_failed++;
        double completed = (double)node->tasks_completed;
        double failed = (double)node->tasks_failed;
        double timed_out = (double)node->tasks_timed_out;
        double total = completed + failed + timed_out;
        node->reliability_score = (total > 0) ? completed / total : 0.5;
    }
}

static uint32_t count_queued_tasks(ai_tp_scheduler_context_t *ctx) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ctx->task_count; i++) {
        if (ctx->tasks[i].state == AI_TP_TASK_QUEUED) count++;
    }
    return count;
}

static bool is_external_task(ai_tp_scheduler_context_t *ctx,
                              const ai_tp_task_t *task) {
    /* Tasks submitted by unknown nodes are external */
    for (uint32_t i = 0; i < ctx->node_count; i++) {
        if (memcmp(ctx->nodes[i].node_id, task->submitter_id,
                   AI_TP_SCHEDULER_NODE_ID_BYTES) == 0)
            return false;
    }
    return true;
}

static uint32_t count_external_active(ai_tp_scheduler_context_t *ctx) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ctx->task_count; i++) {
        if ((ctx->tasks[i].state == AI_TP_TASK_RUNNING ||
             ctx->tasks[i].state == AI_TP_TASK_SCHEDULED) &&
            is_external_task(ctx, &ctx->tasks[i]))
            count++;
    }
    return count;
}

static int topological_order(ai_tp_scheduler_context_t *ctx,
                              uint32_t *order, uint32_t *order_count) {
    uint32_t remaining[AI_TP_SCHEDULER_MAX_TASKS];
    uint32_t rem_count = 0;
    bool visited[AI_TP_SCHEDULER_MAX_TASKS];
    memset(visited, 0, sizeof(visited));
    *order_count = 0;

    /* Collect all queued/pending tasks */
    for (uint32_t i = 0; i < ctx->task_count; i++) {
        if (ctx->tasks[i].state == AI_TP_TASK_QUEUED ||
            ctx->tasks[i].state == AI_TP_TASK_PENDING) {
            remaining[rem_count++] = i;
        }
    }

    if (rem_count == 0) return 0;

    /* Simple topological sort by dependency count */
    for (uint32_t round = 0; round < rem_count && *order_count < rem_count; round++) {
        bool added = false;
        for (uint32_t i = 0; i < rem_count; i++) {
            uint32_t idx = remaining[i];
            if (visited[i]) continue;
            if (dependencies_met(ctx, &ctx->tasks[idx])) {
                order[*order_count] = idx;
                (*order_count)++;
                visited[i] = true;
                added = true;
            }
        }
        if (!added) break; /* Circular dependency or unmet deps */
    }
    return 0;
}

/* Compute proof: H(node_id || task_id || output_hash || timestamp) */
static void generate_compute_proof(ai_tp_scheduler_context_t *ctx,
                                    const ai_tp_task_result_t *result,
                                    const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES],
                                    uint8_t *proof_out, size_t *proof_len) {
    uint8_t input[AI_TP_SCHEDULER_NODE_ID_BYTES + 8 + AI_TP_SCHEDULER_RESULT_HASH_BYTES + 8];
    size_t pos = 0;
    memcpy(input + pos, node_id, AI_TP_SCHEDULER_NODE_ID_BYTES);
    pos += AI_TP_SCHEDULER_NODE_ID_BYTES;
    for (int i = 0; i < 8; i++) input[pos + i] = (uint8_t)(result->task_id >> (i * 8));
    pos += 8;
    memcpy(input + pos, result->output_data_hash, AI_TP_SCHEDULER_RESULT_HASH_BYTES);
    pos += AI_TP_SCHEDULER_RESULT_HASH_BYTES;
    time_t now = current_time_ms();
    for (int i = 0; i < 8; i++) input[pos + i] = (uint8_t)(now >> (i * 8));
    pos += 8;

    aitp_stub_hash(input, pos, proof_out, AI_TP_SCHEDULER_PROOF_BYTES);
    *proof_len = AI_TP_SCHEDULER_PROOF_BYTES;
}

/* Verify compute proof */
static bool verify_compute_proof(ai_tp_scheduler_context_t *ctx,
                                  const ai_tp_task_result_t *result,
                                  const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES]) {
    if (result->proof_len == 0) return false;
    if (result->proof_len > AI_TP_SCHEDULER_PROOF_BYTES) return false;

    /* Re-generate and compare */
    uint8_t expected[AI_TP_SCHEDULER_PROOF_BYTES];
    size_t expected_len;
    generate_compute_proof(ctx, result, node_id, expected, &expected_len);

    return memcmp(result->compute_proof, expected, expected_len) == 0;
}

/* ============================================================
 * Core API
 * ============================================================ */

int ai_tp_scheduler_init(ai_tp_scheduler_context_t *ctx) {
    if (!ctx) return AI_TP_SCHEDULER_ERR_INVALID;
    memset(ctx, 0, sizeof(*ctx));

    ctx->config.max_queue_size      = AI_TP_SCHEDULER_MAX_QUEUE_SIZE;
    ctx->config.default_timeout_ms  = AI_TP_SCHEDULER_DEFAULT_TIMEOUT_MS;
    ctx->config.max_retries         = AI_TP_SCHEDULER_DEFAULT_MAX_RETRIES;
    ctx->config.strategy            = AI_TP_SCHEDULE_PRIORITY;
    ctx->config.resource_ratio      = AI_TP_SCHEDULER_DEFAULT_RESOURCE_RATIO;
    ctx->config.require_verification = true;
    ctx->config.require_handshake   = true;
    ctx->next_task_id               = 1;
    ctx->round_robin_index          = 0;
    ctx->initialized                = true;

    return AI_TP_SCHEDULER_OK;
}

void ai_tp_scheduler_destroy(ai_tp_scheduler_context_t *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
}

/* ============================================================
 * Task Management
 * ============================================================ */

uint64_t ai_tp_scheduler_submit_task(ai_tp_scheduler_context_t *ctx,
                                      ai_tp_task_type_t type,
                                      const ai_tp_resource_spec_t *req,
                                      const uint8_t submitter_id[AI_TP_SCHEDULER_NODE_ID_BYTES],
                                      uint32_t priority) {
    if (!ctx || !req || !submitter_id) return 0;
    if (!ctx->initialized) return 0;
    if (ctx->task_count >= AI_TP_SCHEDULER_MAX_TASKS) return 0;

    uint32_t queued = count_queued_tasks(ctx);
    if (queued >= ctx->config.max_queue_size) return 0;

    /* Resource ratio enforcement: check external task limit */
    uint32_t ext_active = count_external_active(ctx);
    bool is_ext = is_external_task(ctx, NULL);
    /* Determine if this is external by checking submitter */
    bool external = true;
    for (uint32_t i = 0; i < ctx->node_count; i++) {
        if (memcmp(ctx->nodes[i].node_id, submitter_id,
                   AI_TP_SCHEDULER_NODE_ID_BYTES) == 0) {
            external = false;
            break;
        }
    }

    if (external) {
        uint32_t total_tasks = ctx->task_count > 0 ? ctx->task_count : 1;
        uint32_t max_ext = (total_tasks * ctx->config.resource_ratio) / 100;
        if (max_ext < 1) max_ext = 1;
        if (ext_active >= max_ext) return 0; /* Ratio limit reached */
    }

    if (priority > AI_TP_SCHEDULER_MAX_PRIORITY)
        priority = AI_TP_SCHEDULER_MAX_PRIORITY;

    ai_tp_task_t *task = &ctx->tasks[ctx->task_count];
    memset(task, 0, sizeof(*task));
    task->task_id       = ctx->next_task_id++;
    task->type          = type;
    task->state         = AI_TP_TASK_QUEUED;
    task->priority      = priority;
    task->resource_req  = *req;
    memcpy(task->submitter_id, submitter_id, AI_TP_SCHEDULER_NODE_ID_BYTES);
    memset(task->assigned_node, 0, AI_TP_SCHEDULER_NODE_ID_BYTES);
    task->submit_time   = time(NULL);
    task->timeout_ms    = ctx->config.default_timeout_ms;
    task->retry_count   = 0;

    /* Generate handshake code: H(submitter_id || task_id) */
    uint8_t hk_input[AI_TP_SCHEDULER_NODE_ID_BYTES + 8];
    memcpy(hk_input, submitter_id, AI_TP_SCHEDULER_NODE_ID_BYTES);
    for (int i = 0; i < 8; i++)
        hk_input[AI_TP_SCHEDULER_NODE_ID_BYTES + i] = (uint8_t)(task->task_id >> (i * 8));
    aitp_stub_hash(hk_input, sizeof(hk_input), task->handshake_code, AI_TP_SCHEDULER_HANDSHAKE_BYTES);

    ctx->task_count++;
    return task->task_id;
}

int ai_tp_scheduler_cancel_task(ai_tp_scheduler_context_t *ctx, uint64_t task_id) {
    if (!ctx) return AI_TP_SCHEDULER_ERR_INVALID;
    int idx = find_task_index(ctx, task_id);
    if (idx < 0) return AI_TP_SCHEDULER_ERR_NOT_FOUND;

    ai_tp_task_t *task = &ctx->tasks[idx];
    if (task->state == AI_TP_TASK_COMPLETED ||
        task->state == AI_TP_TASK_CANCELLED)
        return AI_TP_SCHEDULER_ERR_STATE;

    task->state = AI_TP_TASK_CANCELLED;
    task->end_time = time(NULL);
    return AI_TP_SCHEDULER_OK;
}

int ai_tp_scheduler_set_task_dependencies(ai_tp_scheduler_context_t *ctx,
                                           uint64_t task_id,
                                           const uint64_t *dep_ids, uint32_t dep_count) {
    if (!ctx) return AI_TP_SCHEDULER_ERR_INVALID;
    if (dep_count > AI_TP_SCHEDULER_MAX_DEPENDENCIES)
        return AI_TP_SCHEDULER_ERR_INVALID;

    int idx = find_task_index(ctx, task_id);
    if (idx < 0) return AI_TP_SCHEDULER_ERR_NOT_FOUND;

    ai_tp_task_t *task = &ctx->tasks[idx];
    if (task->state != AI_TP_TASK_QUEUED && task->state != AI_TP_TASK_PENDING)
        return AI_TP_SCHEDULER_ERR_STATE;

    /* Verify all dependencies exist */
    for (uint32_t i = 0; i < dep_count; i++) {
        if (find_task_index(ctx, dep_ids[i]) < 0)
            return AI_TP_SCHEDULER_ERR_DEPENDENCY;
    }

    task->dependency_count = dep_count;
    for (uint32_t i = 0; i < dep_count; i++) {
        task->dependency_ids[i] = dep_ids[i];
    }
    return AI_TP_SCHEDULER_OK;
}

const ai_tp_task_t *ai_tp_scheduler_get_task(ai_tp_scheduler_context_t *ctx,
                                               uint64_t task_id) {
    if (!ctx) return NULL;
    int idx = find_task_index(ctx, task_id);
    if (idx < 0) return NULL;
    return &ctx->tasks[idx];
}

ai_tp_task_state_t ai_tp_scheduler_get_task_state(ai_tp_scheduler_context_t *ctx,
                                                    uint64_t task_id) {
    if (!ctx) return AI_TP_TASK_FAILED;
    int idx = find_task_index(ctx, task_id);
    if (idx < 0) return AI_TP_TASK_FAILED;
    return ctx->tasks[idx].state;
}

/* ============================================================
 * Node Management
 * ============================================================ */

int ai_tp_scheduler_register_node(ai_tp_scheduler_context_t *ctx,
                                   const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES],
                                   const ai_tp_resource_spec_t *resources,
                                   const bool supported_types[5]) {
    if (!ctx || !node_id || !resources || !supported_types) return AI_TP_SCHEDULER_ERR_INVALID;
    if (!ctx->initialized) return AI_TP_SCHEDULER_ERR_INIT;
    if (ctx->node_count >= AI_TP_SCHEDULER_MAX_NODES) return AI_TP_SCHEDULER_ERR_FULL;
    if (find_node_index(ctx, node_id) >= 0) return AI_TP_SCHEDULER_ERR_INVALID;

    ai_tp_node_capability_t *node = &ctx->nodes[ctx->node_count];
    memset(node, 0, sizeof(*node));
    memcpy(node->node_id, node_id, AI_TP_SCHEDULER_NODE_ID_BYTES);
    node->available_resources = *resources;
    for (int i = 0; i < 5; i++) node->supported_types[i] = supported_types[i];
    node->reliability_score = 0.5;
    node->current_load = 0;
    node->online = true;
    node->last_seen = time(NULL);
    ctx->node_count++;

    return AI_TP_SCHEDULER_OK;
}

int ai_tp_scheduler_unregister_node(ai_tp_scheduler_context_t *ctx,
                                     const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES]) {
    if (!ctx || !node_id) return AI_TP_SCHEDULER_ERR_INVALID;
    int idx = find_node_index(ctx, node_id);
    if (idx < 0) return AI_TP_SCHEDULER_ERR_NOT_FOUND;

    /* Mark all assigned tasks as failed */
    for (uint32_t i = 0; i < ctx->task_count; i++) {
        if (ctx->tasks[i].state == AI_TP_TASK_RUNNING &&
            memcmp(ctx->tasks[i].assigned_node, node_id,
                   AI_TP_SCHEDULER_NODE_ID_BYTES) == 0) {
            ctx->tasks[i].state = AI_TP_TASK_FAILED;
            strncpy(ctx->tasks[i].error_msg, "Node unregistered",
                    AI_TP_SCHEDULER_MAX_ERROR_MSG - 1);
            ctx->tasks[i].end_time = time(NULL);
        }
    }

    /* Remove node */
    uint32_t last = ctx->node_count - 1;
    if ((uint32_t)idx != last) {
        ctx->nodes[idx] = ctx->nodes[last];
    }
    ctx->node_count--;
    return AI_TP_SCHEDULER_OK;
}

int ai_tp_scheduler_update_node_resources(ai_tp_scheduler_context_t *ctx,
                                           const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES],
                                           const ai_tp_resource_spec_t *resources) {
    if (!ctx || !node_id || !resources) return AI_TP_SCHEDULER_ERR_INVALID;
    int idx = find_node_index(ctx, node_id);
    if (idx < 0) return AI_TP_SCHEDULER_ERR_NOT_FOUND;
    ctx->nodes[idx].available_resources = *resources;
    ctx->nodes[idx].last_seen = time(NULL);
    return AI_TP_SCHEDULER_OK;
}

const ai_tp_node_capability_t *ai_tp_scheduler_get_node(ai_tp_scheduler_context_t *ctx,
                                                          const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES]) {
    if (!ctx || !node_id) return NULL;
    int idx = find_node_index(ctx, node_id);
    if (idx < 0) return NULL;
    return &ctx->nodes[idx];
}

/* ============================================================
 * Scheduling
 * ============================================================ */

int ai_tp_scheduler_assign_task(ai_tp_scheduler_context_t *ctx,
                                 uint64_t task_id,
                                 const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES]) {
    if (!ctx || !node_id) return AI_TP_SCHEDULER_ERR_INVALID;

    int task_idx = find_task_index(ctx, task_id);
    if (task_idx < 0) return AI_TP_SCHEDULER_ERR_NOT_FOUND;

    int node_idx = find_node_index(ctx, node_id);
    if (node_idx < 0) return AI_TP_SCHEDULER_ERR_NO_NODE;

    ai_tp_task_t *task = &ctx->tasks[task_idx];
    ai_tp_node_capability_t *node = &ctx->nodes[node_idx];

    if (task->state != AI_TP_TASK_QUEUED) return AI_TP_SCHEDULER_ERR_STATE;
    if (!node->online) return AI_TP_SCHEDULER_ERR_NO_NODE;
    if (!node_supports_type(node, task->type)) return AI_TP_SCHEDULER_ERR_RESOURCE;
    if (!resource_fits(&node->available_resources, &task->resource_req))
        return AI_TP_SCHEDULER_ERR_RESOURCE;
    if (!dependencies_met(ctx, task)) return AI_TP_SCHEDULER_ERR_DEPENDENCY;

    task->state = AI_TP_TASK_SCHEDULED;
    memcpy(task->assigned_node, node_id, AI_TP_SCHEDULER_NODE_ID_BYTES);
    task->start_time = time(NULL);
    node->current_load++;

    return AI_TP_SCHEDULER_OK;
}

/* FIFO scheduling: pick the oldest queued task */
static int schedule_fifo(ai_tp_scheduler_context_t *ctx) {
    int best_idx = -1;
    time_t oldest = 0;

    for (uint32_t i = 0; i < ctx->task_count; i++) {
        if (ctx->tasks[i].state != AI_TP_TASK_QUEUED) continue;
        if (!dependencies_met(ctx, &ctx->tasks[i])) continue;
        if (best_idx < 0 || ctx->tasks[i].submit_time < oldest) {
            best_idx = (int)i;
            oldest = ctx->tasks[i].submit_time;
        }
    }
    return best_idx;
}

/* Priority scheduling: pick highest priority, then oldest */
static int schedule_priority(ai_tp_scheduler_context_t *ctx) {
    int best_idx = -1;
    uint32_t best_prio = 0;
    time_t best_time = 0;

    for (uint32_t i = 0; i < ctx->task_count; i++) {
        if (ctx->tasks[i].state != AI_TP_TASK_QUEUED) continue;
        if (!dependencies_met(ctx, &ctx->tasks[i])) continue;

        bool pick = false;
        if (best_idx < 0) {
            pick = true;
        } else if (ctx->tasks[i].priority > best_prio) {
            pick = true;
        } else if (ctx->tasks[i].priority == best_prio &&
                   ctx->tasks[i].submit_time < best_time) {
            pick = true;
        }

        if (pick) {
            best_idx = (int)i;
            best_prio = ctx->tasks[i].priority;
            best_time = ctx->tasks[i].submit_time;
        }
    }
    return best_idx;
}

/* Best resource fit: pick task that best matches available nodes */
static int schedule_resource_fit(ai_tp_scheduler_context_t *ctx) {
    int best_task_idx = -1;
    int best_node_idx = -1;
    double best_fit = -1.0;

    for (uint32_t ti = 0; ti < ctx->task_count; ti++) {
        if (ctx->tasks[ti].state != AI_TP_TASK_QUEUED) continue;
        if (!dependencies_met(ctx, &ctx->tasks[ti])) continue;

        for (uint32_t ni = 0; ni < ctx->node_count; ni++) {
            if (!ctx->nodes[ni].online) continue;
            if (!node_supports_type(&ctx->nodes[ni], ctx->tasks[ti].type)) continue;
            if (!resource_fits(&ctx->nodes[ni].available_resources,
                               &ctx->tasks[ti].resource_req)) continue;

            /* Calculate fit score: how well resources match (lower waste is better) */
            const ai_tp_resource_spec_t *n = &ctx->nodes[ni].available_resources;
            const ai_tp_resource_spec_t *t = &ctx->tasks[ti].resource_req;
            double waste = 0.0;
            if (n->gpu_count > 0)
                waste += (double)(n->gpu_count - t->gpu_count) / (double)n->gpu_count;
            if (n->gpu_mem_mb > 0)
                waste += (double)(n->gpu_mem_mb - t->gpu_mem_mb) / (double)n->gpu_mem_mb;
            if (n->cpu_cores > 0)
                waste += (double)(n->cpu_cores - t->cpu_cores) / (double)n->cpu_cores;
            if (n->ram_mb > 0)
                waste += (double)(n->ram_mb - t->ram_mb) / (double)n->ram_mb;

            double fit = 1.0 / (waste + 0.001);
            fit += ctx->nodes[ni].reliability_score * 5.0;
            fit += (double)ctx->tasks[ti].priority / 100.0;

            if (fit > best_fit) {
                best_fit = fit;
                best_task_idx = (int)ti;
                best_node_idx = (int)ni;
            }
        }
    }

    if (best_task_idx >= 0 && best_node_idx >= 0) {
        ai_tp_task_t *task = &ctx->tasks[best_task_idx];
        task->state = AI_TP_TASK_SCHEDULED;
        memcpy(task->assigned_node, ctx->nodes[best_node_idx].node_id,
               AI_TP_SCHEDULER_NODE_ID_BYTES);
        task->start_time = time(NULL);
        ctx->nodes[best_node_idx].current_load++;
    }
    return (best_task_idx >= 0) ? 0 : -1;
}

/* Round-robin scheduling: pick next queued task, assign to next available node */
static int schedule_round_robin(ai_tp_scheduler_context_t *ctx) {
    if (ctx->node_count == 0) return -1;

    /* Find next queued task */
    int task_idx = -1;
    for (uint32_t i = 0; i < ctx->task_count; i++) {
        if (ctx->tasks[i].state != AI_TP_TASK_QUEUED) continue;
        if (!dependencies_met(ctx, &ctx->tasks[i])) continue;
        task_idx = (int)i;
        break;
    }
    if (task_idx < 0) return -1;

    /* Find next available node in round-robin order */
    for (uint32_t attempt = 0; attempt < ctx->node_count; attempt++) {
        uint32_t ni = ctx->round_robin_index;
        ctx->round_robin_index = (ctx->round_robin_index + 1) % ctx->node_count;

        ai_tp_node_capability_t *node = &ctx->nodes[ni];
        if (!node->online) continue;
        if (!node_supports_type(node, ctx->tasks[task_idx].type)) continue;
        if (!resource_fits(&node->available_resources,
                           &ctx->tasks[task_idx].resource_req)) continue;

        ctx->tasks[task_idx].state = AI_TP_TASK_SCHEDULED;
        memcpy(ctx->tasks[task_idx].assigned_node, node->node_id,
               AI_TP_SCHEDULER_NODE_ID_BYTES);
        ctx->tasks[task_idx].start_time = time(NULL);
        node->current_load++;
        return 0;
    }
    return -1;
}

int ai_tp_scheduler_schedule_next(ai_tp_scheduler_context_t *ctx) {
    if (!ctx || !ctx->initialized) return AI_TP_SCHEDULER_ERR_INVALID;

    switch (ctx->config.strategy) {
    case AI_TP_SCHEDULE_FIFO: {
        int idx = schedule_fifo(ctx);
        if (idx < 0) return AI_TP_SCHEDULER_ERR_NOT_FOUND;
        int node = find_node_for_task(ctx, &ctx->tasks[idx]);
        if (node < 0) return AI_TP_SCHEDULER_ERR_NO_NODE;
        return ai_tp_scheduler_assign_task(ctx, ctx->tasks[idx].task_id,
                                            ctx->nodes[node].node_id);
    }
    case AI_TP_SCHEDULE_PRIORITY: {
        int idx = schedule_priority(ctx);
        if (idx < 0) return AI_TP_SCHEDULER_ERR_NOT_FOUND;
        int node = find_node_for_task(ctx, &ctx->tasks[idx]);
        if (node < 0) return AI_TP_SCHEDULER_ERR_NO_NODE;
        return ai_tp_scheduler_assign_task(ctx, ctx->tasks[idx].task_id,
                                            ctx->nodes[node].node_id);
    }
    case AI_TP_SCHEDULE_RESOURCE_FIT: {
        int ret = schedule_resource_fit(ctx);
        return (ret == 0) ? AI_TP_SCHEDULER_OK : AI_TP_SCHEDULER_ERR_NOT_FOUND;
    }
    case AI_TP_SCHEDULE_ROUND_ROBIN: {
        int ret = schedule_round_robin(ctx);
        return (ret == 0) ? AI_TP_SCHEDULER_OK : AI_TP_SCHEDULER_ERR_NOT_FOUND;
    }
    default:
        return AI_TP_SCHEDULER_ERR_INVALID;
    }
}

int ai_tp_scheduler_run_cycle(ai_tp_scheduler_context_t *ctx) {
    if (!ctx) return AI_TP_SCHEDULER_ERR_INVALID;

    /* Check timeouts first */
    ai_tp_scheduler_check_timeouts(ctx);

    /* Try to schedule pending tasks */
    int scheduled = 0;
    while (ai_tp_scheduler_schedule_next(ctx) == AI_TP_SCHEDULER_OK) {
        scheduled++;
    }
    return scheduled;
}

/* ============================================================
 * Result Management
 * ============================================================ */

int ai_tp_scheduler_report_result(ai_tp_scheduler_context_t *ctx,
                                   const ai_tp_task_result_t *result) {
    if (!ctx || !result) return AI_TP_SCHEDULER_ERR_INVALID;

    int task_idx = find_task_index(ctx, result->task_id);
    if (task_idx < 0) return AI_TP_SCHEDULER_ERR_NOT_FOUND;

    ai_tp_task_t *task = &ctx->tasks[task_idx];
    if (task->state != AI_TP_TASK_RUNNING && task->state != AI_TP_TASK_SCHEDULED)
        return AI_TP_SCHEDULER_ERR_STATE;

    /* Verify result */
    ai_tp_task_result_t result_copy = *result;
    if (ctx->config.require_verification) {
        bool verified = ai_tp_scheduler_verify_result(ctx, &result_copy);
        result_copy.verification_status = verified ?
            AI_TP_VERIFY_PASSED : AI_TP_VERIFY_FAILED;

        if (!verified) {
            task->state = AI_TP_TASK_FAILED;
            strncpy(task->error_msg, "Result verification failed",
                    AI_TP_SCHEDULER_MAX_ERROR_MSG - 1);
            task->end_time = time(NULL);
            update_node_reliability(ctx, task->assigned_node, false);

            /* Reduce node load */
            int node_idx = find_node_index(ctx, task->assigned_node);
            if (node_idx >= 0 && ctx->nodes[node_idx].current_load > 0)
                ctx->nodes[node_idx].current_load--;

            return AI_TP_SCHEDULER_ERR_VERIFY;
        }
    }

    task->state = AI_TP_TASK_COMPLETED;
    memcpy(task->result_hash, result_copy.output_data_hash,
           AI_TP_SCHEDULER_RESULT_HASH_BYTES);
    task->end_time = time(NULL);

    update_node_reliability(ctx, task->assigned_node, true);

    /* Reduce node load */
    int node_idx = find_node_index(ctx, task->assigned_node);
    if (node_idx >= 0 && ctx->nodes[node_idx].current_load > 0)
        ctx->nodes[node_idx].current_load--;

    return AI_TP_SCHEDULER_OK;
}

bool ai_tp_scheduler_verify_result(ai_tp_scheduler_context_t *ctx,
                                    const ai_tp_task_result_t *result) {
    if (!ctx || !result) return false;

    const ai_tp_task_t *task = ai_tp_scheduler_get_task(ctx, result->task_id);
    if (!task) return false;

    /* Verify compute proof */
    if (result->proof_len > 0) {
        if (!verify_compute_proof(ctx, result, task->assigned_node))
            return false;
    }

    /* Verify output is non-empty */
    if (result->output_size == 0) return false;

    /* Hash check - verify output hash is valid */
    uint8_t zero_hash[AI_TP_SCHEDULER_RESULT_HASH_BYTES];
    memset(zero_hash, 0, AI_TP_SCHEDULER_RESULT_HASH_BYTES);
    if (memcmp(result->output_data_hash, zero_hash,
               AI_TP_SCHEDULER_RESULT_HASH_BYTES) == 0)
        return false;

    return true;
}

/* ============================================================
 * Timeout and Retry
 * ============================================================ */

int ai_tp_scheduler_check_timeouts(ai_tp_scheduler_context_t *ctx) {
    if (!ctx) return AI_TP_SCHEDULER_ERR_INVALID;

    time_t now = time(NULL);
    int timed_out = 0;

    for (uint32_t i = 0; i < ctx->task_count; i++) {
        ai_tp_task_t *task = &ctx->tasks[i];
        if (task->state != AI_TP_TASK_RUNNING && task->state != AI_TP_TASK_SCHEDULED)
            continue;

        if (task->start_time == 0) continue;

        time_t elapsed = (now - task->start_time) * 1000;
        if (elapsed >= (time_t)task->timeout_ms) {
            task->state = AI_TP_TASK_TIMEOUT;
            task->end_time = now;
            strncpy(task->error_msg, "Task timed out",
                    AI_TP_SCHEDULER_MAX_ERROR_MSG - 1);

            /* Update node reliability */
            int node_idx = find_node_index(ctx, task->assigned_node);
            if (node_idx >= 0) {
                ctx->nodes[node_idx].tasks_timed_out++;
                if (ctx->nodes[node_idx].current_load > 0)
                    ctx->nodes[node_idx].current_load--;
                update_node_reliability(ctx, task->assigned_node, false);
            }

            timed_out++;
        }
    }
    return timed_out;
}

int ai_tp_scheduler_retry_task(ai_tp_scheduler_context_t *ctx, uint64_t task_id) {
    if (!ctx) return AI_TP_SCHEDULER_ERR_INVALID;

    int idx = find_task_index(ctx, task_id);
    if (idx < 0) return AI_TP_SCHEDULER_ERR_NOT_FOUND;

    ai_tp_task_t *task = &ctx->tasks[idx];
    if (task->state != AI_TP_TASK_FAILED && task->state != AI_TP_TASK_TIMEOUT)
        return AI_TP_SCHEDULER_ERR_STATE;

    if (task->retry_count >= ctx->config.max_retries)
        return AI_TP_SCHEDULER_ERR_STATE;

    /* Reset task for retry */
    task->state = AI_TP_TASK_QUEUED;
    task->retry_count++;
    memset(task->assigned_node, 0, AI_TP_SCHEDULER_NODE_ID_BYTES);
    memset(task->error_msg, 0, AI_TP_SCHEDULER_MAX_ERROR_MSG);
    task->start_time = 0;
    task->end_time = 0;

    return AI_TP_SCHEDULER_OK;
}

/* ============================================================
 * Configuration
 * ============================================================ */

void ai_tp_scheduler_set_config(ai_tp_scheduler_context_t *ctx,
                                 const ai_tp_scheduler_config_t *config) {
    if (!ctx || !config) return;
    ctx->config = *config;
}

const ai_tp_scheduler_config_t *ai_tp_scheduler_get_config(
    const ai_tp_scheduler_context_t *ctx) {
    if (!ctx) return NULL;
    return &ctx->config;
}

void ai_tp_scheduler_set_strategy(ai_tp_scheduler_context_t *ctx,
                                   ai_tp_schedule_strategy_t strategy) {
    if (!ctx) return;
    ctx->config.strategy = strategy;
}

void ai_tp_scheduler_set_resource_ratio(ai_tp_scheduler_context_t *ctx,
                                         uint8_t ratio) {
    if (!ctx) return;
    if (ratio > 100) ratio = 100;
    ctx->config.resource_ratio = ratio;
}

/* ============================================================
 * Utility
 * ============================================================ */

uint32_t ai_tp_scheduler_get_queue_size(const ai_tp_scheduler_context_t *ctx) {
    if (!ctx) return 0;
    return count_queued_tasks((ai_tp_scheduler_context_t *)ctx);
}

bool ai_tp_scheduler_is_node_available(const ai_tp_scheduler_context_t *ctx,
                                        const uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES]) {
    if (!ctx || !node_id) return false;
    int idx = find_node_index((ai_tp_scheduler_context_t *)ctx, node_id);
    if (idx < 0) return false;
    return ctx->nodes[idx].online;
}

const char *ai_tp_scheduler_task_state_name(ai_tp_task_state_t state) {
    switch (state) {
    case AI_TP_TASK_PENDING:    return "Pending";
    case AI_TP_TASK_QUEUED:     return "Queued";
    case AI_TP_TASK_SCHEDULED:  return "Scheduled";
    case AI_TP_TASK_RUNNING:    return "Running";
    case AI_TP_TASK_COMPLETED:  return "Completed";
    case AI_TP_TASK_FAILED:     return "Failed";
    case AI_TP_TASK_CANCELLED:  return "Cancelled";
    case AI_TP_TASK_TIMEOUT:    return "Timeout";
    default:                    return "Unknown";
    }
}

const char *ai_tp_scheduler_task_type_name(ai_tp_task_type_t type) {
    switch (type) {
    case AI_TP_TASK_TYPE_TRAINING:     return "Training";
    case AI_TP_TASK_TYPE_INFERENCE:    return "Inference";
    case AI_TP_TASK_TYPE_FINE_TUNE:    return "FineTune";
    case AI_TP_TASK_TYPE_DATA_PROCESS: return "DataProcess";
    case AI_TP_TASK_TYPE_CUSTOM:       return "Custom";
    default:                           return "Unknown";
    }
}

const char *ai_tp_scheduler_strategy_name(ai_tp_schedule_strategy_t strategy) {
    switch (strategy) {
    case AI_TP_SCHEDULE_FIFO:          return "FIFO";
    case AI_TP_SCHEDULE_PRIORITY:      return "Priority";
    case AI_TP_SCHEDULE_RESOURCE_FIT:  return "ResourceFit";
    case AI_TP_SCHEDULE_ROUND_ROBIN:   return "RoundRobin";
    default:                           return "Unknown";
    }
}

const char *ai_tp_scheduler_verify_status_name(ai_tp_verification_status_t status) {
    switch (status) {
    case AI_TP_VERIFY_PENDING:  return "Pending";
    case AI_TP_VERIFY_PASSED:   return "Passed";
    case AI_TP_VERIFY_FAILED:   return "Failed";
    case AI_TP_VERIFY_FRAUD:    return "Fraud";
    default:                    return "Unknown";
    }
}

const char *ai_tp_scheduler_get_version(void) {
    return "1.0.0";
}
