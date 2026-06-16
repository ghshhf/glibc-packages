/**
 * ai-tp-lan.c - LAN P2P Handshake Protocol Implementation
 */
#include "ai-tp-lan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <time.h>

/* ============================================================
 * Internal helpers
 * ============================================================ */

static uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static int get_local_ip(char *buf, size_t buflen) {
    /* Get the first non-loopback IPv4 address */
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return -1;

    struct ifconf ifc;
    char ifbuf[4096];
    ifc.ifc_len = sizeof(ifbuf);
    ifc.ifc_buf = ifbuf;
    if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) { close(fd); return -1; }

    struct ifreq *ifr = ifc.ifc_req;
    int nifs = ifc.ifc_len / sizeof(struct ifreq);
    for (int i = 0; i < nifs; i++) {
        struct sockaddr_in *sin = (struct sockaddr_in *)&ifr[i].ifr_addr;
        if (sin->sin_family == AF_INET) {
            uint32_t addr = ntohl(sin->sin_addr.s_addr);
            /* Skip loopback (127.x.x.x) */
            if ((addr >> 24) != 127) {
                inet_ntop(AF_INET, &sin->sin_addr, buf, (socklen_t)buflen);
                close(fd);
                return 0;
            }
        }
    }
    close(fd);
    strncpy(buf, "127.0.0.1", buflen);
    return 0;
}

/* Simple pseudo-random for node_id generation */
static uint64_t xorshift64(uint64_t x) {
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return x;
}

void aitp_lan_generate_node_id(uint8_t node_id[AITP_LAN_MAX_NODE_ID]) {
    srand((unsigned int)(now_ms() ^ (uintptr_t)node_id));
    for (int i = 0; i < AITP_LAN_MAX_NODE_ID; i++) {
        node_id[i] = (uint8_t)(rand() & 0xFF);
    }
}

/* Find peer index by node_id */
static int find_peer(aitp_lan_ctx_t *ctx, const uint8_t node_id[AITP_LAN_MAX_NODE_ID]) {
    for (int i = 0; i < ctx->peer_count; i++) {
        if (memcmp(ctx->peers[i].node_id, node_id, AITP_LAN_MAX_NODE_ID) == 0)
            return i;
    }
    return -1;
}

static int find_peer_by_sock(aitp_lan_ctx_t *ctx, int sockfd) {
    for (int i = 0; i < ctx->peer_count; i++) {
        if (ctx->peers[i].sockfd == sockfd)
            return i;
    }
    return -1;
}

/* Add or update a peer */
static int upsert_peer(aitp_lan_ctx_t *ctx, const uint8_t node_id[AITP_LAN_MAX_NODE_ID]) {
    int idx = find_peer(ctx, node_id);
    if (idx >= 0) return idx;
    if (ctx->peer_count >= AITP_LAN_MAX_PEERS) return -1;
    idx = ctx->peer_count;
    memset(&ctx->peers[idx], 0, sizeof(aitp_lan_peer_t));
    memcpy(ctx->peers[idx].node_id, node_id, AITP_LAN_MAX_NODE_ID);
    ctx->peers[idx].state = AITP_LAN_PEER_DISCOVERED;
    ctx->peers[idx].sockfd = -1;
    ctx->peers[idx].hop_count = 0;
    ctx->people_count++;
    return idx;
}

/* ============================================================
 * UDP Discovery
 * ============================================================ */

static int setup_udp_socket(aitp_lan_ctx_t *ctx) {
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) { perror("udp socket"); return -1; }

    int reuse = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(ctx->discovery_port);

    if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("udp bind"); close(sd); return -1;
    }

    /* Join multicast group */
    struct ip_mreq mreq;
    inet_aton(ctx->multicast_group, &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    return sd;
}

static int send_discovery(aitp_lan_ctx_t *ctx) {
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(ctx->discovery_port);
    inet_aton(ctx->multicast_group, &dest.sin_addr);

    /* Build discovery message: type(1) + node_id(32) + tcp_port(2) + device_type(1) + device_name */
    uint8_t buf[512];
    int pos = 0;
    buf[pos++] = (uint8_t)AITP_LAN_MSG_DISCOVERY;
    memcpy(buf + pos, ctx->local_device.node_id, AITP_LAN_MAX_NODE_ID);
    pos += AITP_LAN_MAX_NODE_ID;
    buf[pos++] = (ctx->tcp_port >> 8) & 0xFF;
    buf[pos++] = ctx->tcp_port & 0xFF;
    buf[pos++] = (uint8_t)ctx->local_device.device_type;
    size_t nlen = strlen(ctx->local_device.device_name);
    if (nlen > AITP_LAN_MAX_DEVICE_NAME - 1) nlen = AITP_LAN_MAX_DEVICE_NAME - 1;
    memcpy(buf + pos, ctx->local_device.device_name, nlen);
    pos += (int)nlen;

    int ret = (int)sendto(sd, buf, (size_t)pos, 0,
                          (struct sockaddr *)&dest, sizeof(dest));
    if (ret < 0 && errno != EAGAIN) return -1;
    return 0;
}

static int handle_discovery_msg(aitp_lan_ctx_t *ctx, const uint8_t *buf, int len,
                                 const struct sockaddr_in *from) {
    if (len < 1 + AITP_LAN_MAX_NODE_ID + 2 + 1) return -1;

    const uint8_t *node_id = buf + 1;
    uint16_t tcp_port = ((uint16_t)buf[33] << 8) | buf[34];
    aitp_lan_device_type_t dtype = (aitp_lan_device_type_t)buf[35];

    /* Skip self-discovery */
    if (memcmp(node_id, ctx->local_device.node_id, AITP_LAN_MAX_NODE_ID) == 0)
        return 0;

    int idx = upsert_peer(ctx, node_id);
    if (idx < 0) return -1;

    aitp_lan_peer_t *peer = &ctx->peers[idx];
    inet_ntop(AF_INET, &from->sin_addr, peer->ip, sizeof(peer->ip));
    peer->tcp_port = tcp_port;
    peer->last_seen_ms = now_ms();
    peer->device_info.device_type = dtype;
    peer->device_info.battery_pct = -1;

    if (len > 36) {
        size_t nlen = (size_t)(len - 36);
        if (nlen > AITP_LAN_MAX_DEVICE_NAME - 1) nlen = AITP_LAN_MAX_DEVICE_NAME - 1;
        memcpy(peer->device_info.device_name, buf + 36, nlen);
        peer->device_info.device_name[nlen] = '\0';
    }

    /* If we haven't initiated a handshake yet, auto-connect */
    if (peer->state == AITP_LAN_PEER_DISCOVERED && peer->sockfd < 0) {
        peer->state = AITP_LAN_PEER_HANDSHAKING;
        /* Callback */
        if (ctx->callbacks.on_peer_discovered)
            ctx->callbacks.on_peer_discovered(&peer->device_info);
        /* Try TCP connect in tick() */
    }

    return 0;
}

/* ============================================================
 * TCP Connection Management
 * ============================================================ */

static int try_connect(aitp_lan_ctx_t *ctx, int peer_idx) {
    aitp_lan_peer_t *peer = &ctx->peers[peer_idx];
    if (peer->sockfd >= 0) return 0;

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(peer->tcp_port);
    inet_aton(peer->ip, &addr.sin_addr);

    /* Non-blocking connect */
    int flags = fcntl(sd, F_GETFL, 0);
    fcntl(sd, F_SETFL, flags | O_NONBLOCK);

    int ret = (int)connect(sd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) {
        close(sd);
        return -1;
    }

    /* Will complete in tick() via select/poll */
    peer->sockfd = sd;
    peer->state = AITP_LAN_PEER_HANDSHAKING;
    return 0;
}

/* Send a complete message over TCP (header + payload) */
static int send_msg(int sockfd, uint8_t msg_type,
                    const uint8_t *data, size_t len,
                    uint32_t token_part) {
    if (sockfd < 0) return -1;
    uint8_t header[16];
    header[0] = msg_type;
    header[1] = 0;  /* flags */
    uint16_t net_len = htons((uint16_t)(len + 16));  /* total msg size including header */
    memcpy(header + 2, &net_len, 2);
    uint32_t net_token = htonl(token_part);
    memcpy(header + 4, &net_token, 4);
    memset(header + 8, 0, 8);  /* reserved */

    struct iovec iov[2];
    iov[0].iov_base = header;
    iov[0].iov_len = 16;
    iov[1].iov_base = (void *)data;
    iov[1].iov_len = len;

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;

    ssize_t sent = sendmsg(sockfd, &msg, MSG_NOSIGNAL);
    return (sent >= (ssize_t)(16 + len)) ? 0 : -1;
}

/* ============================================================
 * Handshake Protocol (4-way)
 * ============================================================ */

static int perform_handshake(aitp_lan_ctx_t *ctx, int peer_idx) {
    aitp_lan_peer_t *peer = &ctx->peers[peer_idx];
    if (peer->sockfd < 0) return -1;

    uint8_t buf[256];
    size_t pos;
    uint32_t token_part;

    if (peer->state == AITP_LAN_PEER_HANDSHAKING && peer->session_id[0] == 0) {
        /* Step 1: Send HANDSHAKE_REQ with our node_id + device info */
        pos = 0;
        memcpy(buf + pos, ctx->local_device.node_id, AITP_LAN_MAX_NODE_ID);
        pos += AITP_LAN_MAX_NODE_ID;
        buf[pos++] = (uint8_t)ctx->local_device.device_type;
        size_t dn_len = strlen(ctx->local_device.device_name);
        memcpy(buf + pos, ctx->local_device.device_name, dn_len);
        pos += dn_len;

        token_part = (uint32_t)(now_ms() & 0xFFFFFFFF);
        return send_msg(peer->sockfd, AITP_LAN_MSG_HANDSHAKE_REQ, buf, pos, token_part);
    }
    return 0;
}

/* ============================================================
 * Tick - main loop (call periodically, e.g. every 100ms)
 * ============================================================ */

int aitp_lan_tick(aitp_lan_ctx_t *ctx) {
    if (!ctx || !ctx->running) return -1;

    uint64_t now = now_ms();

    /* 1. Send periodic discovery announcements */
    if (now - ctx->last_discovery_ms >= ctx->discovery_interval_ms) {
        send_discovery(ctx);
        ctx->last_discovery_ms = now;
    }

    /* 2. Check UDP for incoming discovery messages */
    if (ctx->udp_sock >= 0) {
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        uint8_t buf[AITP_LAN_MAX_MSG_SIZE];

        struct timeval tv = {0, 0};  /* non-blocking */
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(ctx->udp_sock, &rfds);

        if (select(ctx->udp_sock + 1, &rfds, NULL, NULL, &tv) > 0) {
            int rlen = (int)recvfrom(ctx->udp_sock, buf, sizeof(buf), 0,
                                      (struct sockaddr *)&from, &fromlen);
            if (rlen > 0) {
                handle_discovery_msg(ctx, buf, rlen, &from);
            }
        }
    }

    /* 3. Accept new TCP connections */
    if (ctx->tcp_listen_sock >= 0) {
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        struct timeval tv = {0, 0};
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(ctx->tcp_listen_sock, &rfds);
        if (select(ctx->tcp_listen_sock + 1, &rfds, NULL, NULL, &tv) > 0) {
            int clifd = (int)accept(ctx->tcp_listen_sock,
                                     (struct sockaddr *)&from, &fromlen);
            if (clifd >= 0) {
                /* Will be matched to peer by node_id during handshake */
                /* For now just store as unknown */;
            }
        }
    }

    /* 4. Try to connect to DISCOVERED peers */
    for (int i = 0; i < ctx->peer_count; i++) {
        if (ctx->peers[i].state == AITP_LAN_PEER_DISCOVERED &&
            ctx->peers[i].sockfd < 0) {
            try_connect(ctx, i);
        }
    }

    /* 5. Handle HANDSHAKING peers (send req) */
    for (int i = 0; i < ctx->peer_count; i++) {
        if (ctx->peers[i].state == AITP_LAN_PEER_HANDSHAKING &&
            ctx->peers[i].sockfd >= 0 &&
            ctx->peers[i].session_id[0] == 0) {
            perform_handshake(ctx, i);
        }
    }

    /* 6. Check for timeout on RECONNECTING peers - retry connection */
    for (int i = 0; i < ctx->peer_count; i++) {
        if (ctx->peers[i].state == AITP_LAN_PEER_RECONNECTING) {
            aitp_lan_peer_t *p = &ctx->peers[i];
            if (p->reconnect_attempts < AITP_LAN_RECONNECT_RETRIES) {
                if (now - p->last_seen_ms > AITP_LAN_RECONNECT_BACKOFF *
                    (1 << p->reconnect_attempts)) {
                    p->reconnect_attempts++;
                    try_connect(ctx, i);
                }
            } else {
                p->state = AITP_LAN_PEER_TERMINATED;
                if (ctx->callbacks.on_peer_disconnected)
                    ctx->callbacks.on_peer_disconnected(p->node_id, "reconnect timeout");
            }
        }
    }

    /* 7. Send heartbeats to ESTABLISHED peers */
    if (now - ctx->last_heartbeat_ms >= ctx->heartbeat_interval_ms) {
        for (int i = 0; i < ctx->peer_count; i++) {
            if (ctx->peers[i].state == AITP_LAN_PEER_ESTABLISHED &&
                ctx->peers[i].sockfd >= 0) {
                send_msg(ctx->peers[i].sockfd, AITP_LAN_MSG_HEARTBEAT, NULL, 0, 0);
            }
        }
        ctx->last_heartbeat_ms = now;
    }

    /* 8. Detect stale peers */
    for (int i = ctx->peer_count - 1; i >= 0; i--) {
        if (ctx->peers[i].state == AITP_LAN_PEER_ESTABLISHED) {
            if (now - ctx->peers[i].last_seen_ms > ctx->peer_timeout_ms) {
                ctx->peers[i].state = AITP_LAN_PEER_RECONNECTING;
                ctx->peers[i].reconnect_attempts = 0;
                if (ctx->peers[i].sockfd >= 0) {
                    close(ctx->peers[i].sockfd);
                    ctx->peers[i].sockfd = -1;
                }
                if (ctx->callbacks.on_peer_disconnected)
                    ctx->callbacks.on_peer_disconnected(ctx->peers[i].node_id, "heartbeat timeout");
            }
        }
        /* Clean up terminated peers after a grace period */
        if (ctx->peers[i].state == AITP_LAN_PEER_TERMINATED &&
            now - ctx->peers[i].last_seen_ms > ctx->peer_timeout_ms * 2) {
            if (ctx->peers[i].sockfd >= 0) close(ctx->peers[i].sockfd);
            /* Remove by shifting */
            memmove(&ctx->peers[i], &ctx->peers[i+1],
                    sizeof(aitp_lan_peer_t) * (ctx->peer_count - i - 1));
            ctx->peer_count--;
        }
    }

    return 0;
}

/* ============================================================
 * Lifecycle
 * ============================================================ */

int aitp_lan_init(aitp_lan_ctx_t *ctx, const aitp_lan_device_info_t *device_info) {
    if (!ctx || !device_info) return -1;
    memset(ctx, 0, sizeof(*ctx));

    memcpy(&ctx->local_device, device_info, sizeof(*device_info));
    ctx->udp_sock = -1;
    ctx->tcp_listen_sock = -1;

    /* Default config */
    ctx->heartbeat_interval_ms = AITP_LAN_HEARTBEAT_MS;
    ctx->discovery_interval_ms = AITP_LAN_DISCOVERY_INTERVAL;
    ctx->peer_timeout_ms = AITP_LAN_PEER_TIMEOUT_MS;
    strncpy(ctx->multicast_group, AITP_LAN_MULTICAST_GRP, sizeof(ctx->multicast_group));
    ctx->discovery_port = AITP_LAN_DISCOVERY_PORT;

    /* If no node_id, generate one */
    int id_empty = 1;
    for (int i = 0; i < AITP_LAN_MAX_NODE_ID; i++) {
        if (ctx->local_device.node_id[i] != 0) { id_empty = 0; break; }
    }
    if (id_empty) {
        aitp_lan_generate_node_id(ctx->local_device.node_id);
    }
    memcpy(ctx->local_device.node_id, ctx->local_device.node_id, AITP_LAN_MAX_NODE_ID);

    ctx->initialized = 1;
    return 0;
}

int aitp_lan_init_with_callbacks(aitp_lan_ctx_t *ctx,
                                   const aitp_lan_device_info_t *device_info,
                                   const aitp_lan_callbacks_t *callbacks) {
    int ret = aitp_lan_init(ctx, device_info);
    if (ret != 0) return ret;
    if (callbacks) memcpy(&ctx->callbacks, callbacks, sizeof(*callbacks));
    return 0;
}

void aitp_lan_destroy(aitp_lan_ctx_t *ctx) {
    if (!ctx) return;
    aitp_lan_stop(ctx);
    ctx->initialized = 0;
}

int aitp_lan_start(aitp_lan_ctx_t *ctx) {
    if (!ctx || !ctx->initialized) return -1;

    /* Get local IP */
    get_local_ip(ctx->local_ip, sizeof(ctx->local_ip));

    /* Setup UDP discovery socket */
    ctx->udp_sock = setup_udp_socket(ctx);
    if (ctx->udp_sock < 0) { perror("udp setup"); return -1; }

    /* Setup TCP listen socket */
    ctx->tcp_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->tcp_listen_sock < 0) { perror("tcp socket"); return -1; }

    int reuse = 1;
    setsockopt(ctx->tcp_listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0;  /* Let OS assign port */

    if (bind(ctx->tcp_listen_sock, (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        perror("tcp bind"); close(ctx->tcp_listen_sock); ctx->tcp_listen_sock = -1;
        return -1;
    }

    if (listen(ctx->tcp_listen_sock, 10) < 0) {
        perror("tcp listen"); close(ctx->tcp_listen_sock); ctx->tcp_listen_sock = -1;
        return -1;
    }

    /* Get assigned port */
    struct sockaddr_in bound;
    socklen_t blen = sizeof(bound);
    getsockname(ctx->tcp_listen_sock, (struct sockaddr *)&bound, &blen);
    ctx->tcp_port = ntohs(bound.sin_port);

    ctx->running = 1;
    ctx->last_discovery_ms = now_ms();
    ctx->last_heartbeat_ms = now_ms();

    return 0;
}

void aitp_lan_stop(aitp_lan_ctx_t *ctx) {
    if (!ctx) return;
    ctx->running = 0;

    for (int i = 0; i < ctx->peer_count; i++) {
        if (ctx->peers[i].sockfd >= 0) {
            send_msg(ctx->peers[i].sockfd, AITP_LAN_MSG_BYE, NULL, 0, 0);
            close(ctx->peers[i].sockfd);
            ctx->peers[i].sockfd = -1;
        }
    }

    if (ctx->udp_sock >= 0) { close(ctx->udp_sock); ctx->udp_sock = -1; }
    if (ctx->tcp_listen_sock >= 0) { close(ctx->tcp_listen_sock); ctx->tcp_listen_sock = -1; }
}

/* ============================================================
 * Peer Management API
 * ============================================================ */

int aitp_lan_get_peer_count(aitp_lan_ctx_t *ctx) {
    if (!ctx) return 0;
    return ctx->peer_count;
}

int aitp_lan_get_peers(aitp_lan_ctx_t *ctx, aitp_lan_peer_t *out_peers, int max_peers) {
    if (!ctx || !out_peers || max_peers <= 0) return -1;
    int n = (ctx->peer_count < max_peers) ? ctx->peer_count : max_peers;
    memcpy(out_peers, ctx->peers, (size_t)n * sizeof(aitp_lan_peer_t));
    return n;
}

int aitp_lan_find_peer(aitp_lan_ctx_t *ctx, const uint8_t node_id[AITP_LAN_MAX_NODE_ID],
                        aitp_lan_peer_t *out_peer) {
    if (!ctx || !node_id || !out_peer) return -1;
    int idx = find_peer(ctx, node_id);
    if (idx < 0) return -1;
    memcpy(out_peer, &ctx->peers[idx], sizeof(aitp_lan_peer_t));
    return 0;
}

int aitp_lan_disconnect_peer(aitp_lan_ctx_t *ctx,
                               const uint8_t node_id[AITP_LAN_MAX_NODE_ID],
                               const char *reason) {
    if (!ctx || !node_id) return -1;
    int idx = find_peer(ctx, node_id);
    if (idx < 0) return -1;
    ctx->peers[idx].state = AITP_LAN_PEER_TERMINATED;
    if (ctx->peers[idx].sockfd >= 0) {
        send_msg(ctx->peers[idx].sockfd, AITP_LAN_MSG_BYE,
                 (const uint8_t *)reason, reason ? strlen(reason) : 0, 0);
        close(ctx->peers[idx].sockfd);
        ctx->peers[idx].sockfd = -1;
    }
    return 0;
}

/* ============================================================
 * Messaging API
 * ============================================================ */

int aitp_lan_send(aitp_lan_ctx_t *ctx, const uint8_t node_id[AITP_LAN_MAX_NODE_ID],
                    uint8_t msg_type, const uint8_t *data, size_t len) {
    if (!ctx || !node_id) return -1;
    int idx = find_peer(ctx, node_id);
    if (idx < 0) return -1;
    aitp_lan_peer_t *peer = &ctx->peers[idx];
    if (peer->state != AITP_LAN_PEER_ESTABLISHED || peer->sockfd < 0) return -1;
    return send_msg(peer->sockfd, msg_type, data, len,
                    (uint32_t)(peer->handshake_token[0] |
                               ((uint32_t)peer->handshake_token[1] << 8)));
}

int aitp_lan_broadcast(aitp_lan_ctx_t *ctx, uint8_t msg_type,
                         const uint8_t *data, size_t len) {
    if (!ctx) return -1;
    int sent = 0;
    for (int i = 0; i < ctx->peer_count; i++) {
        if (ctx->peers[i].state == AITP_LAN_PEER_ESTABLISHED &&
            aitp_lan_send(ctx, ctx->peers[i].node_id, msg_type, data, len) == 0) {
            sent++;
        }
    }
    return sent;
}

/* ============================================================
 * Network Status
 * ============================================================ */

bool aitp_lan_is_alive(aitp_lan_ctx_t *ctx) {
    if (!ctx) return false;
    /* Network is alive if at least 2 peers are established (directly connected)
       OR at least 1 peer established with others in reconnecting state
       (meaning the mesh has a path) */
    int established = 0;
    int reconnecting = 0;
    for (int i = 0; i < ctx->peer_count; i++) {
        if (ctx->peers[i].state == AITP_LAN_PEER_ESTABLISHED) established++;
        if (ctx->peers[i].state == AITP_LAN_PEER_RECONNECTING) reconnecting++;
    }
    return (established >= 2) || (established >= 1 && reconnecting >= 1);
}

int aitp_lan_get_network_stats(aitp_lan_ctx_t *ctx, int *established_count,
                                 int *reconnecting_count, int *discovered_count) {
    if (!ctx) return -1;
    *established_count = 0;
    *reconnecting_count = 0;
    *discovered_count = 0;
    for (int i = 0; i < ctx->peer_count; i++) {
        switch (ctx->peers[i].state) {
            case AITP_LAN_PEER_ESTABLISHED:  (*established_count)++; break;
            case AITP_LAN_PEER_RECONNECTING:  (*reconnecting_count)++; break;
            case AITP_LAN_PEER_DISCOVERED:    (*discovered_count)++; break;
            default: break;
        }
    }
    return 0;
}

/* ============================================================
 * Utility
 * ============================================================ */

const char* aitp_lan_peer_state_name(aitp_lan_peer_state_t state) {
    switch (state) {
        case AITP_LAN_PEER_DISCOVERED:   return "DISCOVERED";
        case AITP_LAN_PEER_HANDSHAKING:  return "HANDSHAKING";
        case AITP_LAN_PEER_ESTABLISHED:  return "ESTABLISHED";
        case AITP_LAN_PEER_RECONNECTING: return "RECONNECTING";
        case AITP_LAN_PEER_TERMINATED:   return "TERMINATED";
        default: return "UNKNOWN";
    }
}

const char* aitp_lan_device_type_name(aitp_lan_device_type_t type) {
    switch (type) {
        case AITP_LAN_DEVICE_UNKNOWN:  return "UNKNOWN";
        case AITP_LAN_DEVICE_PHONE:    return "PHONE";
        case AITP_LAN_DEVICE_TABLET:   return "TABLET";
        case AITP_LAN_DEVICE_LAPTOP:   return "LAPTOP";
        case AITP_LAN_DEVICE_DESKTOP:  return "DESKTOP";
        case AITP_LAN_DEVICE_SERVER:   return "SERVER";
        case AITP_LAN_DEVICE_EMBEDDED: return "EMBEDDED";
        case AITP_LAN_DEVICE_ROUTER:   return "ROUTER";
        default: return "UNKNOWN";
    }
}
