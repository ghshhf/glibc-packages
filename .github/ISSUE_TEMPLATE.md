感谢你为 SkyNet (天网) 提交 Issue！

请根据你的情况选择合适的 Issue 类型：

## Issue 类型

- **Bug 报告**：某个组件/SDK/构建出现问题
- **包请求**：希望添加新的软件包
- **SSI 规范改进**：对 SYSTEM-STANDARD.md 或 SPEC-INTERFACE.md 的建议
- **功能请求**：希望在构建系统或运行时中添加新功能
- **其他**：以上都不适用的情况

## Bug 报告模板

```
**现象描述**
（简洁描述你遇到的问题）

**复现步骤**
1. cd SkyNet/
2. 执行的操作
3. 观察到以下错误：
   （粘贴关键错误日志）

**预期行为**
（描述你期望的正确结果）

**环境信息**
- 平台：（如 Linux x86_64 / Windows 11 / Android 14）
- SkyNet 版本：（`git rev-parse HEAD`）
- 运行模式：（Browser / Native / Embedded）

**额外信息**
（其他有用的信息，如截图、相关链接等）
```

## SSI 规范改进模板

```
**涉及的接口**
（如 SSI-BR / SSI-CORE / SSI-SEC 等）

**当前规范的问题**
（描述现有规范不足之处）

**改进建议**
（具体的接口签名修改或新增功能）

**兼容性影响**
（对已有实现的影响评估）
```

---

> 提示：在提交前，请先搜索现有 Issues，确认你的问题不是重复的。  
> SSI 相关讨论请参考 SYSTEM-STANDARD.md 和 SPEC-INTERFACE.md。
