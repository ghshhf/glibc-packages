/**
 * ai-tp-address.c - Decentralized Addressing Implementation
 */

#include "ai-tp-address.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* ========== Base58 ========== */

static const char B58[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static int b58_encode(const uint8_t *data, size_t len, char *out, size_t olen) {
    size_t i, j;
    uint8_t *tmp = (uint8_t *)calloc(len * 2 + 1, 1);
    if (!tmp) return -1;
    memcpy(tmp, data, len);
    j = 0;
    while (j < olen - 1) {
        int carry = 0, zero = 1;
        for (i = 0; i < len; i++) {
            int v = tmp[i] + carry * 256;
            tmp[i] = (uint8_t)(v / 58);
            carry = v % 58;
            if (tmp[i]) zero = 0;
        }
        if (zero) break;
        out[j++] = B58[carry];
    }
    /* leading 1s for zero bytes */
    for (i = 0; i < len && data[i] == 0 && j < olen - 1; i++)
        out[j++] = B58[0];
    out[j] = 0;
    /* reverse */
    for (i = 0; i < j / 2; i++) {
        char t = out[i]; out[i] = out[j-1-i]; out[j-1-i] = t;
    }
    free(tmp);
    return 0;
}

static int b58_decode(const char *s, uint8_t *out, size_t olen) {
    size_t i, j;
    uint8_t *tmp = (uint8_t *)calloc(olen, 1);
    if (!tmp) return -1;
    memset(out, 0, olen);
    for (i = 0; s[i]; i++) {
        const char *p = strchr(B58, s[i]);
        if (!p) { free(tmp); return -1; }
        int carry = (int)(p - B58);
        for (j = 0; j < olen; j++) {
            int v = tmp[j] * 58 + carry;
            tmp[j] = (uint8_t)(v & 0xFF);
            carry = v >> 8;
        }
        if (carry) { free(tmp); return -1; }
    }
    memcpy(out, tmp, olen);
    free(tmp);
    return 0;
}

/* ========== Stub Crypto ========== */

static void stub_hash(const uint8_t *d, size_t n, uint8_t out[32]) {
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

static void stub_sign(const uint8_t *msg, size_t len,
                      const uint8_t pk[32], uint8_t sig[64]) {
    uint8_t buf[4096];
    size_t cl = len < 4064 ? len : 4064;
    memcpy(buf, msg, cl);
    memcpy(buf + cl, pk, 32);
    stub_hash(buf, cl + 32, sig);
    stub_hash(sig, 32, sig + 32);
}

static int stub_verify(const uint8_t *msg, size_t len,
                       const uint8_t pk[32], const uint8_t sig[64]) {
    uint8_t exp[64];
    stub_sign(msg, len, pk, exp);
    return memcmp(exp, sig, 64) == 0;
}

static uint64_t now_ms(void) { return (uint64_t)time(NULL) * 1000; }

/* ========== Init/Destroy ========== */

int aitp_address_init(aitp_address_ctx_t *ctx) {
    aitp_resolver_config_t cfg = {
        .cache_enabled = 1, .cache_ttl_ms = AITP_DEFAULT_CACHE_TTL,
        .dht_k = AITP_DHT_K, .max_hops = AITP_DEFAULT_MAX_HOPS,
        .resolve_timeout_ms = AITP_DEFAULT_TIMEOUT
    };
    return aitp_address_init_with_config(ctx, &cfg);
}

int aitp_address_init_with_config(aitp_address_ctx_t *ctx,
                                   const aitp_resolver_config_t *cfg) {
    if (!ctx || !cfg) return -1;
    memset(ctx, 0, sizeof(*ctx));
    ctx->config = *cfg;
    uint8_t seed[8] = {0xA1,0xB2,0xC3,0xD4,0xE5,0xF6,0x17,0x28};
    uint64_t ts = now_ms();
    memcpy(seed, &ts, 8);
    stub_hash(seed, 8, ctx->dht.local_id);
    ctx->initialized = 1;
    return 0;
}

void aitp_address_destroy(aitp_address_ctx_t *ctx) {
    if (ctx) memset(ctx, 0, sizeof(*ctx));
}

/* ========== Address Generation ========== */

int aitp_generate_address(const uint8_t pk[32], aitp_address_t *a) {
    if (!pk || !a) return -1;
    memset(a, 0, sizeof(*a));
    a->type = AITP_ADDR_RAW;
    stub_hash(pk, 32, a->raw_bytes);
    char b58[64];
    b58_encode(a->raw_bytes, 32, b58, 64);
    snprintf(a->display_str, sizeof(a->display_str), "at1%s", b58);
    memcpy(a->owner_pubkey, pk, 32);
    return 0;
}

int aitp_address_to_string(const aitp_address_t *a, char *buf, size_t n) {
    if (!a || !buf || n == 0) return -1;
    strncpy(buf, a->display_str, n - 1);
    buf[n - 1] = 0;
    return 0;
}

int aitp_string_to_address(const char *s, aitp_address_t *a) {
    if (!s || !a) return -1;
    memset(a, 0, sizeof(*a));
    if (strncmp(s, "at1", 3) == 0) {
        a->type = AITP_ADDR_RAW;
        strncpy(a->display_str, s, AITP_ADDRESS_RAW_LEN);
        b58_decode(s + 3, a->raw_bytes, 32);
    } else if (s[0] == (char)64) {  /* @ */
        a->type = AITP_ADDR_NAMED;
        strncpy(a->display_str, s, AITP_ADDRESS_NAME_LEN);
    } else if (strncmp(s, "svc:", 4) == 0) {
        a->type = AITP_ADDR_SERVICE;
        strncpy(a->display_str, s, AITP_ADDRESS_NAME_LEN);
    } else return -1;
    return 0;
}

int aitp_validate_address_format(const aitp_address_t *a) {
    if (!a) return 0;
    switch (a->type) {
    case AITP_ADDR_RAW:     return strncmp(a->display_str,"at1",3)==0 && strlen(a->display_str)>=10;
    case AITP_ADDR_NAMED:   return a->display_str[0]==(char)64 && strlen(a->display_str)>=3;
    case AITP_ADDR_SERVICE: return strncmp(a->display_str,"svc:",4)==0 && strlen(a->display_str)>=8;
    default: return 0;
    }
}

int aitp_validate_address_string(const char *s) {
    if (!s) return 0;
    if (strncmp(s,"at1",3)==0 && strlen(s)>=10) return 1;
    if (s[0]==(char)64 && strlen(s)>=3) return 1;
    if (strncmp(s,"svc:",4)==0 && strlen(s)>=8) return 1;
    return 0;
}

/* ========== Name Registry ========== */

int aitp_register_name(aitp_address_ctx_t *ctx, const char *name,
                       const aitp_address_t *addr,
                       const uint8_t pk[32], const uint8_t sig[64]) {
    if (!ctx || !name || !addr || !pk || !sig) return -1;
    if (!ctx->initialized) return -1;
    if (name[0] != (char)64) return AITP_ADDR_ERR_INVALID;
    if (strlen(name) < 3 || strlen(name) > AITP_MAX_NAME_LEN) return AITP_ADDR_ERR_INVALID;
    for (int i = 0; i < ctx->name_count; i++)
        if (ctx->names[i].is_active && strcmp(ctx->names[i].name, name) == 0)
            return AITP_ADDR_ERR_EXISTS;
    if (ctx->name_count >= AITP_MAX_NAMES) return AITP_ADDR_ERR_FULL;
    uint8_t msg[256];
    size_t ml = (size_t)snprintf((char*)msg, sizeof(msg), "%s:%s", name, addr->display_str);
    if (!stub_verify(msg, ml, pk, sig)) return AITP_ADDR_ERR_VERIFY;
    aitp_name_record_t *r = &ctx->names[ctx->name_count];
    strncpy(r->name, name, AITP_MAX_NAME_LEN);
    r->address = *addr;
    memcpy(r->owner_pubkey, pk, 32);
    memcpy(r->signature, sig, 64);
    r->created_at = now_ms();
    r->expires_at = r->created_at + 365ULL*24*3600*1000;
    r->is_active = 1;
    ctx->name_count++;
    return AITP_ADDR_OK;
}

int aitp_resolve_name(aitp_address_ctx_t *ctx, const char *name,
                      aitp_address_t *out) {
    if (!ctx || !name || !out) return -1;
    for (int i = 0; i < ctx->name_count; i++)
        if (ctx->names[i].is_active && strcmp(ctx->names[i].name, name) == 0) {
            *out = ctx->names[i].address;
            return AITP_ADDR_OK;
        }
    return AITP_ADDR_ERR_NOT_FOUND;
}

int aitp_lookup_name_record(aitp_address_ctx_t *ctx, const char *name,
                            aitp_name_record_t *out) {
    if (!ctx || !name || !out) return -1;
    for (int i = 0; i < ctx->name_count; i++)
        if (ctx->names[i].is_active && strcmp(ctx->names[i].name, name) == 0) {
            *out = ctx->names[i];
            return AITP_ADDR_OK;
        }
    return AITP_ADDR_ERR_NOT_FOUND;
}

int aitp_unregister_name(aitp_address_ctx_t *ctx, const char *name,
                         const uint8_t pk[32], const uint8_t sig[64]) {
    if (!ctx || !name || !pk || !sig) return -1;
    for (int i = 0; i < ctx->name_count; i++) {
        if (ctx->names[i].is_active && strcmp(ctx->names[i].name, name) == 0) {
            if (memcmp(ctx->names[i].owner_pubkey, pk, 32) != 0)
                return AITP_ADDR_ERR_NO_OWNER;
            uint8_t msg[256];
            size_t ml = (size_t)snprintf((char*)msg, sizeof(msg), "revoke:%s", name);
            if (!stub_verify(msg, ml, pk, sig)) return AITP_ADDR_ERR_VERIFY;
            ctx->names[i].is_active = 0;
            return AITP_ADDR_OK;
        }
    }
    return AITP_ADDR_ERR_NOT_FOUND;
}

/* ========== DHT ========== */

static int bucket_idx(const uint8_t local[32], const uint8_t node[32]) {
    for (int i = 0; i < 32; i++) {
        uint8_t x = local[i] ^ node[i];
        if (x) { int b = 7; while (b >= 0 && !(x & (1 << b))) b--; return i*8+(7-b); }
    }
    return 255;
}

int aitp_dht_add_node(aitp_address_ctx_t *ctx, const aitp_dht_node_t *n) {
    if (!ctx || !n || !ctx->initialized) return -1;
    int idx = bucket_idx(ctx->dht.local_id, n->node_id);
    if (idx < 0 || idx >= AITP_DHT_BUCKETS) return -1;
    aitp_kbucket_t *b = &ctx->dht.buckets[idx];
    for (int i = 0; i < b->count; i++)
        if (memcmp(b->nodes[i].node_id, n->node_id, 32) == 0) {
            b->nodes[i] = *n; b->nodes[i].last_seen = now_ms(); b->nodes[i].is_alive = 1;
            return 0;
        }
    if (b->count >= ctx->config.dht_k) return -1;
    b->nodes[b->count] = *n;
    b->nodes[b->count].last_seen = now_ms();
    b->nodes[b->count].is_alive = 1;
    b->count++; ctx->dht.total_nodes++;
    return 0;
}

int aitp_dht_remove_node(aitp_address_ctx_t *ctx, const uint8_t nid[32]) {
    if (!ctx || !nid) return -1;
    int idx = bucket_idx(ctx->dht.local_id, nid);
    if (idx < 0 || idx >= AITP_DHT_BUCKETS) return -1;
    aitp_kbucket_t *b = &ctx->dht.buckets[idx];
    for (int i = 0; i < b->count; i++)
        if (memcmp(b->nodes[i].node_id, nid, 32) == 0) {
            for (int j = i; j < b->count-1; j++) b->nodes[j] = b->nodes[j+1];
            b->count--; ctx->dht.total_nodes--;
            return 0;
        }
    return -1;
}

int aitp_dht_find_node(aitp_address_ctx_t *ctx, const uint8_t tgt[32],
                       aitp_dht_node_t *out, int max) {
    if (!ctx || !tgt || !out || max <= 0) return 0;
    int found = 0, start = bucket_idx(ctx->dht.local_id, tgt);
    for (int off = 0; off < AITP_DHT_BUCKETS && found < max; off++) {
        for (int dir = 0; dir <= 1 && found < max; dir++) {
            int idx = start + (dir == 0 ? off : -off);
            if (idx < 0 || idx >= AITP_DHT_BUCKETS) continue;
            if (off == 0 && dir == 1) continue;
            aitp_kbucket_t *b = &ctx->dht.buckets[idx];
            for (int i = 0; i < b->count && found < max; i++)
                if (b->nodes[i].is_alive) out[found++] = b->nodes[i];
        }
    }
    return found;
}

int aitp_dht_get_stats(aitp_address_ctx_t *ctx, int *total, int *alive) {
    if (!ctx) return -1;
    if (total) *total = ctx->dht.total_nodes;
    if (alive) {
        int a = 0;
        for (int i = 0; i < AITP_DHT_BUCKETS; i++)
            for (int j = 0; j < ctx->dht.buckets[i].count; j++)
                if (ctx->dht.buckets[i].nodes[j].is_alive) a++;
        *alive = a;
    }
    return 0;
}

/* ========== Resolution ========== */

int aitp_resolve_address(aitp_address_ctx_t *ctx, const aitp_address_t *addr,
                         aitp_resolve_result_t *res) {
    if (!ctx || !addr || !res) return AITP_RESOLVE_INVALID;
    memset(res, 0, sizeof(*res));
    res->address = *addr;
    /* cache */
    if (ctx->config.cache_enabled) {
        aitp_cache_entry_t e;
        if (aitp_cache_lookup(ctx, addr, &e) == 0) {
            res->node_port = e.resolved_node.port;
            strncpy(res->node_ip, e.resolved_node.ip, AITP_IP_LEN-1);
            memcpy(res->pubkey, addr->owner_pubkey, 32);
            res->source = AITP_RESOLVE_CACHED;
            res->is_verified = 1;
            return AITP_RESOLVE_CACHED;
        }
    }
    /* DHT */
    aitp_dht_node_t nodes[3];
    int n = aitp_dht_find_node(ctx, addr->raw_bytes, nodes, 3);
    if (n > 0) {
        res->node_port = nodes[0].port;
        strncpy(res->node_ip, nodes[0].ip, AITP_IP_LEN-1);
        memcpy(res->pubkey, addr->owner_pubkey, 32);
        res->source = AITP_RESOLVE_DHT_HIT;
        res->is_verified = 1;
        if (ctx->config.cache_enabled)
            aitp_cache_store(ctx, addr, &nodes[0], ctx->config.cache_ttl_ms);
        return AITP_RESOLVE_DHT_HIT;
    }
    /* bridge */
    char ip[AITP_IP_LEN];
    if (aitp_bridge_dns_lookup(ctx, addr->display_str, ip, sizeof(ip)) == 0) {
        strncpy(res->node_ip, ip, AITP_IP_LEN-1);
        res->source = AITP_RESOLVE_BRIDGE_HIT;
        res->is_verified = 0;
        return AITP_RESOLVE_BRIDGE_HIT;
    }
    return AITP_RESOLVE_NOT_FOUND;
}

int aitp_resolve_address_string(aitp_address_ctx_t *ctx, const char *s,
                                aitp_resolve_result_t *res) {
    if (!ctx || !s || !res) return AITP_RESOLVE_INVALID;
    if (s[0] == (char)64) {
        aitp_address_t a;
        if (aitp_resolve_name(ctx, s, &a) == AITP_ADDR_OK)
            return aitp_resolve_address(ctx, &a, res);
    }
    aitp_address_t a;
    if (aitp_string_to_address(s, &a) != 0) return AITP_RESOLVE_INVALID;
    return aitp_resolve_address(ctx, &a, res);
}

/* ========== Cache ========== */

int aitp_cache_lookup(aitp_address_ctx_t *ctx, const aitp_address_t *addr,
                      aitp_cache_entry_t *out) {
    if (!ctx || !addr || !out) return -1;
    uint64_t t = now_ms();
    for (int i = 0; i < ctx->cache_count; i++) {
        if (!ctx->cache[i].is_valid) continue;
        if (memcmp(ctx->cache[i].address.raw_bytes, addr->raw_bytes, 32) == 0) {
            if (t - ctx->cache[i].cached_at > ctx->cache[i].ttl) {
                ctx->cache[i].is_valid = 0; continue;
            }
            *out = ctx->cache[i]; return 0;
        }
    }
    return -1;
}

int aitp_cache_store(aitp_address_ctx_t *ctx, const aitp_address_t *addr,
                     const aitp_dht_node_t *node, uint64_t ttl) {
    if (!ctx || !addr || !node) return -1;
    for (int i = 0; i < ctx->cache_count; i++)
        if (memcmp(ctx->cache[i].address.raw_bytes, addr->raw_bytes, 32) == 0) {
            ctx->cache[i].address = *addr;
            ctx->cache[i].resolved_node = *node;
            ctx->cache[i].cached_at = now_ms();
            ctx->cache[i].ttl = ttl;
            ctx->cache[i].is_valid = 1;
            return 0;
        }
    int slot = -1;
    if (ctx->cache_count >= AITP_MAX_CACHE_ENTRIES) {
        uint64_t oldest = (uint64_t)-1;
        for (int i = 0; i < ctx->cache_count; i++)
            if (ctx->cache[i].cached_at < oldest) { oldest = ctx->cache[i].cached_at; slot = i; }
    } else { slot = ctx->cache_count++; }
    if (slot < 0) return -1;
    ctx->cache[slot].address = *addr;
    ctx->cache[slot].resolved_node = *node;
    ctx->cache[slot].cached_at = now_ms();
    ctx->cache[slot].ttl = ttl;
    ctx->cache[slot].is_valid = 1;
    return 0;
}

int aitp_cache_invalidate(aitp_address_ctx_t *ctx, const aitp_address_t *addr) {
    if (!ctx || !addr) return -1;
    for (int i = 0; i < ctx->cache_count; i++)
        if (memcmp(ctx->cache[i].address.raw_bytes, addr->raw_bytes, 32) == 0) {
            ctx->cache[i].is_valid = 0; return 0;
        }
    return -1;
}

int aitp_cache_clear(aitp_address_ctx_t *ctx) {
    if (!ctx) return -1;
    ctx->cache_count = 0;
    memset(ctx->cache, 0, sizeof(ctx->cache));
    return 0;
}

/* ========== Verification ========== */

int aitp_verify_address_ownership(const aitp_address_t *addr,
                                  const uint8_t pk[32], const uint8_t sig[64]) {
    if (!addr || !pk || !sig) return 0;
    if (memcmp(addr->owner_pubkey, pk, 32) != 0) return 0;
    return stub_verify((const uint8_t*)addr->display_str,
                       strlen(addr->display_str), pk, sig);
}

/* ========== DNS Bridge ========== */

int aitp_bridge_dns_lookup(aitp_address_ctx_t *ctx, const char *domain,
                           char *out_ip, size_t ip_len) {
    if (!ctx || !domain || !out_ip) return -1;
    (void)ctx;
    size_t len = strlen(domain);
    if (len < 4 || strcmp(domain + len - 3, ".at") != 0) return -1;
    uint8_t h[32];
    stub_hash((const uint8_t*)domain, len, h);
    snprintf(out_ip, ip_len, "10.%d.%d.%d", h[0]%256, h[1]%256, h[2]%256);
    return 0;
}

/* ========== Config ========== */

int aitp_get_config(aitp_address_ctx_t *ctx, aitp_resolver_config_t *out) {
    if (!ctx || !out) return -1;
    *out = ctx->config; return 0;
}

int aitp_set_config(aitp_address_ctx_t *ctx, const aitp_resolver_config_t *cfg) {
    if (!ctx || !cfg) return -1;
    ctx->config = *cfg; return 0;
}

/* ========== Utility ========== */

const char *aitp_addr_type_name(aitp_addr_type_t t) {
    switch (t) {
    case AITP_ADDR_RAW:     return "RAW";
    case AITP_ADDR_NAMED:   return "NAMED";
    case AITP_ADDR_SERVICE: return "SERVICE";
    default:                return "UNKNOWN";
    }
}

const char *aitp_resolve_status_name(aitp_resolve_status_t s) {
    switch (s) {
    case AITP_RESOLVE_OK:         return "OK";
    case AITP_RESOLVE_NOT_FOUND:  return "NOT_FOUND";
    case AITP_RESOLVE_TIMEOUT:    return "TIMEOUT";
    case AITP_RESOLVE_INVALID:    return "INVALID";
    case AITP_RESOLVE_CACHED:     return "CACHED";
    case AITP_RESOLVE_DHT_HIT:    return "DHT_HIT";
    case AITP_RESOLVE_BRIDGE_HIT: return "BRIDGE_HIT";
    default:                      return "UNKNOWN";
    }
}

const char *aitp_addr_error_name(aitp_addr_error_t e) {
    switch (e) {
    case AITP_ADDR_OK:           return "OK";
    case AITP_ADDR_ERR_INVALID:  return "INVALID";
    case AITP_ADDR_ERR_EXISTS:   return "EXISTS";
    case AITP_ADDR_ERR_NO_OWNER: return "NO_OWNER";
    case AITP_ADDR_ERR_EXPIRED:  return "EXPIRED";
    case AITP_ADDR_ERR_FULL:     return "FULL";
    case AITP_ADDR_ERR_VERIFY:   return "VERIFY_FAILED";
    case AITP_ADDR_ERR_NOT_FOUND:return "NOT_FOUND";
    default:                     return "UNKNOWN";
    }
}

void aitp_xor_distance(const uint8_t a[32], const uint8_t b[32], uint8_t out[32]) {
    for (int i = 0; i < 32; i++) out[i] = a[i] ^ b[i];
}
