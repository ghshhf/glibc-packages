/**
 * @file test_basic.c
 * @brief ai-tp-crypto basic tests
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../include/ai-tp-crypto.h"

static int passed = 0, total = 0;

#define TEST(name) do { total++; printf("  TEST %d: %s ... ", total, name); } while(0)
#define PASS() do { passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

static void test_init_shutdown(void) {
    TEST("init and shutdown");
    if (ai_tp_crypto_init() != AI_TP_CRYPTO_OK) { FAIL("init"); return; }
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_random(void) {
    TEST("random bytes");
    ai_tp_crypto_init();
    uint8_t buf1[32], buf2[32];
    if (ai_tp_crypto_random_bytes(buf1, 32) != AI_TP_CRYPTO_OK) { FAIL("rand1"); return; }
    if (ai_tp_crypto_random_bytes(buf2, 32) != AI_TP_CRYPTO_OK) { FAIL("rand2"); return; }
    /* Two random buffers should differ (extremely high probability) */
    if (memcmp(buf1, buf2, 32) == 0) { FAIL("identical"); return; }
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_nonce_salt(void) {
    TEST("nonce and salt generation");
    ai_tp_crypto_init();
    ai_tp_crypto_nonce_t n1, n2;
    ai_tp_crypto_salt_t s1, s2;
    ai_tp_crypto_random_nonce(&n1);
    ai_tp_crypto_random_nonce(&n2);
    ai_tp_crypto_random_salt(&s1);
    ai_tp_crypto_random_salt(&s2);
    if (memcmp(n1.bytes, n2.bytes, AI_TP_CRYPTO_NONCE_BYTES) == 0) { FAIL("nonce dup"); return; }
    if (memcmp(s1.bytes, s2.bytes, AI_TP_CRYPTO_SALT_BYTES) == 0) { FAIL("salt dup"); return; }
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_sign_keypair(void) {
    TEST("sign keypair generation");
    ai_tp_crypto_init();
    ai_tp_crypto_keypair_t kp;
    if (ai_tp_crypto_sign_keypair(&kp) != AI_TP_CRYPTO_OK) { FAIL("keygen"); return; }
    /* Public and secret keys should differ */
    if (memcmp(kp.pub.pk, kp.sec.sk, 32) == 0) { FAIL("pk==sk"); return; }
    /* Deterministic from seed */
    ai_tp_crypto_keypair_t kp2;
    uint8_t seed[32] = {0x42};
    ai_tp_crypto_sign_keypair_from_seed(&kp2, seed);
    ai_tp_crypto_keypair_t kp3;
    ai_tp_crypto_sign_keypair_from_seed(&kp3, seed);
    if (memcmp(kp2.pub.pk, kp3.pub.pk, 32) != 0) { FAIL("det"); return; }
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_kx_keypair(void) {
    TEST("key exchange keypair and conversion");
    ai_tp_crypto_init();
    ai_tp_crypto_keypair_t sign_kp, kx_kp;
    ai_tp_crypto_sign_keypair(&sign_kp);
    if (ai_tp_crypto_kx_keypair_from_sign(&kx_kp, &sign_kp) != AI_TP_CRYPTO_OK) { FAIL("convert"); return; }
    /* KX keys should differ from sign keys */
    if (memcmp(sign_kp.pub.pk, kx_kp.pub.pk, 32) == 0) { FAIL("same"); return; }
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_kx_agree(void) {
    TEST("ECDH key agreement");
    ai_tp_crypto_init();
    ai_tp_crypto_keypair_t alice, bob;
    ai_tp_crypto_kx_keypair(&alice);
    ai_tp_crypto_kx_keypair(&bob);
    ai_tp_crypto_session_key_t shared_a, shared_b;
    ai_tp_crypto_kx_agree(&shared_a, &alice.sec, &bob.pub);
    ai_tp_crypto_kx_agree(&shared_b, &bob.sec, &alice.pub);
    /* Both parties should derive the same shared secret */
    if (!ai_tp_crypto_mem_eq(shared_a.bytes, shared_b.bytes, 32)) { FAIL("mismatch"); return; }
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_aead_encrypt_decrypt(void) {
    TEST("AEAD encrypt then decrypt");
    ai_tp_crypto_init();
    ai_tp_crypto_keypair_t kp;
    ai_tp_crypto_kx_keypair(&kp);
    ai_tp_crypto_sym_key_t key;
    memcpy(key.bytes, kp.pub.pk, 32);
    ai_tp_crypto_nonce_t nonce;
    ai_tp_crypto_random_nonce(&nonce);

    const char *plain = "Hello AI-TP encrypted world!";
    size_t pt_len = strlen(plain);
    uint8_t ct[256], pt2[256];
    size_t ct_len = 0, pt2_len = 0;
    ai_tp_crypto_aead_tag_t tag;

    if (ai_tp_crypto_aead_encrypt(ct, &ct_len, (const uint8_t *)plain, pt_len,
            NULL, 0, &nonce, &key, AI_TP_CRYPTO_SYM_AES256GCM, &tag) != AI_TP_CRYPTO_OK) {
        FAIL("encrypt"); return;
    }
    if (ct_len != pt_len) { FAIL("ct_len"); return; }

    if (ai_tp_crypto_aead_decrypt(pt2, &pt2_len, ct, ct_len,
            NULL, 0, &nonce, &key, &tag, AI_TP_CRYPTO_SYM_AES256GCM) != AI_TP_CRYPTO_OK) {
        FAIL("decrypt"); return;
    }
    if (pt2_len != pt_len || memcmp(plain, pt2, pt_len) != 0) { FAIL("mismatch"); return; }

    /* Tampered tag should fail */
    ai_tp_crypto_aead_tag_t bad_tag = tag;
    bad_tag.bytes[0] ^= 0xFF;
    if (ai_tp_crypto_aead_decrypt(pt2, &pt2_len, ct, ct_len,
            NULL, 0, &nonce, &key, &bad_tag, AI_TP_CRYPTO_SYM_AES256GCM) == AI_TP_CRYPTO_OK) {
        FAIL("tamper"); return;
    }
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_sign_verify(void) {
    TEST("sign and verify");
    ai_tp_crypto_init();
    ai_tp_crypto_keypair_t kp;
    ai_tp_crypto_sign_keypair(&kp);
    const char *msg = "test message for signing";
    ai_tp_crypto_signature_t sig;

    if (ai_tp_crypto_sign(&sig, (const uint8_t *)msg, strlen(msg), &kp.sec) != AI_TP_CRYPTO_OK) {
        FAIL("sign"); return;
    }
    if (!ai_tp_crypto_sign_verify(&sig, (const uint8_t *)msg, strlen(msg), &kp.pub)) {
        FAIL("verify"); return;
    }
    /* Wrong message should fail */
    if (ai_tp_crypto_sign_verify(&sig, (const uint8_t *)"wrong", 5, &kp.pub)) {
        FAIL("wrong msg"); return;
    }
    /* Wrong key should fail */
    ai_tp_crypto_keypair_t kp2;
    ai_tp_crypto_sign_keypair(&kp2);
    if (ai_tp_crypto_sign_verify(&sig, (const uint8_t *)msg, strlen(msg), &kp2.pub)) {
        FAIL("wrong key"); return;
    }
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_hash(void) {
    TEST("SHA-256 and SHA-512");
    ai_tp_crypto_init();
    const char *data = "hash me";
    ai_tp_crypto_sha256_t h256;
    ai_tp_crypto_sha512_t h512;

    if (ai_tp_crypto_sha256(&h256, (const uint8_t *)data, strlen(data)) != AI_TP_CRYPTO_OK) { FAIL("sha256"); return; }
    if (ai_tp_crypto_sha512(&h512, (const uint8_t *)data, strlen(data)) != AI_TP_CRYPTO_OK) { FAIL("sha512"); return; }
    /* Deterministic */
    ai_tp_crypto_sha256_t h256_2;
    ai_tp_crypto_sha256(&h256_2, (const uint8_t *)data, strlen(data));
    if (!ai_tp_crypto_mem_eq(h256.bytes, h256_2.bytes, 32)) { FAIL("det"); return; }
    /* Different input -> different hash */
    ai_tp_crypto_sha256_t h256_3;
    ai_tp_crypto_sha256(&h256_3, (const uint8_t *)"different", 9);
    if (ai_tp_crypto_mem_eq(h256.bytes, h256_3.bytes, 32)) { FAIL("collision"); return; }
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_hmac(void) {
    TEST("HMAC compute and verify");
    ai_tp_crypto_init();
    ai_tp_crypto_hmac_key_t key;
    ai_tp_crypto_random_bytes(key.bytes, 32);
    const char *data = "authenticate this";

    ai_tp_crypto_hmac_t mac1, mac2;
    ai_tp_crypto_hmac(&mac1, &key, (const uint8_t *)data, strlen(data));
    ai_tp_crypto_hmac(&mac2, &key, (const uint8_t *)data, strlen(data));
    if (!ai_tp_crypto_hmac_verify(&mac1, &mac2)) { FAIL("verify"); return; }

    /* Different data -> different HMAC */
    ai_tp_crypto_hmac_t mac3;
    ai_tp_crypto_hmac(&mac3, &key, (const uint8_t *)"different", 9);
    if (ai_tp_crypto_hmac_verify(&mac1, &mac3)) { FAIL("collision"); return; }
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_ratchet(void) {
    TEST("ratchet forward secrecy");
    ai_tp_crypto_init();
    /* Alice initiates */
    ai_tp_crypto_keypair_t alice_id, alice_eph, bob_id;
    ai_tp_crypto_sign_keypair(&alice_id);
    ai_tp_crypto_kx_keypair(&alice_eph);
    ai_tp_crypto_sign_keypair(&bob_id);

    ai_tp_crypto_ratchet_t alice_r, bob_r;
    ai_tp_crypto_ratchet_init_initiator(&alice_r, &alice_id, &alice_eph, &bob_id.pub);
    ai_tp_crypto_ratchet_init_responder(&bob_r, &bob_id, &alice_id.pub, &alice_eph.pub);

    /* Alice sends message 1 */
    ai_tp_crypto_sym_key_t msg1_a, msg1_b;
    ai_tp_crypto_ratchet_send(&msg1_a, &alice_r);
    ai_tp_crypto_ratchet_recv(&msg1_b, &bob_r);

    /* Alice sends message 2 (different key due to ratchet) */
    ai_tp_crypto_sym_key_t msg2_a;
    ai_tp_crypto_ratchet_send(&msg2_a, &alice_r);
    if (ai_tp_crypto_mem_eq(msg1_a.bytes, msg2_a.bytes, 32)) { FAIL("same key"); return; }

    ai_tp_crypto_ratchet_destroy(&alice_r);
    ai_tp_crypto_ratchet_destroy(&bob_r);
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_address(void) {
    TEST("pubkey to ai-tp address");
    ai_tp_crypto_init();
    ai_tp_crypto_keypair_t kp;
    ai_tp_crypto_sign_keypair(&kp);
    char addr[64];
    if (ai_tp_crypto_pubkey_to_address(addr, sizeof(addr), &kp.pub) != AI_TP_CRYPTO_OK) { FAIL("addr"); return; }
    if (strncmp(addr, "ai-tp:", 6) != 0) { FAIL("prefix"); return; }
    if (strlen(addr) < 10) { FAIL("short"); return; }
    ai_tp_crypto_shutdown();
    PASS();
}

static void test_mem_eq(void) {
    TEST("constant-time mem_eq");
    uint8_t a[8] = {1,2,3,4,5,6,7,8};
    uint8_t b[8] = {1,2,3,4,5,6,7,8};
    uint8_t c[8] = {1,2,3,4,5,6,7,9};
    if (!ai_tp_crypto_mem_eq(a, b, 8)) { FAIL("equal"); return; }
    if (ai_tp_crypto_mem_eq(a, c, 8)) { FAIL("diff"); return; }
    PASS();
}

static void test_mem_zero(void) {
    TEST("secure mem_zero");
    uint8_t buf[16];
    memset(buf, 0xAA, 16);
    ai_tp_crypto_mem_zero(buf, 16);
    for (int i = 0; i < 16; i++) {
        if (buf[i] != 0) { FAIL("not zero"); return; }
    }
    PASS();
}

int main(void) {
    printf("=== ai-tp-crypto tests ===\n\n");
    test_init_shutdown();
    test_random();
    test_nonce_salt();
    test_sign_keypair();
    test_kx_keypair();
    test_kx_agree();
    test_aead_encrypt_decrypt();
    test_sign_verify();
    test_hash();
    test_hmac();
    test_ratchet();
    test_address();
    test_mem_eq();
    test_mem_zero();
    printf("\n=== Results: %d/%d passed ===\n", passed, total);
    return (passed == total) ? 0 : 1;
}

