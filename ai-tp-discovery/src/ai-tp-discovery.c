#include "ai-tp-discovery.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static int find_node_index(ai_tp_discovery_ctx_t* ctx, const char* node_id) {
    for (uint32_t i = 0; i < ctx->node_count; i++) {
        if (strcmp(ctx->nodes[i].id, node_id) == 0) return (int)i;
    }
    return -1;
}
