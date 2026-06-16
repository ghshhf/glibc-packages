/**
 * ai-tp-lan.h - LAN P2P Handshake Protocol for AI-TP Mesh Network
 *
 * The lowest-level P2P layer for devices on the same LAN/WiFi subnet.
 * Core design principles:
 *
 * 1. HANDSHAKE IS THE NETWORK — Once two devices handshake, they form
 *    a persistent session. The session survives TCP disconnections.
 *    As long as any two handshaked peers can reach each other, the
 *    mesh network is considered alive.
 *
 * 2. SYSTEM-LEVEL INTEGRATION — After handshake, peers exchange
 *    device firmware info, OS version, CPU/RAM/storage capabilities.
 *    This enables automatic resource discovery without manual config.
 *
 * 3. AUTO-RECOVERY — If a peer disconnects (WiFi dropout, phone screen off),
 *    the session enters RECONNECTING state. When the peer reappears,
 *    the handshake token re-establishes the session seamlessly.
 *
 * 4. MESH PERSISTENCE — Every peer maintains a neighbor table. A mesh
 *    network exists if there is ANY path (direct or through intermediate
 *    peers) between handshaked devices.
 */
#ifndef AI_TP_LAN_H
#define AI_TP_LAN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Constants
 * ============================================================ */

#define AITP_LAN_VERSION_MAJOR      1
#define AITP_LAN_VERSION_MINOR      0

#define AITP_LAN_MAX_DEVICE_NAME    64
#define AITP_LAN_MAX_FIRMWARE_VER   32
#define AITP_LAN_MAX_OS_VER         32
#define AITP_LAN_MAX_NODE_ID        32
#define AITP_LAN_HANDSHAKE_TOKEN    64
#define AITP_LAN_SESSION_ID         32
#define AITP_LAN_MAX_PEERS          256
#define AITP_LAN_MAX_NEIGHBORS      64
#define AITP_LAN_DISCOVERY_PORT     48080
#define AITP_LAN_TCP_BASE_PORT      49000
#define AITP_LAN_MULTICAST_GRP      "224.0.0.180"
#define AITP_LAN_HEARTBEAT_MS       3000
#define AITP_LAN_HANDSHAKE_TIMEOUT  10000
#define AITP_LAN_RECONNECT_RETRIES  5
#define AITP_LAN_RECONNECT_BACKOFF  2000
#define AITP_LAN_PEER_TIMEOUT_MS    30000
#define AITP_LAN_DISCOVERY_INTERVAL 5000
#define AITP_LAN_MAX_MSG_SIZE       4096

/* ============================================================
 * Enums
 * ============================================================ */

/** Device type for capability identification */
typedef enum aitp_lan_device_type {
    AITP_LAN_DEVICE_UNKNOWN    = 0,
    AITP_LAN_DEVICE_PHONE      = 1,
    AITP_LAN_DEVICE_TABLET     = 2,
    AITP_LAN_DEVICE_LAPTOP     = 3,
    AITP_LAN_DEVICE_DESKTOP    = 4,
    AITP_LAN_DEVICE_SERVER     = 5,
    AITP_LAN_DEVICE_EMBEDDED   = 6,
    AITP_LAN_DEVICE_ROUTER     = 7,
} aitp_lan_device_type_t;

/** Peer connection state machine */
typedef enum aitp_lan_peer_state {
    AITP_LAN_PEER_DISCOVERED    = 0,  /* Seen via multicast, not connected */
    AITP_LAN_PEER_HANDSHAKING   = 1,  /* TCP connected, handshake in progress */
    AITP_LAN_PEER_ESTABLISHED   = 2,  /* Handshake complete, fully operational */
    AITP_LAN_PEER_RECONNECTING  = 3,  /* Lost connection, trying to resume */
    AITP_LAN_PEER_TERMINATED    = 4,  /* Session closed, will be evicted */
} aitp_lan_peer_state_t;

/** Handshake protocol message types */
typedef enum aitp_lan_msg_type {
    AITP_LAN_MSG_DISCOVERY       = 1,   /* UDP multicast: "I'm here" */
    AITP_LAN_MSG_HANDSHAKE_REQ   = 2,   /* TCP: handshake challenge */
    AITP_LAN_MSG_HANDSHAKE_RESP  = 3,   /* TCP: handshake response */
    AITP_LAN_MSG_HANDSHAKE_ACK   = 4,   /* TCP: handshake confirmed */
    AITP_LAN_MSG_DEVICE_INFO     = 5,   /* TCP: device capability exchange */
    AITP_LAN_MSG_HEARTBEAT       = 6,   /* TCP: keepalive ping */
    AITP_LAN_MSG_HEARTBEAT_ACK   = 7,   /* TCP: keepalive pong */
    AITP_LAN_MSG_RECONNECT       = 8,   /* TCP: reconnect with session token */
    AITP_LAN_MSG_RECONNECT_ACK   = 9,   /* TCP: reconnect accepted */
    AITP_LAN_MSG_BYE             = 10,  /* TCP: graceful disconnect */
    AITP_LAN_MSG_ROUTE_ANNOUNCE  = 11,  /* TCP: announce known routes */
} aitp_lan_msg_type_t;

/* ============================================================
 * Data Structures
 * ============================================================ */

/**
 * Device firmware/system information
 * Exchanged after successful handshake
 */
typedef struct aitp_lan_device_info {
    aitp_lan_device_type_t device_type;
    char device_name[AITP_LAN_MAX_DEVICE_NAME];
    char firmware_version[AITP_LAN_MAX_FIRMWARE_VER];
    char os_version[AITP_LAN_MAX_OS_VER];
    uint32_t cpu_cores;
    uint32_t cpu_freq_mhz;
    uint64_t ram_total_mb;
    uint64_t storage_total_mb;
    uint64_t storage_avail_mb;
    int32_t  battery_pct;         /* -1 if not applicable (desktop) */
    uint32_t uptime_secs;         /* device uptime */
    uint32_t flags;               /* capability flags (bitmask) */
    uint8_t  node_id[AITP_LAN_MAX_NODE_ID];
} aitp_lan_device_info_t;

/** A discovered or connected peer */
typedef struct aitp_lan_peer {
    aitp_lan_peer_state_t   state;
    aitp_lan_device_info_t  device_info;
    uint8_t                 node_id[AITP_LAN_MAX_NODE_ID];
    uint8_t                 session_id[AITP_LAN_SESSION_ID];
    uint8_t                 handshake_token[AITP_LAN_HANDSHAKE_TOKEN];
    char                    ip[INET6_ADDRSTRLEN];
    uint16_t                tcp_port;
    int                     sockfd;        /* TCP socket fd, -1 if disconnected */
    uint64_t                last_seen_ms;  /* last heartbeat received */
    uint64_t                connected_since_ms;
    uint32_t                hop_count;     /* 0 = direct, >0 = via relay */
    int                     reconnect_attempts;
} aitp_lan_peer_t;

/** Callback for handshake events */
typedef struct aitp_lan_callbacks {
    void (*on_peer_discovered)(const aitp_lan_device_info_t *info);
    void (*on_peer_connected)(const aitp_lan_device_info_t *info);
    void (*on_peer_disconnected)(const uint8_t node_id[AITP_LAN_MAX_NODE_ID], const char *reason);
    void (*on_peer_reconnected)(const aitp_lan_device_info_t *info);
    void (*on_network_state)(int peer_count, bool is_alive);
    void (*on_message)(const uint8_t node_id[AITP_LAN_MAX_NODE_ID],
                       uint8_t msg_type, const uint8_t *data, size_t len);
} aitp_lan_callbacks_t;

/** Main LAN context */
typedef struct aitp_lan_ctx {
    aitp_lan_device_info_t  local_device;
    aitp_lan_callbacks_t    callbacks;
    aitp_lan_peer_t         peers[AITP_LAN_MAX_PEERS];
    int                     peer_count;
    char                    local_ip[INET6_ADDRSTRLEN];
    uint16_t                tcp_port;
    int                     udp_sock;         /* UDP multicast socket */
    int                     tcp_listen_sock;  /* TCP listen socket */
    uint64_t                last_discovery_ms;
    uint64_t                last_heartbeat_ms;
    bool                    running;
    bool                    initialized;
    /* Config */
    uint32_t                heartbeat_interval_ms;
    uint32_t                discovery_interval_ms;
    uint32_t                peer_timeout_ms;
    char                    multicast_group[16];
    uint16_t                discovery_port;
} aitp_lan_ctx_t;

/* ============================================================
 * API Functions
 * ============================================================ */

/* --- Lifecycle --- */
int  aitp_lan_init(aitp_lan_ctx_t *ctx, const aitp_lan_device_info_t *device_info);
int  aitp_lan_init_with_callbacks(aitp_lan_ctx_t *ctx,
                                   const aitp_lan_device_info_t *device_info,
                                   const aitp_lan_callbacks_t *callbacks);
void aitp_lan_destroy(aitp_lan_ctx_t *ctx);

/* --- Start/Stop --- */
int  aitp_lan_start(aitp_lan_ctx_t *ctx);
void aitp_lan_stop(aitp_lan_ctx_t *ctx);

/* --- Tick (call periodically for maintenance) --- */
int  aitp_lan_tick(aitp_lan_ctx_t *ctx);

/* --- Peer Management --- */
int  aitp_lan_get_peer_count(aitp_lan_ctx_t *ctx);
int  aitp_lan_get_peers(aitp_lan_ctx_t *ctx, aitp_lan_peer_t *out_peers, int max_peers);
int  aitp_lan_find_peer(aitp_lan_ctx_t *ctx, const uint8_t node_id[AITP_LAN_MAX_NODE_ID],
                        aitp_lan_peer_t *out_peer);
int  aitp_lan_disconnect_peer(aitp_lan_ctx_t *ctx,
                               const uint8_t node_id[AITP_LAN_MAX_NODE_ID],
                               const char *reason);

/* --- Messaging --- */
int  aitp_lan_send(aitp_lan_ctx_t *ctx, const uint8_t node_id[AITP_LAN_MAX_NODE_ID],
                    uint8_t msg_type, const uint8_t *data, size_t len);
int  aitp_lan_broadcast(aitp_lan_ctx_t *ctx, uint8_t msg_type,
                         const uint8_t *data, size_t len);

/* --- Network Status --- */
bool aitp_lan_is_alive(aitp_lan_ctx_t *ctx);
int  aitp_lan_get_network_stats(aitp_lan_ctx_t *ctx, int *established_count,
                                 int *reconnecting_count, int *discovered_count);

/* --- Utility --- */
const char* aitp_lan_peer_state_name(aitp_lan_peer_state_t state);
const char* aitp_lan_device_type_name(aitp_lan_device_type_t type);
void aitp_lan_generate_node_id(uint8_t node_id[AITP_LAN_MAX_NODE_ID]);

#ifdef __cplusplus
}
#endif

#endif /* AI_TP_LAN_H */
