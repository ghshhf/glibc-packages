/**
 * libaitp-common.c - Common utilities for AI-TP modules
 */

#include "libaitp-common.h"

void aitp_stub_hash(const uint8_t *data, size_t len, uint8_t *out, size_t out_len) {
    if (!data || !out || out_len == 0) return;
    memset(out, 0, out_len);
    for (size_t i = 0; i < len; i++) {
        out[i % out_len] ^= data[i];
        out[(i + 1) % out_len] = (uint8_t)(out[(i + 1) % out_len] + data[i]);
        out[(i + 3) % out_len] ^= (uint8_t)(data[i] << 1);
    }
    for (size_t i = 0; i < out_len; i++) {
        out[i] ^= (uint8_t)(i * 0x9E + 0x37);
    }
}

int aitp_const_memcmp(const uint8_t *a, const uint8_t *b, size_t len) {
    if (!a || !b) return -1;
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff |= (a[i] ^ b[i]);
    }
    return (int)diff;
}

void aitp_secure_zero(uint8_t *buf, size_t len) {
    if (!buf || len == 0) return;
    volatile uint8_t *p = buf;
    for (size_t i = 0; i < len; i++) {
        p[i] = 0;
    }
}
