/**
 * @file ai-tp-sync.h
 * @brief AI-TP P2P文件同步模块头文件
 * @version 1.0.0
 * @date 2026-06-16
 */

#ifndef AI_TP_SYNC_H
#define AI_TP_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define AI_TP_SYNC_VERSION_MAJOR 1
#define AI_TP_SYNC_VERSION_MINOR 0
#define AI_TP_SYNC_VERSION_PATCH 0

#define AI_TP_MAX_FILE_PATH 512
#define AI_TP_MAX_FILE_NAME 256
#define AI_TP_DEFAULT_CHUNK_SIZE (1024 * 1024)
#define AI_TP_MAX_CHUNKS 10000
#define AI_TP_HASH_SIZE 64
#define AI_TP_MAX_PEERS 100

typedef struct {
    uint64_t chunk_id;
    uint64_t offset;
    uint64_t size;
    char hash[AI_TP_HASH_SIZE];
    bool is_transferred;
    time_t last_transfer_time;
} ai_tp_file_chunk_t;

typedef struct {
    char file_path[AI_TP_MAX_FILE_PATH];
    char file_name[AI_TP_MAX_FILE_NAME];
    uint64_t file_size;
    char file_hash[AI_TP_HASH_SIZE];
    uint64_t chunk_size;
    uint32_t chunk_count;
    ai_tp_file_chunk_t* chunks;
    time_t create_time;
    time_t modify_time;
    bool is_complete;
} ai_tp_file_info_t;

typedef struct {
    char node_id[64];
    char node_addr[128];
    uint32_t capabilities;
    bool is_connected;
    time_t last_seen;
} ai_tp_peer_info_t;

typedef struct {
    ai_tp_file_info_t* files;
    uint32_t file_count;
    uint32_t max_files;
    ai_tp_peer_info_t* peers;
    uint32_t peer_count;
    uint64_t default_chunk_size;
    bool use_compression;
    bool use_encryption;
    void (*on_chunk_transferred)(const char* file_path, uint64_t chunk_id, bool success);
    void (*on_file_synced)(const char* file_path, bool success);
} ai_tp_sync_ctx_t;

ai_tp_sync_ctx_t* ai_tp_sync_init(uint32_t max_files, uint64_t default_chunk_size);
void ai_tp_sync_destroy(ai_tp_sync_ctx_t* ctx);
int ai_tp_sync_add_file(ai_tp_sync_ctx_t* ctx, const char* file_path, uint64_t chunk_size);
int ai_tp_sync_remove_file(ai_tp_sync_ctx_t* ctx, const char* file_path);
int ai_tp_sync_split_file(ai_tp_sync_ctx_t* ctx, const char* file_path);
int ai_tp_sync_transfer_chunk(ai_tp_sync_ctx_t* ctx, const char* file_path, uint64_t chunk_id, const char* peer_id);
int ai_tp_sync_sync_file(ai_tp_sync_ctx_t* ctx, const char* file_path, const char* peer_id);
int ai_tp_sync_get_progress(ai_tp_sync_ctx_t* ctx, const char* file_path);
int ai_tp_sync_resume(ai_tp_sync_ctx_t* ctx, const char* file_path, const char* peer_id);
int ai_tp_sync_add_peer(ai_tp_sync_ctx_t* ctx, const char* node_id, const char* node_addr, uint32_t capabilities);
int ai_tp_sync_remove_peer(ai_tp_sync_ctx_t* ctx, const char* node_id);
const char* ai_tp_sync_get_version(void);

#endif /* AI_TP_SYNC_H */
