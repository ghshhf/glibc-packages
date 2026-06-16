/**
 * test_basic.c - Basic tests for ai-tp-address module
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ai-tp-address.h"

static int passed = 0, failed = 0;
#define TEST(n) printf("  Testing: %s ... ", n);
#define PASS() do { printf("PASS\n"); passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); failed++; } while(0)
#define ASSERT(c,m) do { if(!(c)){FAIL(m);return;} } while(0)

static void test_init_destroy(void) {
    TEST("init and destroy");
    aitp_address_ctx_t ctx;
    int r = aitp_address_init(&ctx);
    ASSERT(r == 0, "init failed");
    ASSERT(ctx.initialized == 1, "not initialized");
    ASSERT(ctx.config.cache_enabled == 1, "cache not enabled");
    ASSERT(ctx.config.dht_k == AITP_DHT_K, "wrong dht_k");
    aitp_address_destroy(&ctx);
    ASSERT(ctx.initialized == 0, "not destroyed");
    PASS();
}

static void test_generate_address(void) {
    TEST("generate address from public key");
    uint8_t pk[32]; for(int i=0;i<32;i++) pk[i]=(uint8_t)(i*7+3);
    aitp_address_t a;
    int r = aitp_generate_address(pk, &a);
    ASSERT(r == 0, "generate failed");
    ASSERT(a.type == AITP_ADDR_RAW, "wrong type");
    ASSERT(strncmp(a.display_str, "at1", 3) == 0, "wrong prefix");
    ASSERT(strlen(a.display_str) >= 10, "too short");
    ASSERT(memcmp(a.owner_pubkey, pk, 32) == 0, "pk mismatch");
    PASS();
}

static void test_validate_address(void) {
    TEST("validate address format");
    uint8_t pk[32]; memset(pk, 0xAB, 32);
    aitp_address_t raw; aitp_generate_address(pk, &raw);
    ASSERT(aitp_validate_address_format(&raw) == 1, "raw invalid");
    ASSERT(aitp_validate_address_string("@alice.at") == 1, "@alice.at invalid");
    ASSERT(aitp_validate_address_string("svc:compute.at") == 1, "svc invalid");
    ASSERT(aitp_validate_address_string("invalid") == 0, "should fail");
    PASS();
}

static void test_string_conversion(void) {
    TEST("address string conversion");
    uint8_t pk[32]; for(int i=0;i<32;i++) pk[i]=(uint8_t)(i*11+5);
    aitp_address_t a1; aitp_generate_address(pk, &a1);
    char buf[128]; aitp_address_to_string(&a1, buf, sizeof(buf));
    ASSERT(strcmp(buf, a1.display_str) == 0, "mismatch");
    aitp_address_t a2;
    ASSERT(aitp_string_to_address(buf, &a2) == 0, "parse failed");
    ASSERT(a2.type == AITP_ADDR_RAW, "wrong type");
    PASS();
}

static void test_name_registry(void) {
    TEST("name registration and resolution");
    aitp_address_ctx_t ctx; aitp_address_init(&ctx);
    uint8_t pk[32]; memset(pk, 0xCD, 32);
    aitp_address_t addr; aitp_generate_address(pk, &addr);
    /* sign for registration */
    uint8_t msg[256];
    size_t ml = (size_t)snprintf((char*)msg, sizeof(msg), "@alice.at:%s", addr.display_str);
    uint8_t sig[64]; stub_sign_internal(msg, ml, pk, sig);
    int r = aitp_register_name(&ctx, "@alice.at", &addr, pk, sig);
    ASSERT(r == AITP_ADDR_OK, "register failed");
    /* resolve */
    aitp_address_t resolved;
    r = aitp_resolve_name(&ctx, "@alice.at", &resolved);
    ASSERT(r == AITP_ADDR_OK, "resolve failed");
    /* invalid name */
    r = aitp_register_name(&ctx, "alice", &addr, pk, sig);
    ASSERT(r == AITP_ADDR_ERR_INVALID, "should reject no-@");
    aitp_address_destroy(&ctx);
    PASS();
}

/* Forward declaration for sign helper used by test_name_registry */
static void stub_sign_internal(const uint8_t *msg, size_t len,
                               const uint8_t pk[32], uint8_t sig[64]);

/* Internal sign helper for testing - must match src/ai-tp-address.c stub_hash + stub_sign */
static void stub_hash_test(const uint8_t *d, size_t n, uint8_t out[32]) {
    memset(out, 0, 32);
    for (size_t i = 0; i < n; i++) {
        out[i % 32] ^= d[i];
        out[(i+7) % 32] = (uint8_t)(out[(i+7) % 32] + d[i] * 31 + i);
        out[(i+13) % 32] ^= (uint8_t)(d[i] << (i % 4));
    }
    for (int r = 0; r < 8; r++)
        for (int i = 0; i < 32; i++)
            out[i] = (uint8_t)(out[i] * 17 + out[(i+3) % 32] + r);
}

static void stub_sign_internal(const uint8_t *msg, size_t len,
                               const uint8_t pk[32], uint8_t sig[64]) {
    uint8_t buf[4096];
    size_t cl = len < 4064 ? len : 4064;
    memcpy(buf, msg, cl);
    memcpy(buf + cl, pk, 32);
    stub_hash_test(buf, cl + 32, sig);
    stub_hash_test(sig, 32, sig + 32);
}

static void test_dht_operations(void) {
    TEST("DHT add/remove/find node");
    aitp_address_ctx_t ctx; aitp_address_init(&ctx);
    for (int i = 0; i < 10; i++) {
        aitp_dht_node_t n; memset(&n, 0, sizeof(n));
        n.node_id[0] = (uint8_t)i; n.node_id[1] = (uint8_t)(i*2);
        snprintf(n.ip, sizeof(n.ip), "192.168.%d.%d", i, i+1);
        n.port = (uint16_t)(9000+i); n.is_alive = 1;
        aitp_dht_add_node(&ctx, &n);
    }
    int total, alive;
    aitp_dht_get_stats(&ctx, &total, &alive);
    ASSERT(total > 0, "should have nodes");
    ASSERT(alive > 0, "should have alive");
    uint8_t tgt[32]; memset(tgt, 0, 32); tgt[0] = 5;
    aitp_dht_node_t found[5];
    int n = aitp_dht_find_node(&ctx, tgt, found, 5);
    ASSERT(n > 0, "should find nodes");
    aitp_address_destroy(&ctx);
    PASS();
}

static void test_cache_operations(void) {
    TEST("cache store/lookup/invalidate");
    aitp_address_ctx_t ctx; aitp_address_init(&ctx);
    uint8_t pk[32]; memset(pk, 0xEF, 32);
    aitp_address_t addr; aitp_generate_address(pk, &addr);
    aitp_dht_node_t node; memset(&node, 0, sizeof(node));
    strncpy(node.ip, "10.0.0.1", sizeof(node.ip));
    node.port = 8080; node.is_alive = 1;
    ASSERT(aitp_cache_store(&ctx, &addr, &node, 60000) == 0, "store failed");
    aitp_cache_entry_t e;
    ASSERT(aitp_cache_lookup(&ctx, &addr, &e) == 0, "lookup failed");
    ASSERT(strcmp(e.resolved_node.ip, "10.0.0.1") == 0, "ip mismatch");
    ASSERT(aitp_cache_invalidate(&ctx, &addr) == 0, "invalidate failed");
    ASSERT(aitp_cache_lookup(&ctx, &addr, &e) != 0, "should be gone");
    aitp_cache_store(&ctx, &addr, &node, 60000);
    ASSERT(aitp_cache_clear(&ctx) == 0, "clear failed");
    aitp_address_destroy(&ctx);
    PASS();
}

static void test_resolve_address(void) {
    TEST("resolve address via DHT");
    aitp_address_ctx_t ctx; aitp_address_init(&ctx);
    uint8_t pk[32]; for(int i=0;i<32;i++) pk[i]=(uint8_t)(i*3+1);
    aitp_address_t addr; aitp_generate_address(pk, &addr);
    aitp_dht_node_t node; memset(&node, 0, sizeof(node));
    memcpy(node.node_id, addr.raw_bytes, 32);
    strncpy(node.ip, "172.16.0.42", sizeof(node.ip));
    node.port = 7777; node.is_alive = 1;
    aitp_dht_add_node(&ctx, &node);
    aitp_resolve_result_t res;
    int r = aitp_resolve_address(&ctx, &addr, &res);
    ASSERT(r == AITP_RESOLVE_DHT_HIT || r == AITP_RESOLVE_CACHED, "should resolve");
    ASSERT(res.is_verified == 1, "should verify");
    aitp_address_destroy(&ctx);
    PASS();
}

static void test_ownership(void) {
    TEST("ownership verification");
    uint8_t pk[32]; for(int i=0;i<32;i++) pk[i]=(uint8_t)(i+100);
    aitp_address_t addr; aitp_generate_address(pk, &addr);
    uint8_t sig[64]; memset(sig, 0, 64);
    int r = aitp_verify_address_ownership(&addr, pk, sig);
    (void)r;
    uint8_t wpk[32]; memset(wpk, 0xFF, 32);
    ASSERT(aitp_verify_address_ownership(&addr, wpk, sig) == 0, "wrong pk");
    PASS();
}

static void test_dns_bridge(void) {
    TEST("DNS bridge lookup");
    aitp_address_ctx_t ctx; aitp_address_init(&ctx);
    char ip[AITP_IP_LEN];
    ASSERT(aitp_bridge_dns_lookup(&ctx, "example.at", ip, sizeof(ip)) == 0, ".at failed");
    ASSERT(strncmp(ip, "10.", 3) == 0, "should be 10.x");
    ASSERT(aitp_bridge_dns_lookup(&ctx, "example.com", ip, sizeof(ip)) != 0, "non-.at fail");
    aitp_address_destroy(&ctx);
    PASS();
}

static void test_config(void) {
    TEST("get/set configuration");
    aitp_address_ctx_t ctx; aitp_address_init(&ctx);
    aitp_resolver_config_t cfg;
    ASSERT(aitp_get_config(&ctx, &cfg) == 0, "get failed");
    ASSERT(cfg.cache_enabled == 1, "cache on");
    cfg.cache_enabled = 0; cfg.cache_ttl_ms = 60000; cfg.max_hops = 4;
    ASSERT(aitp_set_config(&ctx, &cfg) == 0, "set failed");
    aitp_resolver_config_t c2; aitp_get_config(&ctx, &c2);
    ASSERT(c2.cache_enabled == 0, "cache off");
    ASSERT(c2.cache_ttl_ms == 60000, "ttl updated");
    ASSERT(c2.max_hops == 4, "hops updated");
    aitp_address_destroy(&ctx);
    PASS();
}

static void test_utilities(void) {
    TEST("utility functions");
    ASSERT(strcmp(aitp_addr_type_name(AITP_ADDR_RAW), "RAW") == 0, "RAW");
    ASSERT(strcmp(aitp_addr_type_name(AITP_ADDR_NAMED), "NAMED") == 0, "NAMED");
    ASSERT(strcmp(aitp_addr_error_name(AITP_ADDR_ERR_EXISTS), "EXISTS") == 0, "EXISTS");
    uint8_t a[32], b[32], d[32];
    memset(a, 0xFF, 32); memset(b, 0x00, 32);
    aitp_xor_distance(a, b, d);
    for (int i = 0; i < 32; i++) ASSERT(d[i] == 0xFF, "xor wrong");
    PASS();
}

int main(void) {
    printf("=== ai-tp-address Basic Tests ===\n\n");
    test_init_destroy();
    test_generate_address();
    test_validate_address();
    test_string_conversion();
    test_name_registry();
    test_dht_operations();
    test_cache_operations();
    test_resolve_address();
    test_ownership();
    test_dns_bridge();
    test_config();
    test_utilities();
    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
