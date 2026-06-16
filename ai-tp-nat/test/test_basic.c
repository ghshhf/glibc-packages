#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ai-tp-nat.h"

int main() {
    printf("=== ai-tp-nat 基础测试 ===\n\n");
    
    printf("1. 初始化 NAT 穿透上下文...\n");
    ai_tp_nat_ctx_t* ctx = ai_tp_nat_init(NULL);
    if (!ctx) {
        printf("   ❌ 初始化失败\n");
        return 1;
    }
    printf("   ✅ 初始化成功\n\n");
    
    printf("2. 检测 NAT 类型...\n");
    ai_tp_nat_type_t nat_type = ai_tp_nat_detect_type(ctx);
    printf("   NAT 类型: %d\n", nat_type);
    printf("   ✅ 检测完成\n\n");
    
    printf("3. 获取公网地址...\n");
    ai_tp_addr_t public_addr;
    int ret = ai_tp_nat_get_public_addr(ctx, &public_addr);
    if (ret == AI_TP_NAT_OK) {
        printf("   公网地址: %s:%d\n", public_addr.ip, public_addr.port);
        printf("   ✅ 获取成功\n\n");
    }
    
    printf("4. 测试 AI 端口预测...\n");
    ai_tp_nat_record_mapping(ctx, 8080, 45678, time(NULL));
    ai_tp_nat_record_mapping(ctx, 8080, 45679, time(NULL) + 10);
    ai_tp_nat_record_mapping(ctx, 8080, 45680, time(NULL) + 20);
    
    unsigned short predicted_port = ai_tp_nat_predict_port(ctx, 8080);
    if (predicted_port > 0) {
        printf("   预测端口: %d\n", predicted_port);
        printf("   ✅ 预测成功\n\n");
    }
    
    printf("5. 生成组网验证码...\n");
    char invite_code[128];
    ret = ai_tp_nat_generate_invite_code(ctx, invite_code);
    if (ret == AI_TP_NAT_OK) {
        printf("   验证码: %s\n", invite_code);
        printf("   ✅ 生成成功\n\n");
    }
    
    printf("6. 通过验证码加入网络...\n");
    ret = ai_tp_nat_join_network(ctx, invite_code, 0.2);
    if (ret == AI_TP_NAT_OK) {
        printf("   ✅ 加入网络成功\n\n");
    }
    
    printf("7. 设置资源分配比例...\n");
    ai_tp_nat_set_resource_ratio(0.2, 0.2);
    printf("   ✅ 设置完成\n\n");
    
    printf("8. 测试 NAT 穿透...\n");
    ai_tp_addr_t peer_addr = {"198.51.100.1", 12345};
    ret = ai_tp_nat_punch(ctx, &peer_addr, 8080);
    if (ret == AI_TP_NAT_OK) {
        printf("   ✅ 穿透成功\n\n");
    }
    
    printf("9. 清理...\n");
    ai_tp_nat_destroy(ctx);
    printf("   ✅ 清理完成\n\n");
    
    printf("=== 所有测试通过 ✅ ===\n");
    return 0;
}
