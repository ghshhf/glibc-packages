/**
 * test_basic.c - Basic tests for ai-tp-worker
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ai-tp-worker.h"

static int passed = 0, failed = 0;
#define TEST(n) printf("  Testing: %s ... ", n);
#define PASS() do { printf("PASS\n"); passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); failed++; } while(0)
#define ASSERT(c,m) do { if(!(c)){FAIL(m);return;} } while(0)

static void test_init_destroy(void) {
    TEST("init and destroy");
    uint8_t nid[32]; memset(nid, 0xAB, 32);
    aitp_worker_ctx_t ctx;
    int r = aitp_worker_init(&ctx, nid);
    ASSERT(r == 0, "init failed");
    ASSERT(ctx.initialized == true, "not init");
    ASSERT(ctx.resources.quota_pct == 20, "wrong default quota");
    ASSERT(memcmp(ctx.node_id, nid, 32) == 0, "node id mismatch");
    aitp_worker_destroy(&ctx);
    ASSERT(ctx.initialized == false, "not destroyed");
    PASS();
}

static void test_set_quota(void) {
    TEST("set and get quota");
    uint8_t nid[32]; memset(nid, 0, 32);
    aitp_worker_ctx_t ctx; aitp_worker_init(&ctx, nid);
    ASSERT(aitp_worker_get_quota(&ctx) == 20, "default 20");
    ASSERT(aitp_worker_set_quota(&ctx, 50) == 0, "set 50");
    ASSERT(aitp_worker_get_quota(&ctx) == 50, "get 50");
    ASSERT(aitp_worker_set_quota(&ctx, 101) == -1, "invalid reject");
    ASSERT(aitp_worker_set_quota(NULL, 50) == -1, "null reject");
    aitp_worker_destroy(&ctx);
    PASS();
}

static void test_submit_local_task(void) {
    TEST("submit local task");
    uint8_t nid[32]; memset(nid, 0xCD, 32);
    aitp_worker_ctx_t ctx; aitp_worker_init(&ctx, nid);
    aitp_worker_task_t t; memset(&t, 0, sizeof(t));
    t.is_local = true;
    t.required_cpu_cores = 2;
    t.required_ram_mb = 512;
    int r = aitp_worker_submit_task(&ctx, &t);
    ASSERT(r == 0, "submit failed");
    ASSERT(ctx.task_count == 1, "task count");
    ASSERT(ctx.tasks[0].state == AITP_WORK_TASK_PENDING, "should be pending");
    ASSERT(strlen(ctx.tasks[0].task_id) > 0, "should have task id");
    aitp_worker_destroy(&ctx);
    PASS();
}

static void test_submit_network_task_quota(void) {
    TEST("submit network task with quota check");
    uint8_t nid[32]; memset(nid, 0xEF, 32);
    aitp_worker_ctx_t ctx; aitp_worker_init(&ctx, nid);
    /* With 4 cores, 20% = 0.8 cores. Request 1 core -> should exceed quota */
    ctx.resources.cpu_cores_total = 4;
    ctx.resources.ram_total_mb = 4096;
    ctx.resources.disk_total_mb = 64000;

    aitp_worker_task_t t; memset(&t, 0, sizeof(t));
    t.is_network = true;
    t.required_cpu_cores = 4;  /* exceeds 20% (0.8 cores) */
    t.required_ram_mb = 512;
    int r = aitp_worker_submit_task(&ctx, &t);
    ASSERT(r == -3, "should exceed quota");
    ASSERT(ctx.task_count == 0, "task not added");
    aitp_worker_destroy(&ctx);
    PASS();
}

static void test_submit_network_within_quota(void) {
    TEST("submit network task within quota");
    uint8_t nid[32]; memset(nid, 0, 32);
    aitp_worker_ctx_t ctx; aitp_worker_init(&ctx, nid);
    ctx.resources.cpu_cores_total = 8;
    ctx.resources.ram_total_mb = 16384;
    ctx.resources.disk_total_mb = 256000;
    ctx.resources.battery_pct = 80;

    aitp_worker_task_t t; memset(&t, 0, sizeof(t));
    t.is_network = true;
    t.required_cpu_cores = 1;  /* 8 * 20% = 1.6, request 1 < 1.6 */
    t.required_ram_mb = 512;
    int r = aitp_worker_submit_task(&ctx, &t);
    ASSERT(r == 0, "submit ok");
    ASSERT(ctx.task_count == 1, "task added");
    ASSERT(ctx.tasks[0].state == AITP_WORK_TASK_QUEUED, "network task queued");
    aitp_worker_destroy(&ctx);
    PASS();
}

static void test_task_execute_complete(void) {
    TEST("execute and complete task");
    uint8_t nid[32]; memset(nid, 0x12, 32);
    aitp_worker_ctx_t ctx; aitp_worker_init(&ctx, nid);
    aitp_worker_task_t t; memset(&t, 0, sizeof(t));
    t.is_local = true;
    aitp_worker_submit_task(&ctx, &t);
    ASSERT(aitp_worker_execute_task(&ctx, ctx.tasks[0].task_id) == 0, "exec");
    ASSERT(ctx.tasks[0].state == AITP_WORK_TASK_RUNNING, "running");
    uint8_t result[] = "test output";
    ASSERT(aitp_worker_complete_task(&ctx, ctx.tasks[0].task_id, result, 11) == 0, "complete");
    ASSERT(ctx.tasks[0].state == AITP_WORK_TASK_COMPLETED, "completed");
    ASSERT(ctx.tasks[0].result_len == 11, "result len");
    aitp_worker_destroy(&ctx);
    PASS();
}

static void test_task_fail(void) {
    TEST("fail task");
    uint8_t nid[32]; memset(nid, 0x34, 32);
    aitp_worker_ctx_t ctx; aitp_worker_init(&ctx, nid);
    aitp_worker_task_t t; memset(&t, 0, sizeof(t));
    t.is_local = true;
    aitp_worker_submit_task(&ctx, &t);
    ASSERT(aitp_worker_fail_task(&ctx, ctx.tasks[0].task_id, "test error") == 0, "fail");
    ASSERT(ctx.tasks[0].state == AITP_WORK_TASK_FAILED, "failed");
    ASSERT(strcmp(ctx.tasks[0].error_msg, "test error") == 0, "error msg");
    aitp_worker_destroy(&ctx);
    PASS();
}

static void test_get_tasks(void) {
    TEST("get tasks list");
    uint8_t nid[32]; memset(nid, 0x56, 32);
    aitp_worker_ctx_t ctx; aitp_worker_init(&ctx, nid);
    aitp_worker_task_t tasks[5];
    ASSERT(aitp_worker_get_tasks(&ctx, tasks, 5) == 0, "empty list");
    aitp_worker_task_t t1, t2; memset(&t1, 0, sizeof(t1)); memset(&t2, 0, sizeof(t2));
    aitp_worker_submit_task(&ctx, &t1);
    aitp_worker_submit_task(&ctx, &t2);
    ASSERT(aitp_worker_get_tasks(&ctx, tasks, 5) == 2, "2 tasks");
    ASSERT(aitp_worker_get_active_count(&ctx) == 0, "none active yet");
    aitp_worker_destroy(&ctx);
    PASS();
}

static void test_state_names(void) {
    TEST("task state names");
    ASSERT(strcmp(aitp_worker_state_name(AITP_WORK_TASK_PENDING), "PENDING") == 0, "");
    ASSERT(strcmp(aitp_worker_state_name(AITP_WORK_TASK_RUNNING), "RUNNING") == 0, "");
    ASSERT(strcmp(aitp_worker_state_name(AITP_WORK_TASK_COMPLETED), "COMPLETED") == 0, "");
    ASSERT(strcmp(aitp_worker_state_name(AITP_WORK_TASK_QUEUED), "QUEUED") == 0, "");
    PASS();
}

static void test_battery_low_rejects(void) {
    TEST("low battery rejects network tasks");
    uint8_t nid[32]; memset(nid, 0x78, 32);
    aitp_worker_ctx_t ctx; aitp_worker_init(&ctx, nid);
    ctx.resources.cpu_cores_total = 8;
    ctx.resources.ram_total_mb = 16384;
    ctx.resources.battery_pct = 15;  /* below 20% */
    ctx.resources.is_charging = false;

    ASSERT(aitp_worker_can_accept_task(&ctx, 1, 512, 0) == false, "low batt reject");
    ctx.resources.is_charging = true;
    ASSERT(aitp_worker_can_accept_task(&ctx, 1, 512, 0) == true, "charging accept");
    aitp_worker_destroy(&ctx);
    PASS();
}

int main(void) {
    printf("=== ai-tp-worker Basic Tests ===\n\n");
    test_init_destroy();
    test_set_quota();
    test_submit_local_task();
    test_submit_network_task_quota();
    test_submit_network_within_quota();
    test_task_execute_complete();
    test_task_fail();
    test_get_tasks();
    test_state_names();
    test_battery_low_rejects();
    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
