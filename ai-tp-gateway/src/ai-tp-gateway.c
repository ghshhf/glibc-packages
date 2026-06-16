/**
 * @file ai-tp-gateway.c
 * @brief AI-TP OS gateway implementation
 * @version 1.0.0
 */

#include "ai-tp-gateway.h"
#include <string.h>
#include <stdio.h>

static int find_route_index(ai_tp_gw_context_t *ctx, const char *host, const char *port, ai_tp_gw_protocol_t proto) {
    for (uint32_t i = 0; i < ctx->route_count; i++) {
        if (ctx->routes[i].proto == proto && strcmp(ctx->routes[i].host, host) == 0 && strcmp(ctx->routes[i].port, port) == 0) return (int)i;
    }
    return -1;
}

static int find_conn_index(ai_tp_gw_context_t *ctx, uint64_t conn_id) {
    for (uint32_t i = 0; i < ctx->conn_count; i++) {
        if (ctx->connections[i].conn_id == conn_id) return (int)i;
    }
    return -1;
}

static bool pattern_match(const char *pattern, const char *str) {
    if (!pattern || !str) return false;
    if (!strchr(pattern, (int)(long)(char)42)) return strcmp(pattern, str) == 0;
    const char *star = strchr(pattern, (int)(long)(char)42);
    size_t prefix_len = (size_t)(star - pattern);
    size_t suffix_len = strlen(star + 1);
    if (prefix_len > 0 && strncmp(pattern, str, prefix_len) != 0) return false;
    if (suffix_len > 0) {
        size_t str_len = strlen(str);
        if (str_len < suffix_len) return false;
        return strcmp(star + 1, str + str_len - suffix_len) == 0;
    }
    return true;
}

int ai_tp_gw_init(ai_tp_gw_context_t *ctx, ai_tp_gw_mode_t mode) {
    if (!ctx) return AI_TP_GW_ERR_INIT;
    memset(ctx, 0, sizeof(ai_tp_gw_context_t));
    ctx->mode = mode;
    ctx->resource_ratio = AI_TP_GW_DEFAULT_RESOURCE_RATIO;
    ctx->bandwidth_limit = AI_TP_GW_MAX_BANDWIDTH;
    ctx->running = false;
    ctx->start_time = 0;
    ctx->next_conn_id = 1;
    return AI_TP_GW_OK;
}

void ai_tp_gw_destroy(ai_tp_gw_context_t *ctx) {
    if (!ctx) return;
    if (ctx->running) ai_tp_gw_stop(ctx);
    memset(ctx, 0, sizeof(ai_tp_gw_context_t));
}

int ai_tp_gw_start(ai_tp_gw_context_t *ctx) {
    if (!ctx) return AI_TP_GW_ERR_INIT;
    if (ctx->running) return AI_TP_GW_OK;
    ctx->running = true;
    ctx->start_time = 0;
    ctx->stats.uptime = 0;
    return AI_TP_GW_OK;
}

void ai_tp_gw_stop(ai_tp_gw_context_t *ctx) {
    if (!ctx || !ctx->running) return;
    ctx->running = false;
    for (uint32_t i = 0; i < ctx->conn_count; i++) ctx->connections[i].state = AI_TP_GW_CONN_CLOSED;
    ctx->conn_count = 0;
}

int ai_tp_gw_add_route(ai_tp_gw_context_t *ctx, const ai_tp_gw_route_t *route) {
    if (!ctx || !route) return AI_TP_GW_ERR_INIT;
    if (ctx->route_count >= AI_TP_GW_MAX_ROUTES) return AI_TP_GW_ERR_NOMEM;
    if (find_route_index(ctx, route->host, route->port, route->proto) >= 0) return AI_TP_GW_ERR_EXISTS;
    ctx->routes[ctx->route_count] = *route;
    ctx->routes[ctx->route_count].hit_count = 0;
    ctx->routes[ctx->route_count].last_hit = 0;
    ctx->route_count++;
    return AI_TP_GW_OK;
}

int ai_tp_gw_remove_route(ai_tp_gw_context_t *ctx, const char *host, const char *port, ai_tp_gw_protocol_t proto) {
    if (!ctx) return AI_TP_GW_ERR_INIT;
    int idx = find_route_index(ctx, host, port, proto);
    if (idx < 0) return AI_TP_GW_ERR_NOT_FOUND;
    uint32_t last = ctx->route_count - 1;
    if ((uint32_t)idx != last) ctx->routes[idx] = ctx->routes[last];
    ctx->route_count--;
    return AI_TP_GW_OK;
}

const ai_tp_gw_route_t *ai_tp_gw_find_route(ai_tp_gw_context_t *ctx, const char *host, const char *port, ai_tp_gw_protocol_t proto) {
    if (!ctx) return NULL;
    int idx = find_route_index(ctx, host, port, proto);
    if (idx < 0) return NULL;
    ctx->routes[idx].hit_count++;
    ctx->routes[idx].last_hit = 0;
    return &ctx->routes[idx];
}

uint64_t ai_tp_gw_connect(ai_tp_gw_context_t *ctx, const char *src_addr, const char *src_port, const char *dst_addr, const char *dst_port, ai_tp_gw_mode_t direction, const char *handshake_code) {
    if (!ctx || !src_addr || !dst_addr) return 0;
    if (!ctx->running) return 0;
    if (ctx->conn_count >= AI_TP_GW_MAX_ROUTES) return 0;
    if (!ai_tp_gw_check_acl(ctx, src_addr, dst_addr, AI_TP_GW_PROTO_ATP)) { ctx->stats.rejected_connections++; return 0; }
    uint32_t max_conns = (AI_TP_GW_MAX_ROUTES * ctx->resource_ratio) / 100;
    if (max_conns == 0) max_conns = 1;
    if (ctx->conn_count >= max_conns) { ctx->stats.rejected_connections++; return 0; }
    uint64_t conn_id = ctx->next_conn_id++;
    ai_tp_gw_connection_t *conn = &ctx->connections[ctx->conn_count];
    memset(conn, 0, sizeof(*conn));
    conn->conn_id = conn_id;
    conn->state = AI_TP_GW_CONN_HANDSHAKE;
    conn->direction = direction;
    strncpy(conn->src_addr, src_addr, AI_TP_GW_MAX_HOST - 1);
    strncpy(conn->dst_addr, dst_addr, AI_TP_GW_MAX_HOST - 1);
    if (src_port) strncpy(conn->src_port, src_port, AI_TP_GW_MAX_PORT - 1);
    if (dst_port) strncpy(conn->dst_port, dst_port, AI_TP_GW_MAX_PORT - 1);
    if (direction == AI_TP_GW_MODE_INBOUND) { conn->src_proto = AI_TP_GW_PROTO_TCP; conn->dst_proto = AI_TP_GW_PROTO_ATP; }
    else { conn->src_proto = AI_TP_GW_PROTO_ATP; conn->dst_proto = AI_TP_GW_PROTO_TCP; }
    if (handshake_code) strncpy(conn->handshake_code, handshake_code, sizeof(conn->handshake_code) - 1);
    conn->state = AI_TP_GW_CONN_ACTIVE;
    ctx->conn_count++;
    ctx->stats.total_connections++;
    ctx->stats.active_connections++;
    if (ctx->on_connection) ctx->on_connection(conn, ctx->user_data);
    return conn_id;
}

int ai_tp_gw_disconnect(ai_tp_gw_context_t *ctx, uint64_t conn_id) {
    if (!ctx) return AI_TP_GW_ERR_INIT;
    int idx = find_conn_index(ctx, conn_id);
    if (idx < 0) return AI_TP_GW_ERR_NOT_FOUND;
    ctx->connections[idx].state = AI_TP_GW_CONN_CLOSED;
    ctx->stats.active_connections--;
    uint32_t last = ctx->conn_count - 1;
    if ((uint32_t)idx != last) ctx->connections[idx] = ctx->connections[last];
    ctx->conn_count--;
    return AI_TP_GW_OK;
}

int64_t ai_tp_gw_forward(ai_tp_gw_context_t *ctx, uint64_t conn_id, const void *data, size_t len, ai_tp_gw_mode_t direction) {
    if (!ctx || !data || len == 0) return AI_TP_GW_ERR_INIT;
    if (!ctx->running) return AI_TP_GW_ERR_IO;
    int idx = find_conn_index(ctx, conn_id);
    if (idx < 0) return AI_TP_GW_ERR_NOT_FOUND;
    ai_tp_gw_connection_t *conn = &ctx->connections[idx];
    if (conn->state != AI_TP_GW_CONN_ACTIVE) return AI_TP_GW_ERR_PROTOCOL;
    if (len > ctx->bandwidth_limit) return AI_TP_GW_ERR_OVERLOAD;
    if (direction == AI_TP_GW_MODE_INBOUND) { conn->bytes_in += len; ctx->stats.total_bytes_in += len; ctx->stats.total_packets_in++; }
    else { conn->bytes_out += len; ctx->stats.total_bytes_out += len; ctx->stats.total_packets_out++; }
    conn->last_activity = 0;
    return (int64_t)len;
}

const ai_tp_gw_connection_t *ai_tp_gw_get_connection(ai_tp_gw_context_t *ctx, uint64_t conn_id) {
    if (!ctx) return NULL;
    int idx = find_conn_index(ctx, conn_id);
    if (idx < 0) return NULL;
    return &ctx->connections[idx];
}

int ai_tp_gw_add_acl(ai_tp_gw_context_t *ctx, const ai_tp_gw_acl_rule_t *rule) {
    if (!ctx || !rule) return AI_TP_GW_ERR_INIT;
    if (ctx->acl_count >= AI_TP_GW_MAX_ACL_RULES) return AI_TP_GW_ERR_NOMEM;
    ctx->acl[ctx->acl_count] = *rule;
    ctx->acl_count++;
    return AI_TP_GW_OK;
}

int ai_tp_gw_remove_acl(ai_tp_gw_context_t *ctx, uint32_t index) {
    if (!ctx) return AI_TP_GW_ERR_INIT;
    if (index >= ctx->acl_count) return AI_TP_GW_ERR_NOT_FOUND;
    uint32_t last = ctx->acl_count - 1;
    if (index != last) ctx->acl[index] = ctx->acl[last];
    ctx->acl_count--;
    return AI_TP_GW_OK;
}

bool ai_tp_gw_check_acl(ai_tp_gw_context_t *ctx, const char *src, const char *dst, ai_tp_gw_protocol_t proto) {
    if (!ctx) return false;
    if (ctx->acl_count == 0) return true;
    int best_priority = -1;
    bool best_action = true;
    for (uint32_t i = 0; i < ctx->acl_count; i++) {
        const ai_tp_gw_acl_rule_t *rule = &ctx->acl[i];
        if (!rule->enabled) continue;
        if (rule->proto != AI_TP_GW_PROTO_ATP && rule->proto != proto) continue;
        if (!pattern_match(rule->src_pattern, src)) continue;
        if (!pattern_match(rule->dst_pattern, dst)) continue;
        if (best_priority < 0 || rule->priority < (uint16_t)best_priority) {
            best_priority = (int)rule->priority;
            best_action = (rule->action == AI_TP_GW_ACL_ALLOW);
        }
    }
    return best_action;
}

void ai_tp_gw_set_resource_ratio(ai_tp_gw_context_t *ctx, uint8_t ratio) {
    if (!ctx) return;
    ctx->resource_ratio = (ratio > 100) ? 100 : ratio;
}

void ai_tp_gw_set_bandwidth_limit(ai_tp_gw_context_t *ctx, uint64_t bytes_per_second) {
    if (!ctx) return;
    ctx->bandwidth_limit = bytes_per_second;
}

const ai_tp_gw_stats_t *ai_tp_gw_get_stats(ai_tp_gw_context_t *ctx) {
    if (!ctx) return NULL;
    return &ctx->stats;
}

const char *ai_tp_gw_proto_name(ai_tp_gw_protocol_t proto) {
    switch (proto) {
        case AI_TP_GW_PROTO_TCP:  return "TCP";
        case AI_TP_GW_PROTO_UDP:  return "UDP";
        case AI_TP_GW_PROTO_ICMP: return "ICMP";
        case AI_TP_GW_PROTO_HTTP: return "HTTP";
        case AI_TP_GW_PROTO_HTTPS:return "HTTPS";
        case AI_TP_GW_PROTO_ATP:  return "ATP";
        default:                  return "UNKNOWN";
    }
}

const char *ai_tp_gw_get_version(void) { return "1.0.0"; }

