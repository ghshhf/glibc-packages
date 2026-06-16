/**
 * @file ai-tp-nat.h
 * @brief AI-TP OS NAT 穿透与节点组网 API
 * 
 * 功能：
 * 1. NAT 类型检测（基于 STUN）
 * 2. NAT 穿透（优化传统 ICE）
 * 3. AI 端口预测（对称 NAT）
 * 4. 去中心化中继池
 * 5. 节点组网（验证码/提示词握手）
 */

#ifndef AI_TP_NAT_H
#define AI_TP_NAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 错误码 ============ */

#define AI_TP_NAT_OK             0
#define AI_TP_NAT_ERR_IO        -1
#define AI_TP_NAT_ERR_TIMEOUT   -2
#define AI_TP_NAT_ERR_NOMEM    -3
#define AI_TP_NAT_ERR_NO_ROUTE -4

/* ============ NAT 类型 ============ */

typedef enum {
    AI_TP_NAT_FULL_CONE = 0,     /* 全锥型 NAT */
    AI_TP_NAT_RESTRICTED_CONE = 1, /* 限制锥型 NAT */
    AI_TP_NAT_PORT_RESTRICTED = 2, /* 端口限制锥型 NAT */
    AI_TP_NAT_SYMMETRIC = 3,      /* 对称 NAT */
    AI_TP_NAT_UNKNOWN = 4           /* 未知 */
} ai_tp_nat_type_t;

/* ============ 地址结构 ============ */

typedef struct {
    char ip[64];
    unsigned short port;
} ai_tp_addr_t;

/* ============ NAT 上下文 ============ */

typedef struct ai_tp_nat_ctx ai_tp_nat_ctx_t;

/* ============ 核心 API：NAT 检测与穿透 ============ */

/**
 * @brief 初始化 NAT 穿透上下文
 * 
 * @param stun_server STUN 服务器地址（NULL 使用默认）
 * @return NAT 上下文，失败返回 NULL
 */
ai_tp_nat_ctx_t* ai_tp_nat_init(const char* stun_server);

/**
 * @brief 销毁 NAT 穿透上下文
 * 
 * @param ctx NAT 上下文
 */
void ai_tp_nat_destroy(ai_tp_nat_ctx_t* ctx);

/**
 * @brief 检测 NAT 类型
 * 
 * @param ctx NAT 上下文
 * @return NAT 类型
 */
ai_tp_nat_type_t ai_tp_nat_detect_type(ai_tp_nat_ctx_t* ctx);

/**
 * @brief 获取公网地址（通过 STUN）
 * 
 * @param ctx NAT 上下文
 * @param public_addr 输出：公网地址
 * @return AI_TP_NAT_OK 成功，其他值失败
 */
int ai_tp_nat_get_public_addr(ai_tp_nat_ctx_t* ctx, ai_tp_addr_t* public_addr);

/**
 * @brief 执行 NAT 穿透（建立 P2P 连接）
 * 
 * @param ctx NAT 上下文
 * @param peer_addr 对等节点地址
 * @param local_port 本地端口
 * @return AI_TP_NAT_OK 成功，其他值失败
 */
int ai_tp_nat_punch(ai_tp_nat_ctx_t* ctx, const ai_tp_addr_t* peer_addr, unsigned short local_port);

/* ============ AI 端口预测 ============ */

/**
 * @brief 记录 NAT 映射历史（用于 AI 预测）
 * 
 * @param ctx NAT 上下文
 * @param private_port 内网端口
 * @param public_port 公网端口
 * @param timestamp 时间戳
 */
void ai_tp_nat_record_mapping(ai_tp_nat_ctx_t* ctx, unsigned short private_port, 
                              unsigned short public_port, time_t timestamp);

/**
 * @brief 预测下一次公网端口（AI 预测）
 * 
 * @param ctx NAT 上下文
 * @param private_port 内网端口
 * @return 预测的公网端口，0 表示无法预测
 */
unsigned short ai_tp_nat_predict_port(ai_tp_nat_ctx_t* ctx, unsigned short private_port);

/* ============ 去中心化中继池 ============ */

/**
 * @brief 注册为中继节点
 * 
 * @param ctx NAT 上下文
 * @return AI_TP_NAT_OK 成功，其他值失败
 */
int ai_tp_nat_register_relay(ai_tp_nat_ctx_t* ctx);

/**
 * @brief 获取中继节点列表
 * 
 * @param ctx NAT 上下文
 * @param relays 输出：中继节点地址列表
 * @param count 输出：中继节点数量
 * @return AI_TP_NAT_OK 成功，其他值失败
 */
int ai_tp_nat_get_relays(ai_tp_nat_ctx_t* ctx, ai_tp_addr_t** relays, size_t* count);

/* ============ 节点组网：验证码握手 ============ */

/**
 * @brief 生成组网验证码（提示词）
 * 
 * 验证码既是数字，也是字母，本质上是指令
 * 形成底层握手，永续存在
 * 
 * @param ctx NAT 上下文
 * @param code 输出：验证码（至少 32 字节）
 * @return AI_TP_NAT_OK 成功，其他值失败
 */
int ai_tp_nat_generate_invite_code(ai_tp_nat_ctx_t* ctx, char* code);

/**
 * @brief 通过验证码加入网络
 * 
 * 节点启用后，只要能接收互联网消息或完成握手协议
 * 就默认提供算力（20% 提供算力，80% 自用）
 * 组网同样逻辑：20% 资源用于组网，80% 自用
 * 
 * @param ctx NAT 上下文
 * @param code 验证码（提示词）
 * @param resource_ratio 资源提供比例（0.0-1.0，默认 0.2）
 * @return AI_TP_NAT_OK 成功，其他值失败
 */
int ai_tp_nat_join_network(ai_tp_nat_ctx_t* ctx, const char* code, double resource_ratio);

/**
 * @brief 设置资源分配比例
 * 
 * @param compute_ratio 算力提供比例（默认 0.2）
 * @param network_ratio 组网资源比例（默认 0.2）
 */
void ai_tp_nat_set_resource_ratio(double compute_ratio, double network_ratio);

/* ============ 与传统互联网交互 ============ */

/**
 * @brief 初始化网关模式（与传统互联网互通）
 * 
 * @param ctx NAT 上下文
 * @param gateway_addr 网关地址
 * @return AI_TP_NAT_OK 成功，其他值失败
 */
int ai_tp_nat_init_gateway(ai_tp_nat_ctx_t* ctx, const ai_tp_addr_t* gateway_addr);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_NAT_H */
