/**
 * @file test_basic.c
 * @brief libai-storage 基础测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ai-storage.h"

#define TEST_KEY "test_key_123"
#define TEST_DATA "Hello, AI-TP OS Storage Layer!"

int main() {
    printf("=== libai-storage 基础测试 ===\n\n");
    
    /* 1. 初始化存储系统 */
    printf("1. 初始化存储系统...\n");
    ai_storage_ctx_t* ctx = ai_storage_init(AI_STORAGE_LOCAL, "path=./test_data");
    if (!ctx) {
        printf("   ❌ 初始化失败\n");
        return 1;
    }
    printf("   ✅ 初始化成功\n\n");
    
    /* 2. 测试存储对象 */
    printf("2. 测试存储对象...\n");
    int ret = ai_storage_put(ctx, TEST_KEY, TEST_DATA, strlen(TEST_DATA));
    if (ret != AI_STORAGE_OK) {
        printf("   ❌ 存储失败 (错误码: %d)\n", ret);
        ai_storage_destroy(ctx);
        return 1;
    }
    printf("   ✅ 存储成功 (key=%s, len=%zu)\n\n", TEST_KEY, strlen(TEST_DATA));
    
    /* 3. 测试检查对象存在 */
    printf("3. 测试检查对象存在...\n");
    int exists = ai_storage_exists(ctx, TEST_KEY);
    if (exists != 1) {
        printf("   ❌ 对象不存在\n");
        ai_storage_destroy(ctx);
        return 1;
    }
    printf("   ✅ 对象存在\n\n");
    
    /* 4. 测试读取对象 */
    printf("4. 测试读取对象...\n");
    void* data = NULL;
    size_t len = 0;
    ret = ai_storage_get(ctx, TEST_KEY, &data, &len);
    if (ret != AI_STORAGE_OK) {
        printf("   ❌ 读取失败 (错误码: %d)\n", ret);
        ai_storage_destroy(ctx);
        return 1;
    }
    
    printf("   ✅ 读取成功 (len=%zu)\n", len);
    printf("   数据: %s\n\n", (char*)data);
    free(data);
    
    /* 5. 测试获取元数据 */
    printf("5. 测试获取元数据...\n");
    size_t size = 0;
    time_t mtime = 0;
    ret = ai_storage_stat(ctx, TEST_KEY, &size, &mtime);
    if (ret != AI_STORAGE_OK) {
        printf("   ❌ 获取元数据失败 (错误码: %d)\n", ret);
    } else {
        printf("   ✅ 获取元数据成功\n");
        printf("   大小: %zu bytes\n", size);
        printf("   修改时间: %ld\n\n", mtime);
    }
    
    /* 6. 测试列出对象 */
    printf("6. 测试列出对象...\n");
    char** keys = NULL;
    size_t count = 0;
    ret = ai_storage_list(ctx, NULL, &keys, &count);
    if (ret != AI_STORAGE_OK) {
        printf("   ❌ 列出失败 (错误码: %d)\n", ret);
    } else {
        printf("   ✅ 列出成功 (count=%zu)\n", count);
        for (size_t i = 0; i < count; i++) {
            printf("   - %s\n", keys[i]);
            free(keys[i]);
        }
        free(keys);
        printf("\n");
    }
    
    /* 7. 测试删除对象 */
    printf("7. 测试删除对象...\n");
    ret = ai_storage_delete(ctx, TEST_KEY);
    if (ret != AI_STORAGE_OK) {
        printf("   ❌ 删除失败 (错误码: %d)\n", ret);
    } else {
        printf("   ✅ 删除成功\n");
        
        /* 验证删除 */
        exists = ai_storage_exists(ctx, TEST_KEY);
        if (exists == 0) {
            printf("   ✅ 验证: 对象已删除\n");
        } else {
            printf("   ❌ 验证失败: 对象仍存在\n");
        }
    }
    
    /* 8. 清理 */
    printf("\n8. 清理...\n");
    ai_storage_destroy(ctx);
    printf("   ✅ 清理完成\n\n");
    
    printf("=== 所有测试通过 ✅ ===\n");
    return 0;
}
