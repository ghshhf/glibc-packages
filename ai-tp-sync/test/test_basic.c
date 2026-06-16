/**
 * @file test_basic.c
 * @brief ai-tp-sync 基础功能测试
 * @version 1.0.0
 * @date 2026-06-16
 */

#include "ai-tp-sync.h"
#include <stdio.h>
#include <stdlib.h>

/* ========== 回调函数 ========== */

void on_chunk_transferred(const char* file_path, uint64_t chunk_id, bool success) {
    printf("[Callback] Chunk transferred: file='%s', chunk=%lu, success=%s\n",
           file_path, chunk_id, success ? "true" : "false");
}

void on_file_synced(const char* file_path, bool success) {
    printf("[Callback] File synced: file='%s', success=%s\n",
           file_path, success ? "true" : "false");
}

/* ========== 测试函数 ========== */

int test_init_and_destroy(void) {
    printf("\n=== 测试1: 初始化和销毁 ===\n");
    
    /* 初始化 */
    ai_tp_sync_ctx_t* ctx = ai_tp_sync_init(100, 0);
    if (!ctx) {
        printf("FAIL: Failed to init\n");
        return -1;
    }
    printf("PASS: Init successful\n");
    printf("  max_files=%u\n", ctx->max_files);
    printf("  default_chunk_size=%lu\n", ctx->default_chunk_size);
    
    /* 销毁 */
    ai_tp_sync_destroy(ctx);
    printf("PASS: Destroy successful\n");
    
    return 0;
}

int test_add_remove_peer(ai_tp_sync_ctx_t* ctx) {
    printf("\n=== 测试2: 添加/移除对等节点 ===\n");
    
    /* 添加节点 */
    int ret = ai_tp_sync_add_peer(ctx, "node-001", "192.168.1.101:8080", AI_TP_CAP_STORAGE);
    if (ret != 0) {
        printf("FAIL: Failed to add peer (ret=%d)\n", ret);
        return -1;
    }
    printf("PASS: Peer 'node-001' added\n");
    
    ret = ai_tp_sync_add_peer(ctx, "node-002", "192.168.1.102:8080", AI_TP_CAP_COMPUTE);
    if (ret != 0) {
        printf("FAIL: Failed to add peer (ret=%d)\n", ret);
        return -1;
    }
    printf("PASS: Peer 'node-002' added\n");
    
    /* 尝试添加重复节点 */
    ret = ai_tp_sync_add_peer(ctx, "node-001", "192.168.1.101:8080", AI_TP_CAP_STORAGE);
    if (ret == 0) {
        printf("FAIL: Should fail to add duplicate peer\n");
        return -1;
    }
    printf("PASS: Duplicate peer rejected\n");
    
    /* 移除节点 */
    ret = ai_tp_sync_remove_peer(ctx, "node-001");
    if (ret != 0) {
        printf("FAIL: Failed to remove peer (ret=%d)\n", ret);
        return -1;
    }
    printf("PASS: Peer 'node-001' removed\n");
    
    return 0;
}

int test_add_remove_file(ai_tp_sync_ctx_t* ctx) {
    printf("\n=== 测试3: 添加/移除文件 ===\n");
    
    /* 创建测试文件 */
    FILE* fp = fopen("test_file.dat", "wb");
    if (fp) {
        /* 写入5MB测试数据 */
        void* data = malloc(1024);
        for (int i = 0; i < 1024; i++) {
            ((uint8_t*)data)[i] = (uint8_t)(i % 256);
        }
        for (int i = 0; i < 5120; i++) {  /* 5MB = 5120 * 1024 */
            fwrite(data, 1, 1024, fp);
        }
        free(data);
        fclose(fp);
        printf("PASS: Test file created (5MB)\n");
    } else {
        printf("FAIL: Cannot create test file\n");
        return -1;
    }
    
    /* 添加文件 */
    int ret = ai_tp_sync_add_file(ctx, "test_file.dat", 1024 * 1024);  /* 1MB块 */
    if (ret != 0) {
        printf("FAIL: Failed to add file (ret=%d)\n", ret);
        return -1;
    }
    printf("PASS: File added\n");
    printf("  chunk_size=%lu bytes\n", ctx->files[0].chunk_size);
    printf("  chunk_count=%u\n", ctx->files[0].chunk_count);
    
    /* 尝试添加重复文件 */
    ret = ai_tp_sync_add_file(ctx, "test_file.dat", 1024 * 1024);
    if (ret == 0) {
        printf("FAIL: Should fail to add duplicate file\n");
        return -1;
    }
    printf("PASS: Duplicate file rejected\n");
    
    /* 移除文件 */
    ret = ai_tp_sync_remove_file(ctx, "test_file.dat");
    if (ret != 0) {
        printf("FAIL: Failed to remove file (ret=%d)\n", ret);
        return -1;
    }
    printf("PASS: File removed\n");
    
    /* 清理测试文件 */
    remove("test_file.dat");
    
    return 0;
}

int test_split_file(ai_tp_sync_ctx_t* ctx) {
    printf("\n=== 测试4: 文件分块 ===\n");
    
    /* 创建测试文件 */
    FILE* fp = fopen("test_split.dat", "wb");
    if (fp) {
        /* 写入3MB测试数据 */
        void* data = malloc(1024);
        for (int i = 0; i < 1024; i++) {
            ((uint8_t*)data)[i] = (uint8_t)(i % 256);
        }
        for (int i = 0; i < 3072; i++) {  /* 3MB */
            fwrite(data, 1, 1024, fp);
        }
        free(data);
        fclose(fp);
        printf("PASS: Test file created (3MB)\n");
    } else {
        printf("FAIL: Cannot create test file\n");
        return -1;
    }
    
    /* 添加文件 */
    int ret = ai_tp_sync_add_file(ctx, "test_split.dat", 1024 * 1024);  /* 1MB块 */
    if (ret != 0) {
        printf("FAIL: Failed to add file (ret=%d)\n", ret);
        return -1;
    }
    
    /* 分块文件 */
    ret = ai_tp_sync_split_file(ctx, "test_split.dat");
    if (ret != 0) {
        printf("FAIL: Failed to split file (ret=%d)\n", ret);
        return -1;
    }
    printf("PASS: File split complete\n");
    
    /* 检查块信息 */
    ai_tp_file_info_t* file = &ctx->files[0];
    printf("  file_size=%lu bytes\n", file->file_size);
    printf("  chunk_count=%u\n", file->chunk_count);
    printf("  file_hash=%s\n", file->file_hash);
    
    for (uint32_t i = 0; i < file->chunk_count; i++) {
        printf("  Chunk %u: offset=%lu, size=%lu, hash=%s\n",
               i,
               file->chunks[i].offset,
               file->chunks[i].size,
               file->chunks[i].hash);
    }
    
    /* 清理 */
    ai_tp_sync_remove_file(ctx, "test_split.dat");
    remove("test_split.dat");
    
    return 0;
}

int test_sync_file(ai_tp_sync_ctx_t* ctx) {
    printf("\n=== 测试5: 文件同步 ===\n");
    
    /* 添加对等节点 */
    ai_tp_sync_add_peer(ctx, "node-001", "192.168.1.101:8080", AI_TP_CAP_STORAGE);
    
    /* 创建测试文件 */
    FILE* fp = fopen("test_sync.dat", "wb");
    if (fp) {
        /* 写入2MB测试数据 */
        void* data = malloc(1024);
        for (int i = 0; i < 1024; i++) {
            ((uint8_t*)data)[i] = (uint8_t)(i % 256);
        }
        for (int i = 0; i < 2048; i++) {  /* 2MB */
            fwrite(data, 1, 1024, fp);
        }
        free(data);
        fclose(fp);
        printf("PASS: Test file created (2MB)\n");
    } else {
        printf("FAIL: Cannot create test file\n");
        return -1;
    }
    
    /* 添加文件 */
    int ret = ai_tp_sync_add_file(ctx, "test_sync.dat", 1024 * 1024);  /* 1MB块 */
    if (ret != 0) {
        printf("FAIL: Failed to add file (ret=%d)\n", ret);
        return -1;
    }
    
    /* 设置回调 */
    ctx->on_chunk_transferred = on_chunk_transferred;
    ctx->on_file_synced = on_file_synced;
    
    /* 同步文件 */
    ret = ai_tp_sync_file(ctx, "test_sync.dat", "node-001");
    if (ret != 0) {
        printf("FAIL: Failed to sync file (ret=%d)\n", ret);
        return -1;
    }
    printf("PASS: File sync started\n");
    
    /* 获取进度 */
    int progress = ai_tp_sync_get_progress(ctx, "test_sync.dat");
    printf("  Progress: %d%%\n", progress);
    
    /* 清理 */
    ai_tp_sync_remove_file(ctx, "test_sync.dat");
    ai_tp_sync_remove_peer(ctx, "node-001");
    remove("test_sync.dat");
    
    return 0;
}

int test_resume_sync(ai_tp_sync_ctx_t* ctx) {
    printf("\n=== 测试6: 断点续传 ===\n");
    
    /* 添加对等节点 */
    ai_tp_sync_add_peer(ctx, "node-002", "192.168.1.102:8080", AI_TP_CAP_STORAGE);
    
    /* 创建测试文件 */
    FILE* fp = fopen("test_resume.dat", "wb");
    if (fp) {
        /* 写入1.5MB测试数据 */
        void* data = malloc(1024);
        for (int i = 0; i < 1024; i++) {
            ((uint8_t*)data)[i] = (uint8_t)(i % 256);
        }
        for (int i = 0; i < 1536; i++) {  /* 1.5MB */
            fwrite(data, 1, 1024, fp);
        }
        free(data);
        fclose(fp);
        printf("PASS: Test file created (1.5MB)\n");
    } else {
        printf("FAIL: Cannot create test file\n");
        return -1;
    }
    
    /* 添加文件并分块 */
    ai_tp_sync_add_file(ctx, "test_resume.dat", 1024 * 1024);
    ai_tp_sync_split_file(ctx, "test_resume.dat");
    
    /* 模拟部分传输（只传输前1块）*/
    printf("Simulating partial transfer (chunk 0 only)...\n");
    ctx->files[0].chunks[0].is_transferred = true;
    ctx->files[0].chunks[0].last_transfer_time = time(NULL);
    
    /* 断点续传 */
    printf("Resuming sync...\n");
    int ret = ai_tp_sync_resume(ctx, "test_resume.dat", "node-002");
    if (ret != 0) {
        printf("FAIL: Failed to resume sync (ret=%d)\n", ret);
        return -1;
    }
    printf("PASS: Sync resumed\n");
    
    /* 获取进度 */
    int progress = ai_tp_sync_get_progress(ctx, "test_resume.dat");
    printf("  Progress: %d%%\n", progress);
    
    /* 清理 */
    ai_tp_sync_remove_file(ctx, "test_resume.dat");
    ai_tp_sync_remove_peer(ctx, "node-002");
    remove("test_resume.dat");
    
    return 0;
}

/* ========== 主函数 ========== */

int main(void) {
    printf("========================================\n");
    printf("  AI-TP Sync Module - Basic Test\n");
    printf("  Version: %s\n", ai_tp_sync_get_version());
    printf("========================================\n");
    
    int failed = 0;
    
    /* 测试1: 初始化和销毁 */
    if (test_init_and_destroy() != 0) {
        failed++;
    }
    
    /* 重新初始化用于后续测试 */
    ai_tp_sync_ctx_t* ctx = ai_tp_sync_init(100, 0);
    if (!ctx) {
        printf("FAIL: Cannot continue without context\n");
        return 1;
    }
    
    /* 测试2: 添加/移除对等节点 */
    if (test_add_remove_peer(ctx) != 0) {
        failed++;
    }
    
    /* 测试3: 添加/移除文件 */
    if (test_add_remove_file(ctx) != 0) {
        failed++;
    }
    
    /* 测试4: 文件分块 */
    if (test_split_file(ctx) != 0) {
        failed++;
    }
    
    /* 测试5: 文件同步 */
    if (test_sync_file(ctx) != 0) {
        failed++;
    }
    
    /* 测试6: 断点续传 */
    if (test_resume_sync(ctx) != 0) {
        failed++;
    }
    
    /* 销毁上下文 */
    ai_tp_sync_destroy(ctx);
    
    /* 总结 */
    printf("\n========================================\n");
    if (failed == 0) {
        printf("  All tests PASSED!\n");
    } else {
        printf("  %d test(s) FAILED!\n", failed);
    }
    printf("========================================\n");
    
    return failed;
}
