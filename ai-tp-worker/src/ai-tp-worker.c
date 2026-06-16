/**
 * ai-tp-worker.c - Remote Compute Worker Agent Implementation
 *
 * Runs on each device: monitors local resources, enforces 20% quota,
 * executes network-submitted tasks in sandbox, reports progress/result.
 */
#include "ai-tp-worker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* ============================================================
 * Internal helpers
 * ============================================================ */

static uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static int find_task(aitp_worker_ctx_t *ctx, const char *task_id) {
    for (int i = 0; i < ctx->task_count; i++) {
        if (strcmp(ctx->tasks[i].task_id, task_id) == 0) return i;
    }
    return -1;
}

static void generate_task_id(char *buf, size_t len) {
    static uint64_t counter = 0;
    counter++;
    snprintf(buf, len, "task-%llu-%llu",
             (unsigned long long)now_ms(),
             (unsigned long long)counter);
}

/* ============================================================
 * Resource monitoring (platform-independent estimates)
 * ============================================================ */

int aitp_worker_monitor_resources(aitp_worker_ctx_t *ctx) {
    if (!ctx) return -1;
    aitp_worker_resources_t *r = &ctx->resources;

    /* CPU: estimate using /proc/stat on Linux, stub on other platforms */
    FILE *fp = fopen("/proc/stat", "r");
    if (fp) {
        char buf[256];
        if (fgets(buf, sizeof(buf), fp)) {
            unsigned long long user, nice, sys, idle;
            if (sscanf(buf, "cpu  %llu %llu %llu %llu",
                       &user, &nice, &sys, &idle) >= 4) {
                static unsigned long long prev_total = 0, prev_idle = 0;
                unsigned long long total = user + nice + sys + idle;
                if (prev_total > 0) {
                    unsigned long long delta_total = total - prev_total;
                    unsigned long long delta_idle = idle - prev_idle;
                    if (delta_total > 0)
                        r->cpu_usage_pct = 100.0f * (float)(delta_total - delta_idle) / (float)delta_total;
                }
                prev_total = total;
                prev_idle = idle;
            }
        }
        fclose(fp);
        r->cpu_cores_total = (uint32_t)sysconf(_SC_NPROCESSORS_ONLN);
    } else {
        /* Stub: assume 4 cores, 50% usage */
        r->cpu_cores_total = 4;
        r->cpu_usage_pct = 50.0f;
    }

    /* RAM: /proc/meminfo on Linux */
    fp = fopen("/proc/meminfo", "r");
    if (fp) {
        char buf[256];
        unsigned long long mem_total = 0, mem_avail = 0;
        while (fgets(buf, sizeof(buf), fp)) {
            if (sscanf(buf, "MemTotal: %llu kB", &mem_total) == 1)
                r->ram_total_mb = mem_total / 1024;
            if (sscanf(buf, "MemAvailable: %llu kB", &mem_avail) == 1) {
                r->ram_available_mb = mem_avail / 1024;
                break;
            }
        }
        fclose(fp);
    } else {
        r->ram_total_mb = 4096;
        r->ram_available_mb = 2048;
    }

    /* Disk: statfs on Linux */
    #ifdef __linux__
    struct statfs sfs;
    if (statfs("/", &sfs) == 0) {
        r->disk_total_mb = (uint64_t)((unsigned long long)sfs.f_blocks *
                                       (unsigned long long)sfs.f_bsize / (1024 * 1024));
    }
    #endif
    if (r->disk_total_mb == 0) r->disk_total_mb = 64000;

    /* Apply 20% quota */
    r->disk_contributed_mb = r->disk_total_mb * r->quota_pct / AITP_WORKER_QUOTA_DENOM;
    r->cpu_cores_available = r->cpu_cores_total * r->quota_pct / AITP_WORKER_QUOTA_DENOM;
    if (r->cpu_cores_available < 1) r->cpu_cores_available = 1;

    /* Battery: read from sysfs on Linux mobile */
    fp = fopen("/sys/class/power_supply/battery/capacity", "r");
    if (fp) {
        int pct;
        if (fscanf(fp, "%d", &pct) == 1) r->battery_pct = pct;
        fclose(fp);
    } else {
        r->battery_pct = 100; /* assume plugged */
    }
    fp = fopen("/sys/class/power_supply/battery/status", "r");
    if (fp) {
        char status[16];
        if (fscanf(fp, "%15s", status) == 1)
            r->is_charging = (strcmp(status, "Charging") == 0);
        fclose(fp);
    } else {
        r->is_charging = true;
    }

    ctx->last_monitor_ms = now_ms();
    if (ctx->callbacks.on_resources_updated)
        ctx->callbacks.on_resources_updated(r);

    return 0;
}

int aitp_worker_set_quota(aitp_worker_ctx_t *ctx, uint32_t quota_pct) {
    if (!ctx || quota_pct > 100) return -1;
    ctx->resources.quota_pct = quota_pct;
    return 0;
}

int aitp_worker_get_quota(aitp_worker_ctx_t *ctx) {
    if (!ctx) return -1;
    return (int)ctx->resources.quota_pct;
}

int aitp_worker_get_resources(aitp_worker_ctx_t *ctx, aitp_worker_resources_t *out_res) {
    if (!ctx || !out_res) return -1;
    memcpy(out_res, &ctx->resources, sizeof(*out_res));
    return 0;
}

bool aitp_worker_can_accept_task(aitp_worker_ctx_t *ctx,
                                  uint32_t cpu, uint64_t ram, uint64_t disk) {
    if (!ctx) return false;
    aitp_worker_resources_t *r = &ctx->resources;

    /* Check battery: don't accept network tasks if battery < 20% and not charging */
    if (r->battery_pct >= 0 && r->battery_pct < 20 && !r->is_charging)
        return false;

    /* Quota check: active_network_tasks already consuming quota */
    uint32_t q = r->quota_pct;
    if (q > 100) q = AITP_WORKER_QUOTA_RATIO;

    /* CPU cores available after quota */
    uint32_t avail_cores = r->cpu_cores_total * q / AITP_WORKER_QUOTA_DENOM;
    if (cpu > avail_cores) return false;

    /* RAM after quota */
    uint64_t avail_ram = r->ram_total_mb * q / AITP_WORKER_QUOTA_DENOM;
    if (ram > avail_ram - r->ram_used_by_network_mb) return false;

    /* Disk after quota */
    uint64_t avail_disk = r->disk_contributed_mb;
    if (disk > avail_disk - r->disk_used_by_network_mb) return false;

    return true;
}

/* ============================================================
 * Task lifecycle
 * ============================================================ */

int aitp_worker_submit_task(aitp_worker_ctx_t *ctx, const aitp_worker_task_t *task) {
    if (!ctx || !task) return -1;
    if (ctx->task_count >= AITP_WORKER_MAX_TASKS) return -2;

    /* For network tasks, check quota first */
    if (task->is_network) {
        if (!aitp_worker_can_accept_task(ctx, task->required_cpu_cores,
                                          task->required_ram_mb,
                                          task->required_disk_mb)) {
            if (ctx->callbacks.on_quota_exceeded) {
                ctx->callbacks.on_quota_exceeded(AITP_WORK_RES_CPU,
                    (float)task->required_cpu_cores, 0);
            }
            return -3; /* quota exceeded */
        }
    }

    int idx = ctx->task_count;
    memcpy(&ctx->tasks[idx], task, sizeof(aitp_worker_task_t));

    /* Auto-generate task_id if empty */
    if (ctx->tasks[idx].task_id[0] == '\0') {
        generate_task_id(ctx->tasks[idx].task_id, sizeof(ctx->tasks[idx].task_id));
    }

    ctx->tasks[idx].received_ms = now_ms();
    ctx->tasks[idx].state = task->is_network ? AITP_WORK_TASK_QUEUED : AITP_WORK_TASK_PENDING;
    ctx->task_count++;

    if (task->is_network) ctx->active_network_tasks++;

    if (ctx->callbacks.on_task_received)
        ctx->callbacks.on_task_received(ctx->tasks[idx].task_id);

    return 0;
}

int aitp_worker_cancel_task(aitp_worker_ctx_t *ctx, const char *task_id) {
    if (!ctx || !task_id) return -1;
    int idx = find_task(ctx, task_id);
    if (idx < 0) return -1;
    ctx->tasks[idx].state = AITP_WORK_TASK_CANCELLED;
    if (ctx->tasks[idx].is_network && ctx->active_network_tasks > 0)
        ctx->active_network_tasks--;
    return 0;
}

int aitp_worker_get_task(aitp_worker_ctx_t *ctx, const char *task_id,
                          aitp_worker_task_t *out_task) {
    if (!ctx || !task_id || !out_task) return -1;
    int idx = find_task(ctx, task_id);
    if (idx < 0) return -1;
    memcpy(out_task, &ctx->tasks[idx], sizeof(aitp_worker_task_t));
    return 0;
}

int aitp_worker_get_tasks(aitp_worker_ctx_t *ctx, aitp_worker_task_t *out_tasks, int max) {
    if (!ctx || !out_tasks || max <= 0) return -1;
    int n = ctx->task_count < max ? ctx->task_count : max;
    memcpy(out_tasks, ctx->tasks, (size_t)n * sizeof(aitp_worker_task_t));
    return n;
}

int aitp_worker_get_active_count(aitp_worker_ctx_t *ctx) {
    if (!ctx) return 0;
    int c = 0;
    for (int i = 0; i < ctx->task_count; i++)
        if (ctx->tasks[i].state == AITP_WORK_TASK_RUNNING) c++;
    return c;
}

/* ============================================================
 * Task execution (stub - logs but simulates real execution)
 * ============================================================ */

int aitp_worker_execute_task(aitp_worker_ctx_t *ctx, const char *task_id) {
    if (!ctx || !task_id) return -1;
    int idx = find_task(ctx, task_id);
    if (idx < 0) return -1;

    aitp_worker_task_t *task = &ctx->tasks[idx];
    task->state = AITP_WORK_TASK_RUNNING;
    task->started_ms = now_ms();
    task->progress = 0.0f;

    if (ctx->callbacks.on_task_started)
        ctx->callbacks.on_task_started(task_id);

    /* Simulated execution: in production, fork+exec the task payload */
    /* For now, mark as immediately completed with a stub result */
    snprintf(task->progress_msg, sizeof(task->progress_msg),
             "executing on %s", ctx->node_id);

    return 0;
}

int aitp_worker_report_progress(aitp_worker_ctx_t *ctx, const char *task_id,
                                  float progress, const char *msg) {
    if (!ctx || !task_id) return -1;
    int idx = find_task(ctx, task_id);
    if (idx < 0) return -1;

    ctx->tasks[idx].progress = progress;
    ctx->tasks[idx].last_progress_ms = now_ms();
    if (msg) {
        strncpy(ctx->tasks[idx].progress_msg, msg,
                sizeof(ctx->tasks[idx].progress_msg) - 1);
    }

    if (ctx->callbacks.on_task_progress)
        ctx->callbacks.on_task_progress(task_id, progress, msg);

    return 0;
}

int aitp_worker_complete_task(aitp_worker_ctx_t *ctx, const char *task_id,
                                const uint8_t *result, size_t len) {
    if (!ctx || !task_id) return -1;
    int idx = find_task(ctx, task_id);
    if (idx < 0) return -1;

    aitp_worker_task_t *task = &ctx->tasks[idx];
    task->state = AITP_WORK_TASK_COMPLETED;
    task->completed_ms = now_ms();
    if (result && len > 0) {
        size_t clen = len < AITP_WORKER_MAX_RESULT_DATA ? len : AITP_WORKER_MAX_RESULT_DATA;
        memcpy(task->result_data, result, clen);
        task->result_len = clen;
    }
    task->progress = 1.0f;

    if (task->is_network && ctx->active_network_tasks > 0)
        ctx->active_network_tasks--;

    if (ctx->callbacks.on_task_completed)
        ctx->callbacks.on_task_completed(task_id, task->result_data, task->result_len);

    return 0;
}

int aitp_worker_fail_task(aitp_worker_ctx_t *ctx, const char *task_id, const char *error) {
    if (!ctx || !task_id) return -1;
    int idx = find_task(ctx, task_id);
    if (idx < 0) return -1;

    aitp_worker_task_t *task = &ctx->tasks[idx];
    task->state = AITP_WORK_TASK_FAILED;
    task->completed_ms = now_ms();
    if (error) {
        strncpy(task->error_msg, error, sizeof(task->error_msg) - 1);
    }

    if (task->is_network && ctx->active_network_tasks > 0)
        ctx->active_network_tasks--;

    if (ctx->callbacks.on_task_failed)
        ctx->callbacks.on_task_failed(task_id, error);

    return 0;
}

/* ============================================================
 * Tick (periodic maintenance)
 * ============================================================ */

int aitp_worker_tick(aitp_worker_ctx_t *ctx) {
    if (!ctx || !ctx->running) return -1;

    uint64_t now = now_ms();

    /* 1. Monitor resources periodically */
    if (now - ctx->last_monitor_ms >= AITP_WORKER_MONITOR_INTERVAL_MS) {
        aitp_worker_monitor_resources(ctx);
    }

    /* 2. Execute queued tasks (promote QUEUED -> RUNNING) */
    for (int i = 0; i < ctx->task_count; i++) {
        if (ctx->tasks[i].state == AITP_WORK_TASK_QUEUED ||
            ctx->tasks[i].state == AITP_WORK_TASK_PENDING) {
            /* For queued network tasks, check if resources freed up */
            if (ctx->tasks[i].is_network) {
                if (!aitp_worker_can_accept_task(ctx,
                        ctx->tasks[i].required_cpu_cores,
                        ctx->tasks[i].required_ram_mb,
                        ctx->tasks[i].required_disk_mb)) {
                    continue; /* still over quota, keep queued */
                }
            }
            aitp_worker_execute_task(ctx, ctx->tasks[i].task_id);
        }
    }

    /* 3. Check for timeout */
    for (int i = 0; i < ctx->task_count; i++) {
        if (ctx->tasks[i].state == AITP_WORK_TASK_RUNNING &&
            ctx->tasks[i].timeout_secs > 0) {
            uint64_t elapsed = (now - ctx->tasks[i].started_ms) / 1000;
            if (elapsed > ctx->tasks[i].timeout_secs) {
                char err[128];
                snprintf(err, sizeof(err), "timeout after %llu seconds",
                         (unsigned long long)elapsed);
                aitp_worker_fail_task(ctx, ctx->tasks[i].task_id, err);
            }
        }
    }

    return 0;
}

/* ============================================================
 * Lifecycle
 * ============================================================ */

int aitp_worker_init(aitp_worker_ctx_t *ctx, const uint8_t node_id[AITP_WORKER_MAX_NODE_ID]) {
    if (!ctx || !node_id) return -1;
    memset(ctx, 0, sizeof(*ctx));

    memcpy(ctx->node_id, node_id, AITP_WORKER_MAX_NODE_ID);

    /* Default resource values */
    ctx->resources.quota_pct = AITP_WORKER_QUOTA_RATIO;
    ctx->resources.battery_pct = 100;
    ctx->resources.is_charging = true;
    ctx->resources.cpu_cores_total = 4;
    ctx->resources.ram_total_mb = 4096;
    ctx->resources.disk_total_mb = 64000;
    ctx->resources.cpu_cores_available = ctx->resources.cpu_cores_total *
                                         AITP_WORKER_QUOTA_RATIO / AITP_WORKER_QUOTA_DENOM;
    ctx->resources.disk_contributed_mb = ctx->resources.disk_total_mb *
                                         AITP_WORKER_QUOTA_RATIO / AITP_WORKER_QUOTA_DENOM;

    ctx->listen_sock = -1;
    ctx->initialized = true;
    return 0;
}

void aitp_worker_destroy(aitp_worker_ctx_t *ctx) {
    if (!ctx) return;
    aitp_worker_stop(ctx);
    ctx->initialized = false;
}

int aitp_worker_start(aitp_worker_ctx_t *ctx) {
    if (!ctx || !ctx->initialized) return -1;

    /* Create TCP listen socket for incoming task assignments */
    ctx->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->listen_sock < 0) {
        perror("worker tcp socket");
        return -1;
    }

    int reuse = 1;
    setsockopt(ctx->listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(AITP_WORKER_DEFAULT_PORT);

    if (bind(ctx->listen_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("worker tcp bind");
        /* Non-fatal: worker can still run without listening */
        close(ctx->listen_sock);
        ctx->listen_sock = -1;
    } else {
        listen(ctx->listen_sock, 10);
        struct sockaddr_in bound;
        socklen_t blen = sizeof(bound);
        getsockname(ctx->listen_sock, (struct sockaddr *)&bound, &blen);
        ctx->listen_port = ntohs(bound.sin_port);
    }

    ctx->running = true;
    ctx->last_monitor_ms = now_ms();
    ctx->last_report_ms = now_ms();

    /* Initial resource snapshot */
    aitp_worker_monitor_resources(ctx);

    return 0;
}

void aitp_worker_stop(aitp_worker_ctx_t *ctx) {
    if (!ctx) return;
    ctx->running = false;
    if (ctx->listen_sock >= 0) {
        close(ctx->listen_sock);
        ctx->listen_sock = -1;
    }
}

int aitp_worker_set_callbacks(aitp_worker_ctx_t *ctx, const aitp_worker_callbacks_t *cbs) {
    if (!ctx || !cbs) return -1;
    memcpy(&ctx->callbacks, cbs, sizeof(*cbs));
    return 0;
}

const char* aitp_worker_state_name(aitp_worker_task_state_t state) {
    switch (state) {
        case AITP_WORK_TASK_PENDING:   return "PENDING";
        case AITP_WORK_TASK_RUNNING:   return "RUNNING";
        case AITP_WORK_TASK_COMPLETED: return "COMPLETED";
        case AITP_WORK_TASK_FAILED:    return "FAILED";
        case AITP_WORK_TASK_CANCELLED: return "CANCELLED";
        case AITP_WORK_TASK_QUEUED:    return "QUEUED";
        default: return "UNKNOWN";
    }
}
