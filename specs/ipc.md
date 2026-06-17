# SSI IPC 总线规范

> **Inter-Component Communication** — 系统组件间所有通信的标准化总线协议。

---

## 1. 总线架构

```
┌──────────────────────────────────────────────────┐
│                   SSI Service Bus                  │
│                                                    │
│  ┌──────────────┐  ┌──────────────┐              │
│  │   Registry   │  │   Router     │  ┌──────────┐ │
│  │   (名称注册)  │  │   (消息路由)  │  │ Security │ │
│  └──────────────┘  └──────────────┘  │ (ACL)    │ │
│                                        └──────────┘ │
│  ┌────────────────────────────────────────────────┐ │
│  │         Message Queue Manager                   │ │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐         │ │
│  │  │ Q:c1 │ │ Q:c2 │ │ Q:c3 │ │ ...  │         │ │
│  │  └──────┘ └──────┘ └──────┘ └──────┘         │ │
│  └────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────┘
```

## 2. 消息格式（二进制）

```
Offset  Size  Field           Description
────────────────────────────────────────────
0       4     magic           0x53534900 (SSI\0)
4       2     version         Protocol version (1)
6       2     flags           0x01=response, 0x02=error, 0x04=stream
8       4     msg_type        0=call, 1=return, 2=event, 3=stream_data
12      8     message_id      Global unique message ID
20      8     correlation_id  Correlation to request (0=request)
28      16    source_uuid     Sender component UUID
44      16    target_uuid     Recipient (zero=multicast)
60      1     priority        0-255
61      1     ttl             Max hops remaining
62      2     reserved
64      4     payload_length  0-16777215 (16MB max)
68      N     payload         swbn-encoded payload
68+N    4     checksum        CRC32 of header+payload
```

## 3. 传输层

支持三种传输后端，运行时自动选择最佳可用方式：

| 后端 | 延迟 | 吞吐 | 适用场景 |
|------|------|------|---------|
| **共享内存** | <1μs | 10GB/s | 同进程组件通信 |
| **MessageChannel** | 5μs | 100MB/s | 浏览器环境 Web Worker 间通信 |
| **WebSocket** | 5ms | 10MB/s | 跨进程/跨设备组件通信 |

## 4. 服务质量

| 优先级 | 值域 | 延迟保证 | 使用场景 |
|--------|------|---------|---------|
| CRITICAL | 200-255 | <1ms | 实时渲染、输入事件 |
| HIGH | 100-199 | <10ms | UI 更新、用户交互 |
| NORMAL | 50-99 | <100ms | 数据查询、存储读写 |
| LOW | 1-49 | <1s | 日志、监控、批量同步 |
| BACKGROUND | 0 | 尽力而为 | 预加载、缓存刷新 |
