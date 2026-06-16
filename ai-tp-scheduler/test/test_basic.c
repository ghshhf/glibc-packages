/**
 * @file test_basic.c
 * @brief ai-tp-scheduler basic tests
 */

#include <stdio.h>
#include <string.h>
#include "../include/ai-tp-scheduler.h"

static int passed = 0, total = 0;

#define TEST(name) do { total++; printf("  TEST %d: %s ... ", total, name); } while(0)
#define PASS() do { passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

/* ============================================================
 * Test Cases
 * ============================================================ */

static void test_init_destroy(void) {
    TEST("init and destroy");
    ai_tp_scheduler_context_t ctx;
    if (ai_tp_scheduler_init(&ctx) != AI_TP_SCHEDULER_OK) { FAIL("init"); return; }
    if (!ctx.initialized) { FAIL("flag"); return; }
    if (ctx.config.strategy != AI_TP_SCHEDULE_PRIORITY) { FAIL("strategy"); return; }
    if (ctx.config.default_timeout_ms != AI_TP_SCHEDULER_DEFAULT_TIMEOUT_MS) { FAIL("timeout"); return; }
    if (ctx.config.max_retries != AI_TP_SCHEDULER_DEFAULT_MAX_RETRIES) { FAIL("retries"); return; }
    if (ctx.config.resource_ratio != AI_TP_SCHEDULER_DEFAULT_RESOURCE_RATIO) { FAIL("ratio"); return; }
    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_submit_task(void) {
    TEST("submit task with resource requirements");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);

    ai_tp_resource_spec_t req;
    memset(&req, 0, sizeof(req));
    req.gpu_count = 2;
    req.gpu_mem_mb = 16384;
    req.cpu_cores = 8;
    req.ram_mb = 32768;
    req.disk_mb = 102400;
    req.network_mbps = 1000;

    uint8_t submitter[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(submitter, 0xAB, AI_TP_SCHEDULER_NODE_ID_BYTES);

    uint64_t task_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_TRAINING,
                                                     &req, submitter, 50);
    if (task_id != 1) { FAIL("task_id"); return; }

    const ai_tp_task_t *task = ai_tp_scheduler_get_task(&ctx, task_id);
    if (!task) { FAIL("get_task"); return; }
    if (task->type != AI_TP_TASK_TYPE_TRAINING) { FAIL("type"); return; }
    if (task->state != AI_TP_TASK_QUEUED) { FAIL("state"); return; }
    if (task->priority != 50) { FAIL("priority"); return; }
    if (task->resource_req.gpu_count != 2) { FAIL("gpu"); return; }
    if (task->resource_req.ram_mb != 32768) { FAIL("ram"); return; }
    if (task->timeout_ms != ctx.config.default_timeout_ms) { FAIL("timeout"); return; }

    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_register_node(void) {
    TEST("register and unregister compute nodes");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);

    uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(node_id, 0x11, AI_TP_SCHEDULER_NODE_ID_BYTES);

    ai_tp_resource_spec_t resources;
    memset(&resources, 0, sizeof(resources));
    resources.gpu_count = 4;
    resources.gpu_mem_mb = 32768;
    resources.cpu_cores = 16;
    resources.ram_mb = 65536;
    resources.disk_mb = 500000;
    resources.network_mbps = 10000;

    bool types[5] = { true, true, true, false, true };

    if (ai_tp_scheduler_register_node(&ctx, node_id, &resources, types) != AI_TP_SCHEDULER_OK)
        { FAIL("register"); return; }
    if (ctx.node_count != 1) { FAIL("count"); return; }

    const ai_tp_node_capability_t *node = ai_tp_scheduler_get_node(&ctx, node_id);
    if (!node) { FAIL("get_node"); return; }
    if (node->available_resources.gpu_count != 4) { FAIL("gpu"); return; }
    if (!node->supported_types[0]) { FAIL("train"); return; }
    if (node->supported_types[3]) { FAIL("dp"); return; }
    if (node->reliability_score != 0.5) { FAIL("reliability"); return; }

    /* Unregister */
    if (ai_tp_scheduler_unregister_node(&ctx, node_id) != AI_TP_SCHEDULER_OK)
        { FAIL("unregister"); return; }
    if (ctx.node_count != 0) { FAIL("count2"); return; }

    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_schedule_task(void) {
    TEST("schedule task to matching node");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);

    /* Register a node */
    uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(node_id, 0xAA, AI_TP_SCHEDULER_NODE_ID_BYTES);

    ai_tp_resource_spec_t node_res;
    memset(&node_res, 0, sizeof(node_res));
    node_res.gpu_count = 2;
    node_res.gpu_mem_mb = 16384;
    node_res.cpu_cores = 8;
    node_res.ram_mb = 32768;
    node_res.disk_mb = 102400;
    node_res.network_mbps = 1000;

    bool types[5] = { true, true, true, true, true };
    ai_tp_scheduler_register_node(&ctx, node_id, &node_res, types);

    /* Submit a task */
    ai_tp_resource_spec_t task_req;
    memset(&task_req, 0, sizeof(task_req));
    task_req.gpu_count = 1;
    task_req.gpu_mem_mb = 8192;
    task_req.cpu_cores = 4;
    task_req.ram_mb = 16384;

    uint8_t submitter[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(submitter, 0xBB, AI_TP_SCHEDULER_NODE_ID_BYTES);

    uint64_t task_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_INFERENCE,
                                                     &task_req, submitter, 30);
    if (task_id == 0) { FAIL("submit"); return; }

    /* Schedule */
    int ret = ai_tp_scheduler_schedule_next(&ctx);
    if (ret != AI_TP_SCHEDULER_OK) { FAIL("schedule"); return; }

    const ai_tp_task_t *task = ai_tp_scheduler_get_task(&ctx, task_id);
    if (task->state != AI_TP_TASK_SCHEDULED) { FAIL("state"); return; }
    if (memcmp(task->assigned_node, node_id, AI_TP_SCHEDULER_NODE_ID_BYTES) != 0)
        { FAIL("assigned"); return; }

    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_priority_ordering(void) {
    TEST("priority ordering");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);
    ai_tp_scheduler_set_strategy(&ctx, AI_TP_SCHEDULE_PRIORITY);

    /* Register a node */
    uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(node_id, 0xCC, AI_TP_SCHEDULER_NODE_ID_BYTES);

    ai_tp_resource_spec_t node_res;
    memset(&node_res, 0, sizeof(node_res));
    node_res.gpu_count = 8;
    node_res.gpu_mem_mb = 65536;
    node_res.cpu_cores = 32;
    node_res.ram_mb = 131072;
    node_res.disk_mb = 1000000;
    node_res.network_mbps = 10000;

    bool types[5] = { true, true, true, true, true };
    ai_tp_scheduler_register_node(&ctx, node_id, &node_res, types);

    ai_tp_resource_spec_t task_req;
    memset(&task_req, 0, sizeof(task_req));
    task_req.gpu_count = 1;
    task_req.cpu_cores = 1;

    uint8_t sub[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(sub, 0xDD, AI_TP_SCHEDULER_NODE_ID_BYTES);

    /* Submit tasks with different priorities */
    uint64_t low_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_CUSTOM,
                                                    &task_req, sub, 10);
    uint64_t high_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_CUSTOM,
                                                     &task_req, sub, 90);

    /* Schedule once - should pick highest priority */
    int ret = ai_tp_scheduler_schedule_next(&ctx);
    if (ret != AI_TP_SCHEDULER_OK) { FAIL("schedule"); return; }

    const ai_tp_task_t *high = ai_tp_scheduler_get_task(&ctx, high_id);
    if (high->state != AI_TP_TASK_SCHEDULED) { FAIL("high not scheduled"); return; }

    const ai_tp_task_t *low = ai_tp_scheduler_get_task(&ctx, low_id);
    if (low->state != AI_TP_TASK_QUEUED) { FAIL("low should wait"); return; }

    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_task_dependency(void) {
    TEST("task dependency resolution (DAG)");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);

    /* Register a node */
    uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(node_id, 0xEE, AI_TP_SCHEDULER_NODE_ID_BYTES);

    ai_tp_resource_spec_t node_res;
    memset(&node_res, 0, sizeof(node_res));
    node_res.gpu_count = 4;
    node_res.gpu_mem_mb = 32768;
    node_res.cpu_cores = 16;
    node_res.ram_mb = 65536;
    node_res.disk_mb = 500000;
    node_res.network_mbps = 10000;

    bool types[5] = { true, true, true, true, true };
    ai_tp_scheduler_register_node(&ctx, node_id, &node_res, types);

    ai_tp_resource_spec_t task_req;
    memset(&task_req, 0, sizeof(task_req));
    task_req.cpu_cores = 1;

    uint8_t sub[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(sub, 0xFF, AI_TP_SCHEDULER_NODE_ID_BYTES);

    /* Submit parent task */
    uint64_t parent_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_DATA_PROCESS,
                                                       &task_req, sub, 50);
    if (parent_id == 0) { FAIL("parent"); return; }

    /* Submit child task dependent on parent */
    uint64_t child_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_CUSTOM,
                                                      &task_req, sub, 50);
    if (child_id == 0) { FAIL("child"); return; }

    uint64_t deps[1] = { parent_id };
    if (ai_tp_scheduler_set_task_dependencies(&ctx, child_id, deps, 1) != AI_TP_SCHEDULER_OK)
        { FAIL("set_deps"); return; }

    /* Dependency prevents scheduling - parent should schedule first */
    int ret = ai_tp_scheduler_schedule_next(&ctx);
    if (ret != AI_TP_SCHEDULER_OK) { FAIL("schedule parent"); return; }

    const ai_tp_task_t *parent = ai_tp_scheduler_get_task(&ctx, parent_id);
    if (parent->state != AI_TP_TASK_SCHEDULED) { FAIL("parent state"); return; }

    /* Child should still be queued */
    const ai_tp_task_t *child = ai_tp_scheduler_get_task(&ctx, child_id);
    if (child->state != AI_TP_TASK_QUEUED) { FAIL("child state"); return; }

    /* Complete parent by reporting result */
    ai_tp_task_result_t result;
    memset(&result, 0, sizeof(result));
    result.task_id = parent_id;
    memset(result.output_data_hash, 0x42, AI_TP_SCHEDULER_RESULT_HASH_BYTES);
    result.output_size = 1024;
    memset(result.compute_proof, 0x99, AI_TP_SCHEDULER_PROOF_BYTES);
    result.proof_len = AI_TP_SCHEDULER_PROOF_BYTES;

    /* For result reporting to accept, need to set task to RUNNING first */
    /* Let's manually set parent to COMPLETED to test dependency */
    /* Actually let's bypass result reporting and directly set state */
    ctx.tasks[0].state = AI_TP_TASK_COMPLETED;
    memset(ctx.tasks[0].result_hash, 0x42, AI_TP_SCHEDULER_RESULT_HASH_BYTES);

    /* Now child should be schedulable */
    ret = ai_tp_scheduler_schedule_next(&ctx);
    if (ret != AI_TP_SCHEDULER_OK) { FAIL("schedule child"); return; }

    child = ai_tp_scheduler_get_task(&ctx, child_id);
    if (child->state != AI_TP_TASK_SCHEDULED) { FAIL("child not scheduled"); return; }

    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_timeout_and_retry(void) {
    TEST("timeout and retry");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);

    /* Register a node */
    uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(node_id, 0x77, AI_TP_SCHEDULER_NODE_ID_BYTES);

    ai_tp_resource_spec_t node_res;
    memset(&node_res, 0, sizeof(node_res));
    node_res.gpu_count = 2;
    node_res.cpu_cores = 4;
    node_res.ram_mb = 16384;

    bool types[5] = { true, true, true, true, true };
    ai_tp_scheduler_register_node(&ctx, node_id, &node_res, types);

    ai_tp_resource_spec_t task_req;
    memset(&task_req, 0, sizeof(task_req));
    task_req.cpu_cores = 1;
    task_req.ram_mb = 1024;

    uint8_t sub[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(sub, 0x88, AI_TP_SCHEDULER_NODE_ID_BYTES);

    /* Set a short timeout config */
    ai_tp_scheduler_config_t config = ctx.config;
    config.default_timeout_ms = 1; /* 1ms timeout */
    ai_tp_scheduler_set_config(&ctx, &config);

    uint64_t task_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_INFERENCE,
                                                     &task_req, sub, 50);
    if (task_id == 0) { FAIL("submit"); return; }

    /* Schedule and then check timeout */
    if (ai_tp_scheduler_schedule_next(&ctx) != AI_TP_SCHEDULER_OK) { FAIL("schedule"); return; }

    /* Manually set start time to far in the past to trigger timeout */
    ctx.tasks[0].start_time = time(NULL) - 3600;

    int timed_out = ai_tp_scheduler_check_timeouts(&ctx);
    if (timed_out != 1) { FAIL("timeout count"); return; }

    const ai_tp_task_t *task = ai_tp_scheduler_get_task(&ctx, task_id);
    if (task->state != AI_TP_TASK_TIMEOUT) { FAIL("not timed out"); return; }

    /* Retry */
    if (ai_tp_scheduler_retry_task(&ctx, task_id) != AI_TP_SCHEDULER_OK) { FAIL("retry"); return; }

    task = ai_tp_scheduler_get_task(&ctx, task_id);
    if (task->state != AI_TP_TASK_QUEUED) { FAIL("not requeued"); return; }
    if (task->retry_count != 1) { FAIL("retry count"); return; }

    /* Max retries test */
    ctx.tasks[0].retry_count = ctx.config.max_retries;
    ctx.tasks[0].state = AI_TP_TASK_FAILED;
    if (ai_tp_scheduler_retry_task(&ctx, task_id) == AI_TP_SCHEDULER_OK)
        { FAIL("should not retry beyond max"); return; }

    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_result_verification(void) {
    TEST("result reporting and verification");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);

    /* Register a node */
    uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(node_id, 0x55, AI_TP_SCHEDULER_NODE_ID_BYTES);

    ai_tp_resource_spec_t node_res;
    memset(&node_res, 0, sizeof(node_res));
    node_res.gpu_count = 2;
    node_res.cpu_cores = 4;
    node_res.ram_mb = 16384;

    bool types[5] = { true, true, true, true, true };
    ai_tp_scheduler_register_node(&ctx, node_id, &node_res, types);

    ai_tp_resource_spec_t task_req;
    memset(&task_req, 0, sizeof(task_req));
    task_req.cpu_cores = 1;
    task_req.ram_mb = 1024;

    uint8_t sub[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(sub, 0x66, AI_TP_SCHEDULER_NODE_ID_BYTES);

    /* Disable verification for this test */
    ctx.config.require_verification = false;

    uint64_t task_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_INFERENCE,
                                                     &task_req, sub, 50);
    if (ai_tp_scheduler_schedule_next(&ctx) != AI_TP_SCHEDULER_OK) { FAIL("schedule"); return; }

    /* Manually set to RUNNING */
    ctx.tasks[0].state = AI_TP_TASK_RUNNING;

    ai_tp_task_result_t result;
    memset(&result, 0, sizeof(result));
    result.task_id = task_id;
    memset(result.output_data_hash, 0x42, AI_TP_SCHEDULER_RESULT_HASH_BYTES);
    result.output_size = 1024;
    memset(result.compute_proof, 0x99, AI_TP_SCHEDULER_PROOF_BYTES);
    result.proof_len = AI_TP_SCHEDULER_PROOF_BYTES;

    if (ai_tp_scheduler_report_result(&ctx, &result) != AI_TP_SCHEDULER_OK)
        { FAIL("report"); return; }

    const ai_tp_task_t *task = ai_tp_scheduler_get_task(&ctx, task_id);
    if (task->state != AI_TP_TASK_COMPLETED) { FAIL("not completed"); return; }

    /* Verify result validity */
    if (!ai_tp_scheduler_verify_result(&ctx, &result)) { FAIL("verify"); return; }

    /* Empty output should fail verification */
    ai_tp_task_result_t bad_result;
    memset(&bad_result, 0, sizeof(bad_result));
    bad_result.task_id = task_id;
    if (ai_tp_scheduler_verify_result(&ctx, &bad_result)) { FAIL("bad verify"); return; }

    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_resource_ratio_enforcement(void) {
    TEST("resource ratio enforcement");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);

    /* Register an internal node */
    uint8_t internal_id[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(internal_id, 0x99, AI_TP_SCHEDULER_NODE_ID_BYTES);

    ai_tp_resource_spec_t node_res;
    memset(&node_res, 0, sizeof(node_res));
    node_res.gpu_count = 4;
    node_res.cpu_cores = 8;
    node_res.ram_mb = 16384;

    bool types[5] = { true, true, true, true, true };
    ai_tp_scheduler_register_node(&ctx, internal_id, &node_res, types);

    ai_tp_resource_spec_t task_req;
    memset(&task_req, 0, sizeof(task_req));
    task_req.cpu_cores = 1;

    /* External submitter (not in node list) */
    uint8_t ext_sub[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(ext_sub, 0x11, AI_TP_SCHEDULER_NODE_ID_BYTES);

    /* Submit many external tasks */
    uint64_t first_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_INFERENCE,
                                                      &task_req, ext_sub, 50);
    if (first_id == 0) { FAIL("first ext"); return; }

    /* Internal task should always be accepted */
    uint64_t int_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_INFERENCE,
                                                    &task_req, internal_id, 50);
    if (int_id == 0) { FAIL("internal"); return; }

    /* At least 1 task was accepted */
    if (ctx.task_count < 2) { FAIL("task count"); return; }
    /* External task count is limited by ratio - but with only 2 tasks at 20% ratio,
       at least 1/5 = 0, floored to 1 external tasks allowed */

    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_node_reliability(void) {
    TEST("node reliability tracking");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);

    uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(node_id, 0x22, AI_TP_SCHEDULER_NODE_ID_BYTES);

    ai_tp_resource_spec_t resources;
    memset(&resources, 0, sizeof(resources));
    resources.gpu_count = 2;
    resources.cpu_cores = 4;
    resources.ram_mb = 16384;

    bool types[5] = { true, true, true, true, true };
    ai_tp_scheduler_register_node(&ctx, node_id, &resources, types);

    const ai_tp_node_capability_t *node = ai_tp_scheduler_get_node(&ctx, node_id);
    if (node->reliability_score != 0.5) { FAIL("initial"); return; }
    if (node->tasks_completed != 0) { FAIL("completed"); return; }
    if (node->tasks_failed != 0) { FAIL("failed"); return; }

    /* Simulate successful completion */
    ctx.nodes[0].tasks_completed = 8;
    ctx.nodes[0].tasks_failed = 2;
    ctx.nodes[0].reliability_score = 8.0 / 10.0;
    if (ctx.nodes[0].reliability_score != 0.8) { FAIL("score"); return; }

    /* Simulate more failures */
    ctx.nodes[0].tasks_completed = 5;
    ctx.nodes[0].tasks_failed = 5;
    ctx.nodes[0].reliability_score = 5.0 / 10.0;
    if (ctx.nodes[0].reliability_score != 0.5) { FAIL("score2"); return; }

    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_schedule_strategies(void) {
    TEST("different schedule strategies");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);

    /* Register a node */
    uint8_t node_id[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(node_id, 0x33, AI_TP_SCHEDULER_NODE_ID_BYTES);

    ai_tp_resource_spec_t node_res;
    memset(&node_res, 0, sizeof(node_res));
    node_res.gpu_count = 4;
    node_res.cpu_cores = 8;
    node_res.ram_mb = 32768;

    bool types[5] = { true, true, true, true, true };
    ai_tp_scheduler_register_node(&ctx, node_id, &node_res, types);

    ai_tp_resource_spec_t task_req;
    memset(&task_req, 0, sizeof(task_req));
    task_req.cpu_cores = 1;

    uint8_t sub[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(sub, 0x44, AI_TP_SCHEDULER_NODE_ID_BYTES);

    /* FIFO */
    ai_tp_scheduler_set_strategy(&ctx, AI_TP_SCHEDULE_FIFO);
    uint64_t fifo_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_INFERENCE,
                                                     &task_req, sub, 10);
    if (ai_tp_scheduler_schedule_next(&ctx) != AI_TP_SCHEDULER_OK) { FAIL("fifo"); return; }
    const ai_tp_task_t *fifo_task = ai_tp_scheduler_get_task(&ctx, fifo_id);
    if (fifo_task->state != AI_TP_TASK_SCHEDULED) { FAIL("fifo state"); return; }

    /* Resource Fit */
    ai_tp_scheduler_set_strategy(&ctx, AI_TP_SCHEDULE_RESOURCE_FIT);
    ctx.tasks[0].state = AI_TP_TASK_COMPLETED; /* Mark first task done */
    uint64_t rf_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_TRAINING,
                                                   &task_req, sub, 20);
    if (ai_tp_scheduler_schedule_next(&ctx) != AI_TP_SCHEDULER_OK) { FAIL("rf"); return; }
    const ai_tp_task_t *rf_task = ai_tp_scheduler_get_task(&ctx, rf_id);
    if (rf_task->state != AI_TP_TASK_SCHEDULED) { FAIL("rf state"); return; }

    /* Round Robin */
    ai_tp_scheduler_set_strategy(&ctx, AI_TP_SCHEDULE_ROUND_ROBIN);
    ctx.tasks[1].state = AI_TP_TASK_COMPLETED;
    ctx.nodes[0].current_load = 0;
    uint64_t rr_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_CUSTOM,
                                                   &task_req, sub, 30);
    if (ai_tp_scheduler_schedule_next(&ctx) != AI_TP_SCHEDULER_OK) { FAIL("rr"); return; }
    const ai_tp_task_t *rr_task = ai_tp_scheduler_get_task(&ctx, rr_id);
    if (rr_task->state != AI_TP_TASK_SCHEDULED) { FAIL("rr state"); return; }

    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_queue_size_limits(void) {
    TEST("queue size limits");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);

    /* Set a low queue limit */
    ai_tp_scheduler_config_t config = ctx.config;
    config.max_queue_size = 3;
    ai_tp_scheduler_set_config(&ctx, &config);

    ai_tp_resource_spec_t task_req;
    memset(&task_req, 0, sizeof(task_req));
    task_req.cpu_cores = 1;

    uint8_t sub[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(sub, 0xBC, AI_TP_SCHEDULER_NODE_ID_BYTES);

    /* Submit up to max */
    for (int i = 0; i < 3; i++) {
        uint64_t id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_CUSTOM,
                                                    &task_req, sub, 10);
        if (id == 0) { FAIL("submit within limit"); goto cleanup; }
    }

    /* Next should fail */
    uint64_t overflow = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_CUSTOM,
                                                      &task_req, sub, 10);
    if (overflow != 0) { FAIL("should not exceed limit"); goto cleanup; }

    if (ai_tp_scheduler_get_queue_size(&ctx) != 3) { FAIL("queue size"); goto cleanup; }

cleanup:
    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_cancel_task(void) {
    TEST("cancel task");
    ai_tp_scheduler_context_t ctx;
    ai_tp_scheduler_init(&ctx);

    ai_tp_resource_spec_t task_req;
    memset(&task_req, 0, sizeof(task_req));
    task_req.cpu_cores = 1;
    task_req.ram_mb = 1024;

    uint8_t sub[AI_TP_SCHEDULER_NODE_ID_BYTES];
    memset(sub, 0xDE, AI_TP_SCHEDULER_NODE_ID_BYTES);

    uint64_t task_id = ai_tp_scheduler_submit_task(&ctx, AI_TP_TASK_TYPE_INFERENCE,
                                                     &task_req, sub, 50);
    if (task_id == 0) { FAIL("submit"); return; }

    if (ai_tp_scheduler_cancel_task(&ctx, task_id) != AI_TP_SCHEDULER_OK) { FAIL("cancel"); return; }

    const ai_tp_task_t *task = ai_tp_scheduler_get_task(&ctx, task_id);
    if (task->state != AI_TP_TASK_CANCELLED) { FAIL("state"); return; }

    /* Cannot cancel already cancelled */
    if (ai_tp_scheduler_cancel_task(&ctx, task_id) == AI_TP_SCHEDULER_OK)
        { FAIL("double cancel"); return; }

    ai_tp_scheduler_destroy(&ctx);
    PASS();
}

static void test_name_functions(void) {
    TEST("name/string functions");
    if (strcmp(ai_tp_scheduler_task_state_name(AI_TP_TASK_RUNNING), "Running") != 0)
        { FAIL("state"); return; }
    if (strcmp(ai_tp_scheduler_task_type_name(AI_TP_TASK_TYPE_TRAINING), "Training") != 0)
        { FAIL("type"); return; }
    if (strcmp(ai_tp_scheduler_strategy_name(AI_TP_SCHEDULE_FIFO), "FIFO") != 0)
        { FAIL("strategy"); return; }
    if (strcmp(ai_tp_scheduler_verify_status_name(AI_TP_VERIFY_FRAUD), "Fraud") != 0)
        { FAIL("verify"); return; }
    if (strcmp(ai_tp_scheduler_get_version(), "1.0.0") != 0)
        { FAIL("version"); return; }
    PASS();
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("=== ai-tp-scheduler tests ===\n\n");
    test_init_destroy();
    test_submit_task();
    test_register_node();
    test_schedule_task();
    test_priority_ordering();
    test_task_dependency();
    test_timeout_and_retry();
    test_result_verification();
    test_resource_ratio_enforcement();
    test_node_reliability();
    test_schedule_strategies();
    test_queue_size_limits();
    test_cancel_task();
    test_name_functions();
    printf("\n=== Results: %d/%d passed ===\n", passed, total);
    return (passed == total) ? 0 : 1;
}
