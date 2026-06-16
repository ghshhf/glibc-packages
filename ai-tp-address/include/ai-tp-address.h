/**
 * ai-tp-address.h - Decentralized Addressing for AI-TP Network
 *
 * Replaces traditional DNS with a decentralized addressing system:
 * - Address generation from Ed25519 public keys (SHA-256 + base58)
 * - DHT-based resolution (Kademlia-style k-bucket routing)
 * - Human-readable name registration with ownership verification
 * - Local cache with TTL for fast resolution
 * - DNS bridge for traditional internet interop
 */

#ifndef AI_TP_ADDRESS_H
#define AI_TP_ADDRESS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Constants ========== */

#define AITP_PUBKEY_SIZE       32
#define AITP_SIGNATURE_SIZE    64
#define AITP_HASH_SIZE         32
#define AITP_ADDRESS_RAW_LEN   56
#define AITP_ADDRESS_NAME_LEN  64
#define AITP_MAX_NAME_LEN      48
#define AITP_IP_LEN            46
#define AITP_DHT_K             20
#define AITP_DHT_BUCKETS       256
#define AITP_MAX_CACHE_ENTRIES 1024
#define AITP_MAX_DHT_NODES     4096
#define AITP_MAX_NAMES         1024
#define AITP_DEFAULT_CACHE_TTL 300000
#define AITP_DEFAULT_MAX_HOPS  8
#define AITP_DEFAULT_TIMEOUT   5000

/* ========== Enumerations ========== */

typedef enum aitp_addr_type {
    AITP_ADDR_RAW     = 0,
    AITP_ADDR_NAMED   = 1,
    AITP_ADDR_SERVICE = 2
} aitp_addr_type_t;

typedef enum aitp_resolve_status {
    AITP_RESOLVE_OK          = 0,
    AITP_RESOLVE_NOT_FOUND   = -1,
    AITP_RESOLVE_TIMEOUT     = -2,
    AITP_RESOLVE_INVALID     = -3,
    AITP_RESOLVE_CACHED      = 1,
    AITP_RESOLVE_DHT_HIT     = 2,
    AITP_RESOLVE_BRIDGE_HIT  = 3
} aitp_resolve_status_t;

typedef enum aitp_addr_error {
    AITP_ADDR_OK            = 0,
    AITP_ADDR_ERR_INVALID   = -1,
    AITP_ADDR_ERR_EXISTS    = -2,
    AITP_ADDR_ERR_NO_OWNER  = -3,
    AITP_ADDR_ERR_EXPIRED   = -4,
    AITP_ADDR_ERR_FULL      = -5,
    AITP_ADDR_ERR_VERIFY    = -6,
    AITP_ADDR_ERR_NOT_FOUND = -7
} aitp_addr_error_t;

/* ========== Data Structures ========== */

typedef struct aitp_address {
    aitp_addr_type_t type;
    uint8_t  raw_bytes[AITP_HASH_SIZE];
    char     display_str[AITP_ADDRESS_RAW_LEN + 1];
    uint8_t  owner_pubkey[AITP_PUBKEY_SIZE];
    uint8_t  signature[AITP_SIGNATURE_SIZE];
    uint64_t ttl;
} aitp_address_t;

typedef struct aitp_dht_node {
    uint8_t  node_id[AITP_HASH_SIZE];
    char     ip[AITP_IP_LEN];
    uint16_t port;
    uint64_t last_seen;
    int      is_alive;
} aitp_dht_node_t;

typedef struct aitp_name_record {
    char     name[AITP_MAX_NAME_LEN + 1];
    aitp_address_t address;
    uint8_t  owner_pubkey[AITP_PUBKEY_SIZE];
    uint8_t  signature[AITP_SIGNATURE_SIZE];
    uint64_t created_at;
    uint64_t expires_at;
    int      is_active;
} aitp_name_record_t;

typedef struct aitp_cache_entry {
    aitp_address_t  address;
    aitp_dht_node_t resolved_node;
    uint64_t cached_at;
    uint64_t ttl;
    int      is_valid;
} aitp_cache_entry_t;

typedef struct aitp_resolve_result {
    aitp_address_t  address;
    char            node_ip[AITP_IP_LEN];
    uint16_t        node_port;
    uint8_t         pubkey[AITP_PUBKEY_SIZE];
    int             is_verified;
    aitp_resolve_status_t source;
} aitp_resolve_result_t;

typedef struct aitp_resolver_config {
    int      cache_enabled;
    uint64_t cache_ttl_ms;
    int      dht_k;
    int      max_hops;
    uint64_t resolve_timeout_ms;
} aitp_resolver_config_t;

typedef struct aitp_kbucket {
    aitp_dht_node_t nodes[AITP_DHT_K];
    int count;
} aitp_kbucket_t;

typedef struct aitp_dht {
    aitp_kbucket_t buckets[AITP_DHT_BUCKETS];
    uint8_t local_id[AITP_HASH_SIZE];
    int total_nodes;
} aitp_dht_t;

typedef struct aitp_address_ctx {
    aitp_dht_t           dht;
    aitp_cache_entry_t   cache[AITP_MAX_CACHE_ENTRIES];
    int                  cache_count;
    aitp_name_record_t   names[AITP_MAX_NAMES];
    int                  name_count;
    aitp_resolver_config_t config;
    int                  initialized;
} aitp_address_ctx_t;

/* ========== API Functions ========== */

int aitp_address_init(aitp_address_ctx_t *ctx);
int aitp_address_init_with_config(aitp_address_ctx_t *ctx,
                                   const aitp_resolver_config_t *config);
void aitp_address_destroy(aitp_address_ctx_t *ctx);

int aitp_generate_address(const uint8_t pubkey[AITP_PUBKEY_SIZE],
                          aitp_address_t *out_addr);
int aitp_address_to_string(const aitp_address_t *addr, char *buf, size_t buf_len);
int aitp_string_to_address(const char *str, aitp_address_t *out_addr);
int aitp_validate_address_format(const aitp_address_t *addr);
int aitp_validate_address_string(const char *addr_str);

int aitp_register_name(aitp_address_ctx_t *ctx, const char *name,
                       const aitp_address_t *addr,
                       const uint8_t owner_pubkey[AITP_PUBKEY_SIZE],
                       const uint8_t signature[AITP_SIGNATURE_SIZE]);
int aitp_resolve_name(aitp_address_ctx_t *ctx, const char *name,
                      aitp_address_t *out_addr);
int aitp_lookup_name_record(aitp_address_ctx_t *ctx, const char *name,
                            aitp_name_record_t *out_record);
int aitp_unregister_name(aitp_address_ctx_t *ctx, const char *name,
                         const uint8_t owner_pubkey[AITP_PUBKEY_SIZE],
                         const uint8_t signature[AITP_SIGNATURE_SIZE]);

int aitp_dht_add_node(aitp_address_ctx_t *ctx, const aitp_dht_node_t *node);
int aitp_dht_remove_node(aitp_address_ctx_t *ctx,
                         const uint8_t node_id[AITP_HASH_SIZE]);
int aitp_dht_find_node(aitp_address_ctx_t *ctx,
                       const uint8_t target_id[AITP_HASH_SIZE],
                       aitp_dht_node_t *out_nodes, int max_nodes);
int aitp_dht_get_stats(aitp_address_ctx_t *ctx, int *total, int *alive);

int aitp_resolve_address(aitp_address_ctx_t *ctx, const aitp_address_t *addr,
                         aitp_resolve_result_t *out_result);
int aitp_resolve_address_string(aitp_address_ctx_t *ctx, const char *addr_str,
                                aitp_resolve_result_t *out_result);

int aitp_cache_lookup(aitp_address_ctx_t *ctx, const aitp_address_t *addr,
                      aitp_cache_entry_t *out_entry);
int aitp_cache_store(aitp_address_ctx_t *ctx, const aitp_address_t *addr,
                     const aitp_dht_node_t *node, uint64_t ttl);
int aitp_cache_invalidate(aitp_address_ctx_t *ctx, const aitp_address_t *addr);
int aitp_cache_clear(aitp_address_ctx_t *ctx);

int aitp_verify_address_ownership(const aitp_address_t *addr,
                                  const uint8_t pubkey[AITP_PUBKEY_SIZE],
                                  const uint8_t signature[AITP_SIGNATURE_SIZE]);

int aitp_bridge_dns_lookup(aitp_address_ctx_t *ctx, const char *domain,
                           char *out_ip, size_t ip_len);

int aitp_get_config(aitp_address_ctx_t *ctx, aitp_resolver_config_t *out_config);
int aitp_set_config(aitp_address_ctx_t *ctx, const aitp_resolver_config_t *config);

const char *aitp_addr_type_name(aitp_addr_type_t type);
const char *aitp_resolve_status_name(aitp_resolve_status_t status);
const char *aitp_addr_error_name(aitp_addr_error_t err);
void aitp_xor_distance(const uint8_t a[AITP_HASH_SIZE],
                       const uint8_t b[AITP_HASH_SIZE],
                       uint8_t out[AITP_HASH_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_ADDRESS_H */
