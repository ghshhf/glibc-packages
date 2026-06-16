/**
 * @file test_basic.c
 * @brief ai-tp-gateway basic tests
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../include/ai-tp-gateway.h"

static int passed = 0, total = 0;

#define TEST(name) do { total++; printf("  TEST %d: %s ... ", total, name); } while(0)
#define PASS() do { passed++; printf("PASS
"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s
", msg); } while(0)

static void test_init_destroy(void) {
    TEST("init and destroy");
    ai_tp_gw_context_t ctx;
    if (ai_tp_gw_init(&ctx, AI_TP_GW_MODE_DUAL) != AI_TP_GW_OK) { FAIL("init"); return; }
    if (ctx.mode != AI_TP_GW_MODE_DUAL) { FAIL("mode"); return; }
    if (ctx.resource_ratio != AI_TP_GW_DEFAULT_RESOURCE_RATIO) { FAIL("ratio"); return; }
    ai_tp_gw_destroy(&ctx);
    PASS();
}

static void test_start_stop(void) {
    TEST("start and stop");
    ai_tp_gw_context_t ctx;
    ai_tp_gw_init(&ctx, AI_TP_GW_MODE_DUAL);
    if (ai_tp_gw_start(&ctx) != AI_TP_GW_OK) { FAIL("start"); return; }
    if (!ctx.running) { FAIL("running"); return; }
    ai_tp_gw_stop(&ctx);
    if (ctx.running) { FAIL("stopped"); return; }
    ai_tp_gw_destroy(&ctx);
    PASS();
}

static void test_route(void) {
    TEST("route add/find/remove");
    ai_tp_gw_context_t ctx;
    ai_tp_gw_init(&ctx, AI_TP_GW_MODE_DUAL);
    ai_tp_gw_start(&ctx);

    ai_tp_gw_route_t route = {0};
    strncpy(route.host, "192.168.1.100", AI_TP_GW_MAX_HOST - 1);
    strncpy(route.port, "8080", AI_TP_GW_MAX_PORT - 1);
    route.proto = AI_TP_GW_PROTO_TCP;
    strncpy(route.target_node, "ai-tp:QmTest001", AI_TP_GW_MAX_HOST - 1);
    strncpy(route.target_port, "9001", AI_TP_GW_MAX_PORT - 1);
    route.enabled = true;

    if (ai_tp_gw_add_route(&ctx, &route) != AI_TP_GW_OK) { FAIL("add"); return; }
    if (ctx.route_count != 1) { FAIL("count"); return; }
    if (ai_tp_gw_add_route(&ctx, &route) != AI_TP_GW_ERR_EXISTS) { FAIL("dup"); return; }

    const ai_tp_gw_route_t *found = ai_tp_gw_find_route(&ctx, "192.168.1.100", "8080", AI_TP_GW_PROTO_TCP);
    if (!found) { FAIL("find"); return; }
    if (strcmp(found->target_node, "ai-tp:QmTest001") != 0) { FAIL("target"); return; }

    if (ai_tp_gw_remove_route(&ctx, "192.168.1.100", "8080", AI_TP_GW_PROTO_TCP) != AI_TP_GW_OK) { FAIL("remove"); return; }
    if (ctx.route_count != 0) { FAIL("count2"); return; }

    ai_tp_gw_destroy(&ctx);
    PASS();
}

static void test_connect(void) {
    TEST("connect with handshake, forward, disconnect");
    ai_tp_gw_context_t ctx;
    ai_tp_gw_init(&ctx, AI_TP_GW_MODE_DUAL);
    ai_tp_gw_start(&ctx);

    uint64_t cid = ai_tp_gw_connect(&ctx, "10.0.0.1", "12345", "ai-tp:QmRemote", "8080", AI_TP_GW_MODE_OUTBOUND, "A7X9k2mP");
    if (cid == 0) { FAIL("connect"); return; }
    if (ctx.conn_count != 1) { FAIL("cnt"); return; }

    const ai_tp_gw_connection_t *conn = ai_tp_gw_get_connection(&ctx, cid);
    if (!conn) { FAIL("get"); return; }
    if (conn->state != AI_TP_GW_CONN_ACTIVE) { FAIL("active"); return; }
    if (strcmp(conn->handshake_code, "A7X9k2mP") != 0) { FAIL("handshake"); return; }

    const char *data = "Hello AI-TP";
    if (ai_tp_gw_forward(&ctx, cid, data, strlen(data), AI_TP_GW_MODE_OUTBOUND) != (int64_t)strlen(data)) { FAIL("fwd"); return; }

    if (ai_tp_gw_disconnect(&ctx, cid) != AI_TP_GW_OK) { FAIL("disc"); return; }
    ai_tp_gw_destroy(&ctx);
    PASS();
}

static void test_acl(void) {
    TEST("ACL allow/deny");
    ai_tp_gw_context_t ctx;
    ai_tp_gw_init(&ctx, AI_TP_GW_MODE_DUAL);

    if (!ai_tp_gw_check_acl(&ctx, "any", "any", AI_TP_GW_PROTO_TCP)) { FAIL("default"); return; }

    ai_tp_gw_acl_rule_t deny = {0};
    deny.action = AI_TP_GW_ACL_DENY;
    strncpy(deny.src_pattern, "10.0.0.*", AI_TP_GW_MAX_DOMAIN - 1);
    strncpy(deny.dst_pattern, "*", AI_TP_GW_MAX_DOMAIN - 1);
    deny.proto = AI_TP_GW_PROTO_ATP;
    deny.priority = 10;
    deny.enabled = true;
    ai_tp_gw_add_acl(&ctx, &deny);

    if (ai_tp_gw_check_acl(&ctx, "10.0.0.5", "any", AI_TP_GW_PROTO_TCP)) { FAIL("deny"); return; }
    if (!ai_tp_gw_check_acl(&ctx, "192.168.1.1", "any", AI_TP_GW_PROTO_TCP)) { FAIL("allow"); return; }

    ai_tp_gw_acl_rule_t allow = {0};
    allow.action = AI_TP_GW_ACL_ALLOW;
    strncpy(allow.src_pattern, "10.0.0.1", AI_TP_GW_MAX_DOMAIN - 1);
    strncpy(allow.dst_pattern, "*", AI_TP_GW_MAX_DOMAIN - 1);
    allow.proto = AI_TP_GW_PROTO_ATP;
    allow.priority = 5;
    allow.enabled = true;
    ai_tp_gw_add_acl(&ctx, &allow);

    if (!ai_tp_gw_check_acl(&ctx, "10.0.0.1", "any", AI_TP_GW_PROTO_TCP)) { FAIL("override"); return; }
    ai_tp_gw_destroy(&ctx);
    PASS();
}

static void test_config(void) {
    TEST("resource ratio and bandwidth");
    ai_tp_gw_context_t ctx;
    ai_tp_gw_init(&ctx, AI_TP_GW_MODE_DUAL);
    if (ctx.resource_ratio != 20) { FAIL("def"); return; }
    ai_tp_gw_set_resource_ratio(&ctx, 50);
    if (ctx.resource_ratio != 50) { FAIL("50"); return; }
    ai_tp_gw_set_resource_ratio(&ctx, 200);
    if (ctx.resource_ratio != 100) { FAIL("clamp"); return; }
    ai_tp_gw_set_bandwidth_limit(&ctx, 1024);
    if (ctx.bandwidth_limit != 1024) { FAIL("bw"); return; }
    ai_tp_gw_destroy(&ctx);
    PASS();
}

static void test_stats(void) {
    TEST("traffic statistics");
    ai_tp_gw_context_t ctx;
    ai_tp_gw_init(&ctx, AI_TP_GW_MODE_DUAL);
    ai_tp_gw_start(&ctx);

    uint64_t cid = ai_tp_gw_connect(&ctx, "192.168.1.1", "80", "ai-tp:QmN1", "9000", AI_TP_GW_MODE_INBOUND, "B3Z8n5qR");
    const char *data = "test payload";
    ai_tp_gw_forward(&ctx, cid, data, strlen(data), AI_TP_GW_MODE_INBOUND);
    ai_tp_gw_forward(&ctx, cid, data, strlen(data), AI_TP_GW_MODE_OUTBOUND);

    const ai_tp_gw_stats_t *s = ai_tp_gw_get_stats(&ctx);
    if (!s) { FAIL("null"); return; }
    if (s->total_bytes_in != strlen(data)) { FAIL("in"); return; }
    if (s->total_bytes_out != strlen(data)) { FAIL("out"); return; }
    if (s->total_connections != 1) { FAIL("conn"); return; }
    ai_tp_gw_destroy(&ctx);
    PASS();
}

static void test_misc(void) {
    TEST("proto name and version");
    if (strcmp(ai_tp_gw_proto_name(AI_TP_GW_PROTO_TCP), "TCP") != 0) { FAIL("tcp"); return; }
    if (strcmp(ai_tp_gw_proto_name(AI_TP_GW_PROTO_ATP), "ATP") != 0) { FAIL("atp"); return; }
    if (strcmp(ai_tp_gw_get_version(), "1.0.0") != 0) { FAIL("ver"); return; }
    PASS();
}

int main(void) {
    printf("=== ai-tp-gateway tests ===

");
    test_init_destroy();
    test_start_stop();
    test_route();
    test_connect();
    test_acl();
    test_config();
    test_stats();
    test_misc();
    printf("
=== Results: %d/%d passed ===
", passed, total);
    return (passed == total) ? 0 : 1;
}

