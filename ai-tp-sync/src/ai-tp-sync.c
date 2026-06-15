/* AI-TP Sync Implementation (Simplified) */
#include "ai-tp-sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ai_tp_sync_ctx_t* ai_tp_sync_init(uint32_t max_files, uint64_t default_chunk_size) {
    if (max_files == 0) return NULL;
    ai_tp_sync_ctx_t* ctx = (ai_tp_sync_ctx_t*)malloc(sizeof(ai_tp_sync_ctx_t));
    if (!ctx) return NULL;
    ctx->files = (ai_tp_file_info_t*)malloc(sizeof(ai_tp_file_info_t) * max_files);
    if (!ctx->files) { free(ctx); return NULL; }
    memset(ctx->files, 0, sizeof(ai_tp_file_info_t) * max_files);
    ctx->file_count = 0;
    ctx->max_files = max_files;
    ctx->peer_count = 0;
    printf("AI-TP Sync: Initialized\n");
    return ctx;
}

