/* Basic Test */
#include "../include/ai-tp-sync.h"
#include <stdio.h>

int main(void) {
    printf("AI-TP Sync Test\n");
    ai_tp_sync_ctx_t* ctx = ai_tp_sync_init(100, 0);
    if (ctx) {
        printf("Init OK\n");
        ai_tp_sync_destroy(ctx);
    }
    return 0;
}
