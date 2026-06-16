/**
 * @file ai-tp-sync.c
 * @brief AI-TP P2P文件同步模块实现
 * @version 1.0.0
 * @date 2026-06-16
 */

#include "ai-tp-sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ========== 内部辅助函数 ========== */

/**
 * @brief 查找文件索引
 * @param ctx 上下文指针
 * @param file_path 文件路径
 * @return 文件索引，未找到返回-1
 */
static int find_file_index(ai_tp_sync_ctx_t* ctx, const char* file_path) {
    for (uint32_t i = 0; i < ctx->file_count; i++) {
        if (strcmp(ctx->files[i].file_path, file_path) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/**
 * @brief 查找对等节点索引
 * @param ctx 上下文指针
 * @param node_id 节点ID
 * @return 节点索引，未找到返回-1
 */
static int find_peer_index(ai_tp_sync_ctx_t* ctx, const char* node_id) {
    for (uint32_t i = 0; i < ctx->peer_count; i++) {
        if (strcmp(ctx->peers[i].node_id, node_id) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/**
 * @brief 简化的哈希计算（模拟SHA-256）
 * @param data 数据指针
 * @param size 数据大小
 * @param hash 输出：哈希字符串
 * @note 这是简化实现，仅用于演示
 */
static void simple_hash(const void* data, uint64_t size, char* hash) {
    /* 简化实现：使用XOR和加法模拟哈希 */
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t h = 0x6A09E667F3BCC908ULL;  /* 沃尔夫勒姆常数 */
    
    for (uint64_t i = 0; i < size; i++) {
        h = (h * 31 + bytes[i]) ^ (h >> 27);
    }
    
    /* 转换为十六进制字符串 */
    snprintf(hash, AI_TP_HASH_SIZE, "%016lX%016lX%016lX%016lX",
             h, h >> 16, h >> 32, h >> 48);
}

/**
 * @brief 模拟P2P传输（实际中应通过网络发送）
 * @param file_path 文件路径
 * @param chunk_id 块ID
 * @param peer_id 目标节点ID
 * @return 0成功，非0失败
 * @note 这是模拟实现
 */
static int simulate_p2p_transfer(const char* file_path, uint64_t chunk_id, const char* peer_id) {
    printf("AI-TP Sync: Simulating P2P transfer (file='%s', chunk=%lu, peer='%s')\n",
           file_path, chunk_id, peer_id);
    
    /* 模拟传输延迟 */
    /* usleep(1000);  模拟1ms延迟 */
    
    /* 模拟成功率：95% */
    if ((chunk_id % 100) < 95) {
        return 0;  /* 成功 */
    } else {
        return -1;  /* 失败（模拟网络错误）*/
    }
}

/* ========== 核心API实现 ========== */

ai_tp_sync_ctx_t* ai_tp_sync_init(
    uint32_t max_files,
    uint64_t default_chunk_size
) {
    /* 参数检查 */
    if (max_files == 0) {
        fprintf(stderr, "Error: Invalid max_files (%u)\n", max_files);
        return NULL;
    }
    
    /* 分配上下文 */
    ai_tp_sync_ctx_t* ctx = (ai_tp_sync_ctx_t*)malloc(sizeof(ai_tp_sync_ctx_t));
    if (!ctx) {
        fprintf(stderr, "Error: Failed to allocate context\n");
        return NULL;
    }
    
    /* 分配文件数组 */
    ctx->files = (ai_tp_file_info_t*)malloc(sizeof(ai_tp_file_info_t) * max_files);
    if (!ctx->files) {
        fprintf(stderr, "Error: Failed to allocate files array\n");
        free(ctx);
        return NULL;
    }
    
    /* 分配对等节点数组 */
    ctx->peers = (ai_tp_peer_info_t*)malloc(sizeof(ai_tp_peer_info_t) * AI_TP_MAX_PEERS);
    if (!ctx->peers) {
        fprintf(stderr, "Error: Failed to allocate peers array\n");
        free(ctx->files);
        free(ctx);
        return NULL;
    }
    
    /* 初始化上下文 */
    memset(ctx->files, 0, sizeof(ai_tp_file_info_t) * max_files);
    memset(ctx->peers, 0, sizeof(ai_tp_peer_info_t) * AI_TP_MAX_PEERS);
    ctx->file_count = 0;
    ctx->max_files = max_files;
    ctx->peer_count = 0;
    ctx->default_chunk_size = default_chunk_size > 0 ? default_chunk_size : AI_TP_DEFAULT_CHUNK_SIZE;
    ctx->use_compression = false;
    ctx->use_encryption = false;
    ctx->on_chunk_transferred = NULL;
    ctx->on_file_synced = NULL;
    
    printf("AI-TP Sync: Initialized (max_files=%u, default_chunk_size=%lu bytes)\n",
           max_files, ctx->default_chunk_size);
    
    return ctx;
}

void ai_tp_sync_destroy(ai_tp_sync_ctx_t* ctx) {
    if (!ctx) {
        return;
    }
    
    printf("AI-TP Sync: Destroying context (files=%u, peers=%u)\n",
           ctx->file_count, ctx->peer_count);
    
    /* 释放所有文件的块数组 */
    for (uint32_t i = 0; i < ctx->file_count; i++) {
        if (ctx->files[i].chunks) {
            free(ctx->files[i].chunks);
        }
    }
    
    if (ctx->files) {
        free(ctx->files);
    }
    
    if (ctx->peers) {
        free(ctx->peers);
    }
    
    free(ctx);
}

int ai_tp_sync_add_file(
    ai_tp_sync_ctx_t* ctx,
    const char* file_path,
    uint64_t chunk_size
) {
    /* 参数检查 */
    if (!ctx || !file_path) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 检查文件是否已存在 */
    int idx = find_file_index(ctx, file_path);
    if (idx >= 0) {
        fprintf(stderr, "Error: File '%s' already added\n", file_path);
        return -2;
    }
    
    /* 检查是否达到最大文件数 */
    if (ctx->file_count >= ctx->max_files) {
        fprintf(stderr, "Error: Maximum files reached (%u)\n", ctx->max_files);
        return -3;
    }
    
    /* 获取文件信息（简化：这里应调用stat()获取真实信息）*/
    FILE* fp = fopen(file_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", file_path);
        return -4;
    }
    
    /* 获取文件大小 */
    fseek(fp, 0, SEEK_END);
    uint64_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    /* 关闭文件 */
    fclose(fp);
    
    /* 注册新文件 */
    ai_tp_file_info_t* file = &ctx->files[ctx->file_count];
    strncpy(file->file_path, file_path, AI_TP_MAX_FILE_PATH - 1);
    file->file_path[AI_TP_MAX_FILE_PATH - 1] = '\0';
    
    /* 提取文件名 */
    const char* slash = strrchr(file_path, '/');
    if (!slash) slash = strrchr(file_path, '\\');  /* Windows路径 */
    if (slash) {
        strncpy(file->file_name, slash + 1, AI_TP_MAX_FILE_NAME - 1);
    } else {
        strncpy(file->file_name, file_path, AI_TP_MAX_FILE_NAME - 1);
    }
    file->file_name[AI_TP_MAX_FILE_NAME - 1] = '\0';
    
    file->file_size = file_size;
    file->chunk_size = chunk_size > 0 ? chunk_size : ctx->default_chunk_size;
    file->chunk_count = (uint32_t)((file_size + file->chunk_size - 1) / file->chunk_size);
    file->create_time = time(NULL);
    file->modify_time = file->create_time;
    file->is_complete = false;
    
    /* 分配块数组 */
    file->chunks = (ai_tp_file_chunk_t*)malloc(sizeof(ai_tp_file_chunk_t) * file->chunk_count);
    if (!file->chunks) {
        fprintf(stderr, "Error: Failed to allocate chunks array\n");
        return -5;
    }
    
    /* 初始化块信息 */
    for (uint32_t i = 0; i < file->chunk_count; i++) {
        file->chunks[i].chunk_id = i;
        file->chunks[i].offset = i * file->chunk_size;
        file->chunks[i].size = (i < file->chunk_count - 1) ? 
                               file->chunk_size : 
                               (file_size - i * file->chunk_size);
        file->chunks[i].hash[0] = '\0';
        file->chunks[i].is_transferred = false;
        file->chunks[i].last_transfer_time = 0;
    }
    
    ctx->file_count++;
    
    printf("AI-TP Sync: File added (path='%s', size=%lu bytes, chunks=%u)\n",
           file_path, file_size, file->chunk_count);
    
    return 0;
}

int ai_tp_sync_remove_file(
    ai_tp_sync_ctx_t* ctx,
    const char* file_path
) {
    /* 参数检查 */
    if (!ctx || !file_path) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 查找文件 */
    int idx = find_file_index(ctx, file_path);
    if (idx < 0) {
        fprintf(stderr, "Error: File '%s' not found\n", file_path);
        return -2;
    }
    
    printf("AI-TP Sync: File removed (path='%s')\n", file_path);
    
    /* 释放块数组 */
    if (ctx->files[idx].chunks) {
        free(ctx->files[idx].chunks);
    }
    
    /* 移除文件（用最后一个文件覆盖）*/
    if (idx < (int)(ctx->file_count - 1)) {
        memcpy(&ctx->files[idx], &ctx->files[ctx->file_count - 1], sizeof(ai_tp_file_info_t));
    }
    
    ctx->file_count--;
    
    return 0;
}

int ai_tp_sync_calc_file_hash(
    const char* file_path,
    char* hash
) {
    /* 参数检查 */
    if (!file_path || !hash) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 打开文件 */
    FILE* fp = fopen(file_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", file_path);
        return -2;
    }
    
    /* 读取整个文件并计算哈希 */
    fseek(fp, 0, SEEK_END);
    uint64_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    void* data = malloc(file_size);
    if (!data) {
        fclose(fp);
        fprintf(stderr, "Error: Failed to allocate memory\n");
        return -3;
    }
    
    fread(data, 1, file_size, fp);
    fclose(fp);
    
    /* 计算哈希 */
    simple_hash(data, file_size, hash);
    
    free(data);
    
    printf("AI-TP Sync: File hash calculated (path='%s', hash='%s')\n",
           file_path, hash);
    
    return 0;
}

int ai_tp_sync_calc_chunk_hash(
    const void* data,
    uint64_t size,
    char* hash
) {
    /* 参数检查 */
    if (!data || !hash || size == 0) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 计算哈希 */
    simple_hash(data, size, hash);
    
    return 0;
}

int ai_tp_sync_split_file(
    ai_tp_sync_ctx_t* ctx,
    const char* file_path
) {
    /* 参数检查 */
    if (!ctx || !file_path) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 查找文件 */
    int idx = find_file_index(ctx, file_path);
    if (idx < 0) {
        fprintf(stderr, "Error: File '%s' not found\n", file_path);
        return -2;
    }
    
    ai_tp_file_info_t* file = &ctx->files[idx];
    
    /* 打开文件 */
    FILE* fp = fopen(file_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", file_path);
        return -3;
    }
    
    printf("AI-TP Sync: Splitting file '%s' into %u chunks...\n",
           file_path, file->chunk_count);
    
    /* 读取每个块并计算哈希 */
    for (uint32_t i = 0; i < file->chunk_count; i++) {
        void* chunk_data = malloc(file->chunks[i].size);
        if (!chunk_data) {
            fclose(fp);
            fprintf(stderr, "Error: Failed to allocate chunk buffer\n");
            return -4;
        }
        
        /* 读取块数据 */
        fseek(fp, file->chunks[i].offset, SEEK_SET);
        fread(chunk_data, 1, file->chunks[i].size, fp);
        
        /* 计算块哈希 */
        simple_hash(chunk_data, file->chunks[i].size, file->chunks[i].hash);
        
        /* 释放内存 */
        free(chunk_data);
        
        printf("  Chunk %u/%u: offset=%lu, size=%lu, hash=%s\n",
               i + 1, file->chunk_count,
               file->chunks[i].offset,
               file->chunks[i].size,
               file->chunks[i].hash);
    }
    
    fclose(fp);
    
    /* 计算文件整体哈希 */
    ai_tp_sync_calc_file_hash(file_path, file->file_hash);
    
    printf("AI-TP Sync: File split complete (file_hash='%s')\n", file->file_hash);
    
    return 0;
}

int ai_tp_sync_transfer_chunk(
    ai_tp_sync_ctx_t* ctx,
    const char* file_path,
    uint64_t chunk_id,
    const char* peer_id
) {
    /* 参数检查 */
    if (!ctx || !file_path || !peer_id) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 查找文件 */
    int file_idx = find_file_index(ctx, file_path);
    if (file_idx < 0) {
        fprintf(stderr, "Error: File '%s' not found\n", file_path);
        return -2;
    }
    
    /* 检查块ID */
    if (chunk_id >= ctx->files[file_idx].chunk_count) {
        fprintf(stderr, "Error: Invalid chunk_id (%lu)\n", chunk_id);
        return -3;
    }
    
    /* 查找对等节点 */
    int peer_idx = find_peer_index(ctx, peer_id);
    if (peer_idx < 0) {
        fprintf(stderr, "Error: Peer '%s' not found\n", peer_id);
        return -4;
    }
    
    /* 模拟P2P传输 */
    int ret = simulate_p2p_transfer(file_path, chunk_id, peer_id);
    
    if (ret == 0) {
        /* 传输成功 */
        ctx->files[file_idx].chunks[chunk_id].is_transferred = true;
        ctx->files[file_idx].chunks[chunk_id].last_transfer_time = time(NULL);
        
        printf("AI-TP Sync: Chunk transferred (file='%s', chunk=%lu, peer='%s')\n",
               file_path, chunk_id, peer_id);
        
        /* 调用回调 */
        if (ctx->on_chunk_transferred) {
            ctx->on_chunk_transferred(file_path, chunk_id, true);
        }
    } else {
        /* 传输失败 */
        printf("AI-TP Sync: Chunk transfer failed (file='%s', chunk=%lu, peer='%s')\n",
               file_path, chunk_id, peer_id);
        
        /* 调用回调 */
        if (ctx->on_chunk_transferred) {
            ctx->on_chunk_transferred(file_path, chunk_id, false);
        }
    }
    
    return ret;
}

int ai_tp_sync_receive_chunk(
    ai_tp_sync_ctx_t* ctx,
    const char* file_path,
    uint64_t chunk_id,
    const void* data,
    uint64_t size,
    const char* hash
) {
    /* 参数检查 */
    if (!ctx || !file_path || !data || !hash) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 查找文件 */
    int idx = find_file_index(ctx, file_path);
    if (idx < 0) {
        fprintf(stderr, "Error: File '%s' not found\n", file_path);
        return -2;
    }
    
    /* 检查块ID */
    if (chunk_id >= ctx->files[idx].chunk_count) {
        fprintf(stderr, "Error: Invalid chunk_id (%lu)\n", chunk_id);
        return -3;
    }
    
    /* 检查数据大小 */
    if (size != ctx->files[idx].chunks[chunk_id].size) {
        fprintf(stderr, "Error: Chunk size mismatch (expected=%lu, received=%lu)\n",
               ctx->files[idx].chunks[chunk_id].size, size);
        return -4;
    }
    
    /* 校验哈希 */
    char calc_hash[AI_TP_HASH_SIZE];
    simple_hash(data, size, calc_hash);
    
    if (strcmp(calc_hash, hash) != 0) {
        fprintf(stderr, "Error: Hash mismatch (expected='%s', calculated='%s')\n",
                hash, calc_hash);
        return -5;
    }
    
    /* 写入文件（简化：实际应写入临时文件或块存储）*/
    printf("AI-TP Sync: Chunk received and verified (file='%s', chunk=%lu, hash='%s')\n",
           file_path, chunk_id, hash);
    
    /* 标记为已传输 */
    ctx->files[idx].chunks[chunk_id].is_transferred = true;
    ctx->files[idx].chunks[chunk_id].last_transfer_time = time(NULL);
    strncpy(ctx->files[idx].chunks[chunk_id].hash, hash, AI_TP_HASH_SIZE - 1);
    
    return 0;
}

int ai_tp_sync_file(
    ai_tp_sync_ctx_t* ctx,
    const char* file_path,
    const char* peer_id
) {
    /* 参数检查 */
    if (!ctx || !file_path || !peer_id) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 查找文件 */
    int idx = find_file_index(ctx, file_path);
    if (idx < 0) {
        fprintf(stderr, "Error: File '%s' not found\n", file_path);
        return -2;
    }
    
    ai_tp_file_info_t* file = &ctx->files[idx];
    
    printf("AI-TP Sync: Starting sync (file='%s', peer='%s', chunks=%u)\n",
           file_path, peer_id, file->chunk_count);
    
    /* 分块文件（如果还没分块）*/
    if (file->chunks[0].hash[0] == '\0') {
        int ret = ai_tp_sync_split_file(ctx, file_path);
        if (ret != 0) {
            return ret;
        }
    }
    
    /* 传输所有块 */
    uint32_t transferred = 0;
    for (uint32_t i = 0; i < file->chunk_count; i++) {
        if (!file->chunks[i].is_transferred) {
            int ret = ai_tp_sync_transfer_chunk(ctx, file_path, i, peer_id);
            if (ret == 0) {
                transferred++;
            } else {
                printf("AI-TP Sync: Warning: Failed to transfer chunk %u\n", i);
            }
        } else {
            transferred++;
        }
    }
    
    /* 检查是否所有块都传输完成 */
    if (transferred >= file->chunk_count) {
        file->is_complete = true;
        printf("AI-TP Sync: File sync complete (file='%s', transferred=%u/%u)\n",
               file_path, transferred, file->chunk_count);
        
        /* 调用回调 */
        if (ctx->on_file_synced) {
            ctx->on_file_synced(file_path, true);
        }
    } else {
        printf("AI-TP Sync: File sync incomplete (file='%s', transferred=%u/%u)\n",
               file_path, transferred, file->chunk_count);
    }
    
    return 0;
}

int ai_tp_sync_get_progress(
    ai_tp_sync_ctx_t* ctx,
    const char* file_path
) {
    /* 参数检查 */
    if (!ctx || !file_path) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 查找文件 */
    int idx = find_file_index(ctx, file_path);
    if (idx < 0) {
        fprintf(stderr, "Error: File '%s' not found\n", file_path);
        return -2;
    }
    
    ai_tp_file_info_t* file = &ctx->files[idx];
    
    /* 计算进度 */
    uint32_t transferred = 0;
    for (uint32_t i = 0; i < file->chunk_count; i++) {
        if (file->chunks[i].is_transferred) {
            transferred++;
        }
    }
    
    int progress = (int)(transferred * 100 / file->chunk_count);
    
    return progress;
}

int ai_tp_sync_resume(
    ai_tp_sync_ctx_t* ctx,
    const char* file_path,
    const char* peer_id
) {
    /* 参数检查 */
    if (!ctx || !file_path || !peer_id) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 查找文件 */
    int idx = find_file_index(ctx, file_path);
    if (idx < 0) {
        fprintf(stderr, "Error: File '%s' not found\n", file_path);
        return -2;
    }
    
    ai_tp_file_info_t* file = &ctx->files[idx];
    
    printf("AI-TP Sync: Resuming sync (file='%s', peer='%s')\n",
           file_path, peer_id);
    
    /* 继续传输未完成的块 */
    return ai_tp_sync_file(ctx, file_path, peer_id);
}

int ai_tp_sync_add_peer(
    ai_tp_sync_ctx_t* ctx,
    const char* node_id,
    const char* node_addr,
    uint32_t capabilities
) {
    /* 参数检查 */
    if (!ctx || !node_id || !node_addr) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 检查节点是否已存在 */
    int idx = find_peer_index(ctx, node_id);
    if (idx >= 0) {
        fprintf(stderr, "Error: Peer '%s' already added\n", node_id);
        return -2;
    }
    
    /* 检查是否达到最大节点数 */
    if (ctx->peer_count >= AI_TP_MAX_PEERS) {
        fprintf(stderr, "Error: Maximum peers reached (%u)\n", AI_TP_MAX_PEERS);
        return -3;
    }
    
    /* 添加新节点 */
    ai_tp_peer_info_t* peer = &ctx->peers[ctx->peer_count];
    strncpy(peer->node_id, node_id, 63);
    peer->node_id[63] = '\0';
    strncpy(peer->node_addr, node_addr, 127);
    peer->node_addr[127] = '\0';
    peer->capabilities = capabilities;
    peer->is_connected = true;
    peer->last_seen = time(NULL);
    
    ctx->peer_count++;
    
    printf("AI-TP Sync: Peer added (id='%s', addr='%s', caps=0x%08X)\n",
           node_id, node_addr, capabilities);
    
    return 0;
}

int ai_tp_sync_remove_peer(
    ai_tp_sync_ctx_t* ctx,
    const char* node_id
) {
    /* 参数检查 */
    if (!ctx || !node_id) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    /* 查找节点 */
    int idx = find_peer_index(ctx, node_id);
    if (idx < 0) {
        fprintf(stderr, "Error: Peer '%s' not found\n", node_id);
        return -2;
    }
    
    printf("AI-TP Sync: Peer removed (id='%s', addr='%s')\n",
           ctx->peers[idx].node_id, ctx->peers[idx].node_addr);
    
    /* 移除节点（用最后一个节点覆盖）*/
    if (idx < (int)(ctx->peer_count - 1)) {
        memcpy(&ctx->peers[idx], &ctx->peers[ctx->peer_count - 1], sizeof(ai_tp_peer_info_t));
    }
    
    ctx->peer_count--;
    
    return 0;
}

const char* ai_tp_sync_get_version(void) {
    static char version[32];
    snprintf(version, sizeof(version), "%d.%d.%d",
             AI_TP_SYNC_VERSION_MAJOR,
             AI_TP_SYNC_VERSION_MINOR,
             AI_TP_SYNC_VERSION_PATCH);
    return version;
}
