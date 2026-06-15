/**
 * @file ai-tp-discovery.h
 * @brief AI-TP 节点发现模块头文件
 * @version 1.0.0
 * @date 2026-06-16
 * 
 * 功能：
 * 1. 节点注册与注销
 * 2. 节点心跳保活
 * 3. 节点查找与发现
 * 4. 节点淘汰机制
 */

#ifndef AI_TP_DISCOVERY_H
#define AI_TP_DISCOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#define AI_TP_DISCOVERY_VERSION_MAJOR 1
#define AI_TP_DISCOVERY_VERSION_MINOR 0
#define AI_TP_DISCOVERY_VERSION_PATCH 0

#define AI_TP_MAX_NODE_ID_LEN 64
#define AI_TP_MAX_NODE_ADDR_LEN 128
#define AI_TP_MAX_NODE_CAPS_LEN 256
#define AI_TP_DEFAULT_HEARTBEAT_INTERVAL 30  /* 心跳间隔（秒）*/
#define AI_TP_DEFAULT_NODE_TIMEOUT 90         /* 节点超时时间（秒）*/
#define AI_TP_MAX_NODES 10000                /* 最大节点数 */

/* ========== 数据结构 ========== */

/**
 * @brief 节点能力标志
 */
typedef enum {
    AI_TP_CAP_NONE = 0,
    AI_TP_CAP_STORAGE = (1 << 0),   /* 存储能力 */
    AI_TP_CAP_COMPUTE = (1 << 1),   /* 计算能力 */
    AI_TP_CAP_RELAY = (1 << 2),     /* 中继能力 */
    AI_TP_CAP_GATEWAY = (1 << 3),   /* 网关能力 */
    AI_TP_CAP_ALL = 0xFFFFFFFF       /* 所有能力 */
} ai_tp_node_capability_t;

/**
 * @brief 节点状态
 */
typedef enum {
    AI_TP_NODE_STATUS_OFFLINE = 0,  /* 离线 */
    AI_TP_NODE_STATUS_ONLINE = 1,   /* 在线 */
    AI_TP_NODE_STATUS_SUSPICIOUS = 2 /* 可疑（心跳异常）*/
} ai_tp_node_status_t;

/**
 * @brief 节点信息结构
 */
typedef struct {
    char id[AI_TP_MAX_NODE_ID_LEN];           /* 节点ID */
    char addr[AI_TP_MAX_NODE_ADDR_LEN];       /* 节点地址（IP:端口）*/
    uint32_t capabilities;                     /* 节点能力（位掩码）*/
    ai_tp_node_status_t status;               /* 节点状态 */
    time_t last_heartbeat;                    /* 最后一次心跳时间 */
    time_t register_time;                     /* 注册时间 */
    uint64_t cpu_freq;                        /* CPU频率（MHz）*/
    uint64_t mem_total;                       /* 总内存（字节）*/
    uint64_t storage_total;                   /* 总存储（字节）*/
    uint8_t resource_ratio;                   /* 资源分配比例（0-100）*/
} ai_tp_node_info_t;

/**
 * @brief 节点发现上下文
 */
typedef struct {
    ai_tp_node_info_t* nodes;                /* 节点数组 */
    uint32_t node_count;                      /* 节点数量 */
    uint32_t max_nodes;                       /* 最大节点数 */
    uint32_t heartbeat_interval;               /* 心跳间隔（秒）*/
    uint32_t node_timeout;                     /* 节点超时时间（秒）*/
    bool use_centralized;                      /* 是否使用中心化注册表 */
    char central_registry[AI_TP_MAX_NODE_ADDR_LEN]; /* 中心化注册表地址 */
} ai_tp_discovery_ctx_t;

/* ========== 核心API ========== */

/**
 * @brief 初始化节点发现上下文
 * @param max_nodes 最大节点数
 * @param heartbeat_interval 心跳间隔（秒）
 * @param node_timeout 节点超时时间（秒）
 * @return 初始化后的上下文，失败返回NULL
 */
ai_tp_discovery_ctx_t* ai_tp_discovery_init(
    uint32_t max_nodes,
    uint32_t heartbeat_interval,
    uint32_t node_timeout
);

/**
 * @brief 销毁节点发现上下文
 * @param ctx 上下文指针
 */
void ai_tp_discovery_destroy(ai_tp_discovery_ctx_t* ctx);

/**
 * @brief 注册节点
 * @param ctx 上下文指针
 * @param node_id 节点ID
 * @param node_addr 节点地址
 * @param capabilities 节点能力
 * @return 0成功，非0失败
 */
int ai_tp_discovery_register(
    ai_tp_discovery_ctx_t* ctx,
    const char* node_id,
    const char* node_addr,
    uint32_t capabilities
);

/**
 * @brief 注销节点
 * @param ctx 上下文指针
 * @param node_id 节点ID
 * @return 0成功，非0失败
 */
int ai_tp_discovery_unregister(
    ai_tp_discovery_ctx_t* ctx,
    const char* node_id
);

/**
 * @brief 发送心跳
 * @param ctx 上下文指针
 * @param node_id 节点ID
 * @return 0成功，非0失败
 */
int ai_tp_discovery_heartbeat(
    ai_tp_discovery_ctx_t* ctx,
    const char* node_id
);

/**
 * @brief 查找节点（按ID）
 * @param ctx 上下文指针
 * @param node_id 节点ID
 * @param node_info 输出：节点信息
 * @return 0成功，非0失败
 */
int ai_tp_discovery_find_by_id(
    ai_tp_discovery_ctx_t* ctx,
    const char* node_id,
    ai_tp_node_info_t* node_info
);

/**
 * @brief 查找节点（按能力）
 * @param ctx 上下文指针
 * @param capabilities 所需能力（位掩码）
 * @param nodes 输出：匹配的节点数组
 * @param max_nodes 最大返回节点数
 * @return 匹配的节点数，失败返回-1
 */
int ai_tp_discovery_find_by_capability(
    ai_tp_discovery_ctx_t* ctx,
    uint32_t capabilities,
    ai_tp_node_info_t* nodes,
    uint32_t max_nodes
);

/**
 * @brief 获取所有在线节点
 * @param ctx 上下文指针
 * @param nodes 输出：节点数组
 * @param max_nodes 最大返回节点数
 * @return 在线节点数，失败返回-1
 */
int ai_tp_discovery_get_online_nodes(
    ai_tp_discovery_ctx_t* ctx,
    ai_tp_node_info_t* nodes,
    uint32_t max_nodes
);

/**
 * @brief 淘汰超时节点
 * @param ctx 上下文指针
 * @return 淘汰的节点数
 */
int ai_tp_discovery_evict_timeout_nodes(
    ai_tp_discovery_ctx_t* ctx
);

/**
 * @brief 设置中心化注册表（可选）
 * @param ctx 上下文指针
 * @param registry_addr 注册表地址（IP:端口）
 * @return 0成功，非0失败
 */
int ai_tp_discovery_set_central_registry(
    ai_tp_discovery_ctx_t* ctx,
    const char* registry_addr
);

/**
 * @brief 获取节点发现模块版本
 * @return 版本字符串
 */
const char* ai_tp_discovery_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_DISCOVERY_H */
