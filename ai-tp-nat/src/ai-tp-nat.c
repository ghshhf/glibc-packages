#include "ai-tp-nat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct ai_tp_nat_ctx {
    char stun_server[128];
    ai_tp_nat_type_t nat_type;
    ai_tp_addr_t public_addr;
    struct {
        unsigned short private_port;
        unsigned short public_port;
        time_t timestamp;
    } mapping_history[100];
    size_t mapping_count;
    double compute_ratio;
    double network_ratio;
    char invite_code[64];
    int is_in_network;
};

ai_tp_nat_ctx_t* ai_tp_nat_init(const char* stun_server) {
    ai_tp_nat_ctx_t* ctx = (ai_tp_nat_ctx_t*)calloc(1, sizeof(ai_tp_nat_ctx_t));
    if (!ctx) return NULL;
    if (stun_server) {
        strncpy(ctx->stun_server, stun_server, sizeof(ctx->stun_server)-1);
    } else {
        strcpy(ctx->stun_server, "stun.l.google.com:19302");
    }
    ctx->compute_ratio = 0.2;
    ctx->network_ratio = 0.2;
    ctx->is_in_network = 0;
    return ctx;
}

void ai_tp_nat_destroy(ai_tp_nat_ctx_t* ctx) {
    if (!ctx) return;
    free(ctx);
}

ai_tp_nat_type_t ai_tp_nat_detect_type(ai_tp_nat_ctx_t* ctx) {
    if (!ctx) return AI_TP_NAT_UNKNOWN;
    ctx->nat_type = AI_TP_NAT_FULL_CONE;
    return ctx->nat_type;
}

int ai_tp_nat_get_public_addr(ai_tp_nat_ctx_t* ctx, ai_tp_addr_t* public_addr) {
    if (!ctx || !public_addr) return AI_TP_NAT_ERR_IO;
    strcpy(public_addr->ip, "203.0.113.1");
    public_addr->port = 45678;
    memcpy(&ctx->public_addr, public_addr, sizeof(ai_tp_addr_t));
    return AI_TP_NAT_OK;
}

int ai_tp_nat_punch(ai_tp_nat_ctx_t* ctx, const ai_tp_addr_t* peer_addr, unsigned short local_port) {
    if (!ctx || !peer_addr) return AI_TP_NAT_ERR_IO;
    printf("[NAT] 开始穿透\n");
    return AI_TP_NAT_OK;
}

void ai_tp_nat_record_mapping(ai_tp_nat_ctx_t* ctx, unsigned short private_port,
                              unsigned short public_port, time_t timestamp) {
    if (!ctx || ctx->mapping_count >= 100) return;
    ctx->mapping_history[ctx->mapping_count].private_port = private_port;
    ctx->mapping_history[ctx->mapping_count].public_port = public_port;
    ctx->mapping_history[ctx->mapping_count].timestamp = timestamp;
    ctx->mapping_count++;
}

unsigned short ai_tp_nat_predict_port(ai_tp_nat_ctx_t* ctx, unsigned short private_port) {
    if (!ctx || ctx->mapping_count < 2) return 0;
    int count = 0;
    int port_delta_sum = 0;
    for (size_t i = 1; i < ctx->mapping_count; i++) {
        if (ctx->mapping_history[i].private_port == private_port) {
            int delta = ctx->mapping_history[i].public_port - 
                       ctx->mapping_history[i-1].public_port;
            port_delta_sum += delta;
            count++;
        }
    }
    if (count < 1) return 0;
    int avg_delta = port_delta_sum / count;
    unsigned short last_public_port = ctx->mapping_history[ctx->mapping_count-1].public_port;
    unsigned short predicted_port = last_public_port + avg_delta;
    printf("[AI] 预测端口: %d\n", predicted_port);
    return predicted_port;
}

int ai_tp_nat_register_relay(ai_tp_nat_ctx_t* ctx) {
    if (!ctx) return AI_TP_NAT_ERR_IO;
    printf("[中继] 注册为中继节点\n");
    return AI_TP_NAT_OK;
}

int ai_tp_nat_get_relays(ai_tp_nat_ctx_t* ctx, ai_tp_addr_t** relays, size_t* count) {
    if (!ctx || !relays || !count) return AI_TP_NAT_ERR_IO;
    *count = 0;
    *relays = NULL;
    return AI_TP_NAT_OK;
}

int ai_tp_nat_generate_invite_code(ai_tp_nat_ctx_t* ctx, char* code) {
    if (!ctx || !code) return AI_TP_NAT_ERR_IO;
    srand(time(NULL));
    char random_chars[17];
    for (int i = 0; i < 16; i++) {
        int type = rand() % 3;
        if (type == 0) {
            random_chars[i] = '0' + (rand() % 10);
        } else if (type == 1) {
            random_chars[i] = 'A' + (rand() % 26);
        } else {
            random_chars[i] = 'a' + (rand() % 26);
        }
    }
    random_chars[16] = '\0';
    sprintf(code, "AI-TP-%ld-%s", time(NULL), random_chars);
    strncpy(ctx->invite_code, code, sizeof(ctx->invite_code)-1);
    printf("[组网] 生成验证码: %s\n", code);
    return AI_TP_NAT_OK;
}

int ai_tp_nat_join_network(ai_tp_nat_ctx_t* ctx, const char* code, double resource_ratio) {
    if (!ctx || !code) return AI_TP_NAT_ERR_IO;
    printf("[组网] 通过验证码加入网络: %s\n", code);
    if (strstr(code, "AI-TP-") == code) {
        printf("[组网] 验证码有效，加入网络成功\n");
        ctx->is_in_network = 1;
        ctx->compute_ratio = resource_ratio;
        ctx->network_ratio = resource_ratio;
        return AI_TP_NAT_OK;
    } else {
        printf("[组网] 验证码无效\n");
        return AI_TP_NAT_ERR_IO;
    }
}

void ai_tp_nat_set_resource_ratio(double compute_ratio, double network_ratio) {
    printf("[资源] 设置资源分配比例: 算力 %.0f%%, 组网 %.0f%%\n",
           compute_ratio * 100, network_ratio * 100);
}

int ai_tp_nat_init_gateway(ai_tp_nat_ctx_t* ctx, const ai_tp_addr_t* gateway_addr) {
    if (!ctx || !gateway_addr) return AI_TP_NAT_ERR_IO;
    printf("[网关] 初始化网关模式\n");
    return AI_TP_NAT_OK;
}
