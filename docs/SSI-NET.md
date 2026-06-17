# SSI-NET: SkyNet 网络层

> **免费网络层 — 18 种传输协议后端 + 智能路由 + 自动故障转移**  
> 参考实现：[ghshhf/cloudflared → sidecar/](https://github.com/ghshhf/cloudflared)

---

## 概述

SSI-NET 是 SkyNet 体系中的网络接口规范（对应 [SPEC-INTERFACE.md §5](../SPEC-INTERFACE.md#5-ssi-net-网络协议栈接口)）。  
其参考实现在 [cloudflared/sidecar](https://github.com/ghshhf/cloudflared/tree/skynet/sidecar-enhanced/sidecar) 中实现，基于 cloudflared 开源项目增强改造。

**核心理念**：利用 Cloudflare 免费隧道 + 多种备用传输协议，实现真正的 **零成本网络穿透**。

---

## 18 种传输协议后端

```
cloudflare ─── 官方隧道（免费，默认）
tcp-relay ──── 纯 Go TCP 转发
skynet-p2p ─── SkyNet 原生 P2P 直连
http-proxy ─── HTTP CONNECT 代理
socks5 ─────── RFC1928 SOCKS5 代理
quic ───────── QUIC over UDP
webrtc ─────── ICE/STUN 浏览器 P2P
gre ────────── GRE over TCP
packet-tunnel  GRE + IP 封装
udp-tunnel ─── UDP 隧道 (VXLAN)
dns-tunnel ─── DNS-over-DNS（53/UDP，极端封锁逃生）
icmp-tunnel ── ICMP Ping 数据隧道（极端封锁逃生）
ssh-reverse ── SSH 反向隧道 (frp/ngrok 风格)
dtls ───────── DTLS 1.2（UDP 上的 TLS）
wireguard ──── Noise IK + ChaCha20-Poly1305
rtsp/rtmp ──── 流媒体隧道
sftp ───────── 文件传输隧道
mqtt ───────── IoT 消息隧道
```

### 逃生通道（极端封锁环境）

| 后端 | 原理 | 通过性 |
|------|------|--------|
| `dns-tunnel` | 数据编码在 DNS 查询子域名中 | ✅ 53/UDP 几乎全网开放 |
| `icmp-tunnel` | 数据隐藏在 ICMP echo payload | ✅ 只禁 TCP/UDP 但开放 ping |

---

## 架构组件

```
┌──────────────────────────────────────────────┐
│                SSI 组件生命周期               │
│  (sidecar/ssi/component.go)                   │
│  Init → Start → [Pause/Resume] → Stop        │
├──────────────────────────────────────────────┤
│                  智能路由层                    │
│  (sidecar/router)                             │
│  5 种策略: p2p-first / latency / sticky       │
│            round-robin / failover             │
├──────────────────────────────────────────────┤
│                Overlay 网络                    │
│  (sidecar/overlay)                            │
│  虚拟 IP 分配 / Ed25519 身份 / Peer 管理      │
├──────────────────────────────────────────────┤
│              18 种传输后端                     │
│  (sidecar/tunnel/)                            │
│  统一 Backend 接口: Start/Stop/Ready          │
├──────────────────────────────────────────────┤
│              可观测性层                        │
│  (sidecar/metrics)                            │
│  Prometheus 格式 / JSON API / Web Dashboard   │
└──────────────────────────────────────────────┘
```

---

## SSI 接口规范映射

| SSI-NET 规范方法 | cloudflared/sidecar 实现 |
|-----------------|------------------------|
| `connect()` | `NewBackend()` + `Start()` 建立隧道 |
| `listen()` | HTTP/SOCKS5 代理监听 |
| `accept()` | Dashboard/metrics 端点 |
| `send()` | `tunnel.Backend` 数据传输 |
| `receive()` | `tunnel.Backend` 数据接收 |
| `discover_nodes()` | Overlay peer 发现 |
| `nat_traversal()` | ICE/STUN/WebRTC 后端 |
| `http_request()` | HTTP CONNECT 代理后端 |
| `ws_connect()` | WebRTC DataChannel 后端 |

---

## 快速使用

### 前提
- 一个免费的 Cloudflare 账号
- 克隆 [cloudflared](https://github.com/ghshhf/cloudflared) 仓库

### 启动默认隧道

```bash
# skynet/sidecar-enhanced 分支
git checkout skynet/sidecar-enhanced

# 启动 sidecar（默认 Cloudflare 后端）
go run ./sidecar/main.go --tunnel my-tunnel --url http://localhost:8080
```

### 指定后端类型

```bash
# TCP 直连
go run ./sidecar/main.go --backend tcp-relay --relay-target example.com:9000

# SOCKS5 代理
go run ./sidecar/main.go --backend socks5 --proxy-listen :1080

# DNS 隧道（极端封锁环境）
go run ./sidecar/main.go --backend dns-tunnel --dns-server 8.8.8.8:53

# 智能路由（多后端自动切换）
go run ./sidecar/main.go --backend smart-router \
  --extra-args '{"routing_mode":"latency","sub_backends":["cloudflare","skynet-p2p","tcp-relay"]}'
```

### 查看仪表盘

```bash
export SIDECAR_DASHBOARD_ADDR=:8081
# 访问 http://localhost:8081 查看实时状态
```

---

## 为什么免费？

| 原理 | 说明 |
|------|------|
| Cloudflare Tunnel | 利用 Cloudflare 免费套餐的 Argo Tunnel 功能 |
| TCP Relay | 自建中转服务器（用户自备低配 VPS） |
| P2P 直连 | 通过 NAT 穿透直接建立对等连接（无需服务器） |
| DNS/ICMP Tunnel | 利用网络协议本身的可用性 |
| 零依赖 | 所有代码纯 Go，无第三方付费服务依赖 |

**核心商业模式**：Cloudflare 免费套餐已覆盖绝大多数内网穿透场景。  
sidecar 在此基础上增加了 17 种备选协议，确保在任何网络环境下都能找到可用通道。

---

## 相关资源

- [cloudflared 仓库](https://github.com/ghshhf/cloudflared) — SSI-NET 参考实现
- [SPEC-INTERFACE.md §5](../SPEC-INTERFACE.md#5-ssi-net-网络协议栈接口) — SSI-NET 接口规范
- [sidecar/ssi/component.go](https://github.com/ghshhf/cloudflared/blob/skynet/sidecar-enhanced/sidecar/ssi/component.go) — SSI 组件生命周期实现
- [SYSTEM-STANDARD.md](../SYSTEM-STANDARD.md) — SkyNet 系统架构总纲
