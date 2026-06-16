/**
 * @file ai-storage.h
 * @brief AI-TP OS 存储抽象层 API
 * 
 * 提供统一的存储接口，支持本地存储、P2P 存储、云存储
 */

#ifndef AI_STORAGE_H
#define AI_STORAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 错误码 ============ */

#define AI_STORAGE_OK             0
#define AI_STORAGE_ERR_NOT_FOUND -1
#define AI_STORAGE_ERR_EXISTS    -2
#define AI_STORAGE_ERR_IO        -3
#define AI_STORAGE_ERR_NOMEM    -4
#define AI_STORAGE_ERR_NETWORK   -5

/* ============ 存储后端类型 ============ */

typedef enum {
    AI_STORAGE_LOCAL = 0,  /* 本地存储 */
    AI_STORAGE_P2P  = 1,  /* P2P 存储 */
    AI_STORAGE_CLOUD = 2,  /* 云存储 */
    AI_STORAGE_AUTO  = 3    /* 自动选择 */
} ai_storage_backend_t;

/* ============ 存储句柄 ============ */

typedef struct ai_storage_ctx ai_storage_ctx_t;

/* ============ 核心 API ============ */

/**
 * @brief 初始化存储系统
 * 
 * @param backend 存储后端类型
 * @param config  后端配置（JSON 字符串）
 * @return 存储上下文，失败返回 NULL
 */
ai_storage_ctx_t* ai_storage_init(ai_storage_backend_t backend, const char* config);

/**
 * @brief 销毁存储上下文
 * 
 * @param ctx 存储上下文
 */
void ai_storage_destroy(ai_storage_ctx_t* ctx);

/**
 * @brief 存储对象
 * 
 * @param ctx  存储上下文
 * @param key  对象键（唯一标识）
 * @param data 数据指针
 * @param len  数据长度
 * @return AI_STORAGE_OK 成功，其他值失败
 */
int ai_storage_put(ai_storage_ctx_t* ctx, const char* key, const void* data, size_t len);

/**
 * @brief 读取对象
 * 
 * @param ctx     存储上下文
 * @param key     对象键
 * @param data    输出：数据指针（需要调用者释放）
 * @param len     输出：数据长度
 * @return AI_STORAGE_OK 成功，其他值失败
 */
int ai_storage_get(ai_storage_ctx_t* ctx, const char* key, void** data, size_t* len);

/**
 * @brief 删除对象
 * 
 * @param ctx 存储上下文
 * @param key 对象键
 * @return AI_STORAGE_OK 成功，其他值失败
 */
int ai_storage_delete(ai_storage_ctx_t* ctx, const char* key);

/**
 * @brief 检查对象是否存在
 * 
 * @param ctx 存储上下文
 * @param key 对象键
 * @return 1 存在，0 不存在，-1 错误
 */
int ai_storage_exists(ai_storage_ctx_t* ctx, const char* key);

/**
 * @brief 列出对象（支持前缀匹配）
 * 
 * @param ctx     存储上下文
 * @param prefix  键前缀（NULL 表示列出所有）
 * @param keys    输出：键列表（需要调用者释放）
 * @param count   输出：键数量
 * @return AI_STORAGE_OK 成功，其他值失败
 */
int ai_storage_list(ai_storage_ctx_t* ctx, const char* prefix, char*** keys, size_t* count);

/**
 * @brief 获取对象元数据
 * 
 * @param ctx     存储上下文
 * @param key     对象键
 * @param size    输出：对象大小
 * @param mtime   输出：最后修改时间（Unix 时间戳）
 * @return AI_STORAGE_OK 成功，其他值失败
 */
int ai_storage_stat(ai_storage_ctx_t* ctx, const char* key, size_t* size, time_t* mtime);

/* ============ 高级 API ============ */

/**
 * @brief 批量存储对象
 * 
 * @param ctx   存储上下文
 * @param keys  键数组
 * @param data  数据指针数组
 * @param lens  数据长度数组
 * @param count 对象数量
 * @return AI_STORAGE_OK 全部成功，其他值失败
 */
int ai_storage_put_batch(ai_storage_ctx_t* ctx, const char** keys, 
                        const void** data, size_t* lens, size_t count);

/**
 * @brief 复制对象（本地 → P2P 或反之）
 * 
 * @param ctx_src  源存储上下文
 * @param ctx_dst  目标存储上下文
 * @param key      对象键
 * @return AI_STORAGE_OK 成功，其他值失败
 */
int ai_storage_copy(ai_storage_ctx_t* ctx_src, ai_storage_ctx_t* ctx_dst, const char* key);

/**
 * @brief 同步存储（本地 ↔ P2P）
 * 
 * @param ctx_local 本地存储上下文
 * @param ctx_p2p  P2P 存储上下文
 * @return AI_STORAGE_OK 成功，其他值失败
 */
int ai_storage_sync(ai_storage_ctx_t* ctx_local, ai_storage_ctx_t* ctx_p2p);

#ifdef __cplusplus
}
#endif

#endif /* AI_STORAGE_H */
