/**
 * test_basic.c - Basic tests for libaitp-common
 */

#include <stdio.h>
#include <string.h>
#include "libaitp-common.h"

static int passed = 0, failed = 0;
#define TEST(n) printf("  Testing: %s ... ", n);
#define PASS() do { printf("PASS\n"); passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); failed++; } while(0)
#define ASSERT(c,m) do { if(!(c)){FAIL(m);return;} } while(0)

static void test_hash_deterministic(void) {
    TEST("hash is deterministic");
    uint8_t data[] = "hello world";
    uint8_t a[32], b[32];
    aitp_stub_hash(data, 11, a, 32);
    aitp_stub_hash(data, 11, b, 32);
    ASSERT(memcmp(a, b, 32) == 0, "not deterministic");
    PASS();
}

static void test_hash_different_input(void) {
    TEST("different input produces different output");
    uint8_t data1[] = "hello";
    uint8_t data2[] = "world";
    uint8_t a[32], b[32];
    aitp_stub_hash(data1, 5, a, 32);
    aitp_stub_hash(data2, 5, b, 32);
    ASSERT(memcmp(a, b, 32) != 0, "should differ");
    PASS();
}

static void test_hash_variable_output_len(void) {
    TEST("hash with different output lengths");
    uint8_t data[] = "test data";
    uint8_t out16[16], out32[32], out64[64];
    aitp_stub_hash(data, 9, out16, 16);
    aitp_stub_hash(data, 9, out32, 32);
    aitp_stub_hash(data, 9, out64, 64);
    /* All should be non-zero (or at least not trivially same) */
    int z16 = 1, z32 = 1, z64 = 1;
    for (int i = 0; i < 16; i++) if (out16[i]) z16 = 0;
    for (int i = 0; i < 32; i++) if (out32[i]) z32 = 0;
    for (int i = 0; i < 64; i++) if (out64[i]) z64 = 0;
    ASSERT(z16 == 0, "16-byte hash all zero");
    ASSERT(z32 == 0, "32-byte hash all zero");
    ASSERT(z64 == 0, "64-byte hash all zero");
    PASS();
}

static void test_const_memcmp_equal(void) {
    TEST("const_memcmp equal buffers");
    uint8_t a[] = {1, 2, 3, 4, 5};
    uint8_t b[] = {1, 2, 3, 4, 5};
    ASSERT(aitp_const_memcmp(a, b, 5) == 0, "should be equal");
    PASS();
}

static void test_const_memcmp_diff(void) {
    TEST("const_memcmp different buffers");
    uint8_t a[] = {1, 2, 3, 4, 5};
    uint8_t b[] = {1, 2, 3, 0, 5};
    ASSERT(aitp_const_memcmp(a, b, 5) != 0, "should differ");
    PASS();
}

static void test_const_memcmp_null(void) {
    TEST("const_memcmp null pointers");
    uint8_t buf[] = {1, 2, 3};
    ASSERT(aitp_const_memcmp(NULL, buf, 3) != 0, "null a");
    ASSERT(aitp_const_memcmp(buf, NULL, 3) != 0, "null b");
    PASS();
}

static void test_secure_zero(void) {
    TEST("secure_zero clears buffer");
    uint8_t buf[32];
    memset(buf, 0xFF, 32);
    aitp_secure_zero(buf, 32);
    for (int i = 0; i < 32; i++) {
        ASSERT(buf[i] == 0, "not zeroed");
    }
    PASS();
}

static void test_secure_zero_null(void) {
    TEST("secure_zero null ptr no crash");
    aitp_secure_zero(NULL, 10);
    aitp_secure_zero(NULL, 0);
    PASS();
}

int main(void) {
    printf("=== libaitp-common Basic Tests ===\n\n");
    test_hash_deterministic();
    test_hash_different_input();
    test_hash_variable_output_len();
    test_const_memcmp_equal();
    test_const_memcmp_diff();
    test_const_memcmp_null();
    test_secure_zero();
    test_secure_zero_null();
    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
