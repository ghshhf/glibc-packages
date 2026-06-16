/**
 * @file ai-storage.c
 * @brief AI-TP OS 存储抽象层实现
 */

#include "ai-storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

/* ============ 上下文结构 ============ */

struct ai_storage_ctx {
    ai_storage_backend_t backend;
    char* config;
    char* base_path;  /* 本地存储的基础路径 */
};

/* ============ 内部函数 ============ */

static int is_valid_key(const char* key) {
    if (!key || key[0] == '\0') return 0;
    size_t len = strlen(key);
    if (len > 256) return 0;
    for (size_t i = 0; i < len; i++) {
        if (key[i] == '/' || key[i] == '\\') return 0;
        if (key[i] == '.' && i + 1 < len && key[i+1] == '.') return 0;
    }
    return 1;
}

static char* build_path(ai_storage_ctx_t* ctx, const char* key) {
    if (!is_valid_key(key)) return NULL;
    size_t base_len = strlen(ctx->base_path);
    size_t key_len = strlen(key);
    char* path = (char*)malloc(base_len + key_len + 2);
    if (!path) return NULL;
    
    sprintf(path, "%s/%s", ctx->base_path, key);
    return path;
}

/* ============ API 实现 ============ */

ai_storage_ctx_t* ai_storage_init(ai_storage_backend_t backend, const char* config) {
    ai_storage_ctx_t* ctx = (ai_storage_ctx_t*)calloc(1, sizeof(ai_storage_ctx_t));
    if (!ctx) return NULL;
    
    ctx->backend = backend;
    
    /* 解析配置 */
    if (config) {
        ctx->config = strdup(config);
        
        /* 简单解析：config 格式 "path=/path/to/storage" */
        if (strstr(config, "path=")) {
            char* path_start = strstr(config, "path=") + 5;
            char* path_end = strchr(path_start, ';');
            if (path_end) {
                ctx->base_path = (char*)malloc(path_end - path_start + 1);
                strncpy(ctx->base_path, path_start, path_end - path_start);
                ctx->base_path[path_end - path_start] = '\0';
            } else {
                ctx->base_path = strdup(path_start);
            }
        }
    }
    
    /* 默认路径 */
    if (!ctx->base_path) {
        ctx->base_path = strdup("./ai-storage-data");
    }
    
    /* 创建目录 */
#ifdef _WIN32
    mkdir(ctx->base_path);
#else
    mkdir(ctx->base_path, 0755);
#endif
    
    return ctx;
}

void ai_storage_destroy(ai_storage_ctx_t* ctx) {
    if (!ctx) return;
    if (ctx->config) free(ctx->config);
    if (ctx->base_path) free(ctx->base_path);
    free(ctx);
}

int ai_storage_put(ai_storage_ctx_t* ctx, const char* key, const void* data, size_t len) {
    if (!ctx || !key || !data) return AI_STORAGE_ERR_IO;
    
    char* path = build_path(ctx, key);
    if (!path) return AI_STORAGE_ERR_NOMEM;
    
    FILE* fp = fopen(path, "wb");
    if (!fp) {
        free(path);
        return AI_STORAGE_ERR_IO;
    }
    
    size_t written = fwrite(data, 1, len, fp);
    fclose(fp);
    free(path);
    
    return (written == len) ? AI_STORAGE_OK : AI_STORAGE_ERR_IO;
}

int ai_storage_get(ai_storage_ctx_t* ctx, const char* key, void** data, size_t* len) {
    if (!ctx || !key || !data || !len) return AI_STORAGE_ERR_IO;
    
    char* path = build_path(ctx, key);
    if (!path) return AI_STORAGE_ERR_NOMEM;
    
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        free(path);
        return AI_STORAGE_ERR_NOT_FOUND;
    }
    
    /* 获取文件大小 */
    fseek(fp, 0, SEEK_END);
    *len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    /* 分配内存 */
    *data = malloc(*len);
    if (!*data) {
        fclose(fp);
        free(path);
        return AI_STORAGE_ERR_NOMEM;
    }
    
    /* 读取数据 */
    size_t read = fread(*data, 1, *len, fp);
    fclose(fp);
    free(path);
    
    return (read == *len) ? AI_STORAGE_OK : AI_STORAGE_ERR_IO;
}

int ai_storage_delete(ai_storage_ctx_t* ctx, const char* key) {
    if (!ctx || !key) return AI_STORAGE_ERR_IO;
    
    char* path = build_path(ctx, key);
    if (!path) return AI_STORAGE_ERR_NOMEM;
    
    int ret = remove(path);
    free(path);
    
    return (ret == 0) ? AI_STORAGE_OK : AI_STORAGE_ERR_IO;
}

int ai_storage_exists(ai_storage_ctx_t* ctx, const char* key) {
    if (!ctx || !key) return -1;
    
    char* path = build_path(ctx, key);
    if (!path) return -1;
    
    FILE* fp = fopen(path, "rb");
    free(path);
    
    if (fp) {
        fclose(fp);
        return 1;
    }
    return 0;
}

int ai_storage_list(ai_storage_ctx_t* ctx, const char* prefix, char*** keys, size_t* count) {
    if (!ctx || !keys || !count) return AI_STORAGE_ERR_IO;
    
    /* 简化实现：列出 base_path 下所有文件 */
    *count = 0;
    *keys = NULL;
    
#ifdef _WIN32
    /* Windows 实现 */
    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(ctx->base_path, &fd);
    if (h == INVALID_HANDLE_VALUE) return AI_STORAGE_ERR_IO;
    
    /* TODO: 完整实现 */
    FindClose(h);
#else
    /* Linux/macOS 实现 */
    DIR* dir = opendir(ctx->base_path);
    if (!dir) return AI_STORAGE_ERR_IO;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        /* 检查前缀 */
        if (prefix && strncmp(entry->d_name, prefix, strlen(prefix)) != 0) {
            continue;
        }
        
        (*count)++;
        *keys = (char**)realloc(*keys, (*count) * sizeof(char*));
        (*keys)[(*count)-1] = strdup(entry->d_name);
    }
    
    closedir(dir);
#endif
    
    return AI_STORAGE_OK;
}

int ai_storage_stat(ai_storage_ctx_t* ctx, const char* key, size_t* size, time_t* mtime) {
    if (!ctx || !key) return AI_STORAGE_ERR_IO;
    
    char* path = build_path(ctx, key);
    if (!path) return AI_STORAGE_ERR_NOMEM;
    
#ifdef _WIN32
    /* Windows 实现 */
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesEx(path, GetFileExInfoStandard, &fad)) {
        free(path);
        return AI_STORAGE_ERR_NOT_FOUND;
    }
    
    if (size) {
        *size = (fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
    }
    
    if (mtime) {
        FILETIME ft = fad.ftLastWriteTime;
        *mtime = (time_t)((ft.dwHighDateTime << 32 | ft.dwLowDateTime) / 10000000 - 11644473600LL);
    }
#else
    /* Linux/macOS 实现 */
    struct stat st;
    if (stat(path, &st) != 0) {
        free(path);
        return AI_STORAGE_ERR_NOT_FOUND;
    }
    
    if (size) *size = st.st_size;
    if (mtime) *mtime = st.st_mtime;
#endif
    
    free(path);
    return AI_STORAGE_OK;
}

int ai_storage_put_batch(ai_storage_ctx_t* ctx, const char** keys,
                        const void** data, size_t* lens, size_t count) {
    if (!ctx || !keys || !data || !lens) return AI_STORAGE_ERR_IO;
    
    for (size_t i = 0; i < count; i++) {
        int ret = ai_storage_put(ctx, keys[i], data[i], lens[i]);
        if (ret != AI_STORAGE_OK) return ret;
    }
    
    return AI_STORAGE_OK;
}

int ai_storage_copy(ai_storage_ctx_t* ctx_src, ai_storage_ctx_t* ctx_dst, const char* key) {
    if (!ctx_src || !ctx_dst || !key) return AI_STORAGE_ERR_IO;
    
    /* 从源读取 */
    void* data = NULL;
    size_t len = 0;
    int ret = ai_storage_get(ctx_src, key, &data, &len);
    if (ret != AI_STORAGE_OK) return ret;
    
    /* 写入目标 */
    ret = ai_storage_put(ctx_dst, key, data, len);
    free(data);
    
    return ret;
}

int ai_storage_sync(ai_storage_ctx_t* ctx_local, ai_storage_ctx_t* ctx_p2p) {
    /* TODO: 实现同步逻辑 */
    return AI_STORAGE_OK;
}
