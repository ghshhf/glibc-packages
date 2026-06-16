/**
 * @file ai-tp-gateway.h
 * @brief AI-TP OS 网关模式 API
 * @version 1.0.0
 * @date 2026-06-16
 *
 * 功能：
 * 1. 传统互联网 <-> AI-TP 网络协议转换
 * 2. TCP/UDP 请求代理转发
 * 3. AI-TP 消息解包回传统协议
 * 4. 网关路由表管理
 * 5. 访问控制与流量统计
 */

#ifndef AI_TP_GATEWAY_H
#define AI_TP_GATEWAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AI_TP_GATEWAY_VERSION_MAJOR 1
#define AI_TP_GATEWAY_VERSION_MINOR 0
#define AI_TP_GATEWAY_VERSION_PATCH 0

#define AI_TP_GW_MAX_ROUTES      256
#define AI_TP_GW_MAX_HOST        128
#define AI_TP_GW_MAX_PORT        6
#define AI_TP_GW_MAX_ACL_RULES   128
#define AI_TP_GW_MAX_DOMAIN      256
#define AI_TP_GW_DEFAULT_RESOURCE_RATIO  20
#define AI_TP_GW_MAX_BANDWIDTH   (100 * 1024 * 1024)

#define AI_TP_GW_OK              0
#define AI_TP_GW_ERR_INIT       -1
#define AI_TP_GW_ERR_NOMEM      -2
#define AI_TP_GW_ERR_NOT_FOUND  -3
#define AI_TP_GW_ERR_EXISTS     -4
#define AI_TP_GW_ERR_DENIED     -5
#define AI_TP_GW_ERR_IO         -6
#define AI_TP_GW_ERR_TIMEOUT    -7
#define AI_TP_GW_ERR_OVERLOAD   -8
#define AI_TP_GW_ERR_PROTOCOL   -9

typedef enum {
    AI_TP_GW_PROTO_TCP = 0,
    AI_TP_GW_PROTO_UDP = 1,
    AI_TP_GW_PROTO_ICMP = 2,
    AI_TP_GW_PROTO_HTTP = 3,
    AI_TP_GW_PROTO_HTTPS = 4,
    AI_TP_GW_PROTO_ATP  = 5
} ai_tp_gw_protocol_t;

typedef enum {
    AI_TP_GW_MODE_INBOUND  = 0,
    AI_TP_GW_MODE_OUTBOUND = 1,
    AI_TP_GW_MODE_DUAL     = 2
} ai_tp_gw_mode_t;

typedef enum {
    AI_TP_GW_CONN_CLOSED    = 0,
    AI_TP_GW_CONN_HANDSHAKE = 1,
    AI_TP_GW_CONN_ACTIVE    = 2,
    AI_TP_GW_CONN_IDLE      = 3,
    AI_TP_GW_CONN_ERROR     = 4
} ai_tp_gw_conn_state_t;

typedef enum {
    AI_TP_GW_ACL_ALLOW = 0,
    AI_TP_GW_ACL_DENY  = 1
} ai_tp_gw_acl_action_t;

typedef struct {
    char host[AI_TP_GW_MAX_HOST];
    char port[AI_TP_GW_MAX_PORT];
    ai_tp_gw_protocol_t proto;
    char target_node[AI_TP_GW_MAX_HOST];
    char target_port[AI_TP_GW_MAX_PORT];
    bool enabled;
    uint64_t hit_count;
    time_t last_hit;
} ai_tp_gw_route_t;

typedef struct {
    uint64_t conn_id;
    ai_tp_gw_conn_state_t state;
    ai_tp_gw_mode_t direction;
    char src_addr[AI_TP_GW_MAX_HOST];
    char src_port[AI_TP_GW_MAX_PORT];
    char dst_addr[AI_TP_GW_MAX_HOST];
    char dst_port[AI_TP_GW_MAX_PORT];
    ai_tp_gw_protocol_t src_proto;
    ai_tp_gw_protocol_t dst_proto;
    time_t established;
    time_t last_activity;
    uint64_t bytes_in;
    uint64_t bytes_out;
    char handshake_code[64];
} ai_tp_gw_connection_t;

typedef struct {
    ai_tp_gw_acl_action_t action;
    char src_pattern[AI_TP_GW_MAX_DOMAIN];
    char dst_pattern[AI_TP_GW_MAX_DOMAIN];
    ai_tp_gw_protocol_t proto;
    uint16_t priority;
    bool enabled;
} ai_tp_gw_acl_rule_t;

typedef struct {
    uint64_t total_bytes_in;
    uint64_t total_bytes_out;
    uint64_t total_packets_in;
    uint64_t total_packets_out;
    uint64_t active_connections;
    uint64_t total_connections;
    uint64_t rejected_connections;
    time_t uptime;
} ai_tp_gw_stats_t;

typedef struct {
    ai_tp_gw_mode_t mode;
    uint8_t resource_ratio;
    uint64_t bandwidth_limit;
    ai_tp_gw_route_t routes[AI_TP_GW_MAX_ROUTES];
    uint32_t route_count;
    ai_tp_gw_connection_t connections[AI_TP_GW_MAX_ROUTES];
    uint32_t conn_count;
    ai_tp_gw_acl_rule_t acl[AI_TP_GW_MAX_ACL_RULES];
    uint32_t acl_count;
    ai_tp_gw_stats_t stats;
    bool running;
    time_t start_time;
    uint64_t next_conn_id;
    void (*on_connection)(const ai_tp_gw_connection_t *conn, void *user_data);
    void (*on_error)(int error_code, const char *message, void *user_data);
    void *user_data;
} ai_tp_gw_context_t;

int ai_tp_gw_init(ai_tp_gw_context_t *ctx, ai_tp_gw_mode_t mode);
void ai_tp_gw_destroy(ai_tp_gw_context_t *ctx);
int ai_tp_gw_start(ai_tp_gw_context_t *ctx);
void ai_tp_gw_stop(ai_tp_gw_context_t *ctx);

int ai_tp_gw_add_route(ai_tp_gw_context_t *ctx, const ai_tp_gw_route_t *route);
int ai_tp_gw_remove_route(ai_tp_gw_context_t *ctx, const char *host,
                           const char *port, ai_tp_gw_protocol_t proto);
const ai_tp_gw_route_t *ai_tp_gw_find_route(ai_tp_gw_context_t *ctx,
                                              const char *host,
                                              const char *port,
                                              ai_tp_gw_protocol_t proto);

uint64_t ai_tp_gw_connect(ai_tp_gw_context_t *ctx,
                            const char *src_addr, const char *src_port,
                            const char *dst_addr, const char *dst_port,
                            ai_tp_gw_mode_t direction,
                            const char *handshake_code);
int ai_tp_gw_disconnect(ai_tp_gw_context_t *ctx, uint64_t conn_id);
int64_t ai_tp_gw_forward(ai_tp_gw_context_t *ctx, uint64_t conn_id,
                           const void *data, size_t len,
                           ai_tp_gw_mode_t direction);
const ai_tp_gw_connection_t *ai_tp_gw_get_connection(ai_tp_gw_context_t *ctx,
                                                       uint64_t conn_id);

int ai_tp_gw_add_acl(ai_tp_gw_context_t *ctx, const ai_tp_gw_acl_rule_t *rule);
int ai_tp_gw_remove_acl(ai_tp_gw_context_t *ctx, uint32_t index);
bool ai_tp_gw_check_acl(ai_tp_gw_context_t *ctx, const char *src,
                          const char *dst, ai_tp_gw_protocol_t proto);

void ai_tp_gw_set_resource_ratio(ai_tp_gw_context_t *ctx, uint8_t ratio);
void ai_tp_gw_set_bandwidth_limit(ai_tp_gw_context_t *ctx, uint64_t bytes_per_second);
const ai_tp_gw_stats_t *ai_tp_gw_get_stats(ai_tp_gw_context_t *ctx);
const char *ai_tp_gw_proto_name(ai_tp_gw_protocol_t proto);
const char *ai_tp_gw_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_GATEWAY_H */
