# SSI (System Standard Interface) v1.0 规范

> **统一接口规范——所有组件的通信语言。**  
> 本文件是 SYSTEM-STANDARD.md 的配套详细规范，定义所有标准化接口的具体签名、数据结构和行为契约。

---

## 目录

1. [规范约定](#1-规范约定)
2. [SSI-CORE: 核心组件接口](#2-ssi-core-核心组件接口)
3. [SSI-BR: 浏览器引擎接口](#3-ssi-br-浏览器引擎接口)
4. [SSI-UI: 窗口系统接口](#4-ssi-ui-窗口系统接口)
5. [SSI-NET: 网络协议栈接口](#5-ssi-net-网络协议栈接口)
6. [SSI-DB: 存储引擎接口](#6-ssi-db-存储引擎接口)
7. [SSI-AI: 计算引擎接口](#7-ssi-ai-计算引擎接口)
8. [SSI-KRN: 内核运行时接口](#8-ssi-krn-内核运行时接口)
9. [SSI-FS: 文件系统接口](#9-ssi-fs-文件系统接口)
10. [SSI-HAL: 硬件抽象层接口](#10-ssi-hal-硬件抽象层接口)
11. [SSI-SEC: 安全模块接口](#11-ssi-sec-安全模块接口)
12. [IPC 总线规范](#12-ipc-总线规范)

---

## 1. 规范约定

### 1.1 命名规范

| 类别 | 命名规则 | 示例 |
|------|---------|------|
| 接口前缀 | `I` + 驼峰 | `IBrowserEngine`, `IStorageEngine` |
| 组件类型 | 全小写连字符 | `browser-engine`, `window-manager` |
| SSI 方法 | `ssi_` + layer + 动作 | `ssi_browser_render()` |
| 错误码 | `SSI_ERR_` + 大写 | `SSI_ERR_NOT_FOUND` |
| 状态枚举 | `SSI_STATE_` + 大写 | `SSI_STATE_READY` |
| 数据类型 | `Ssi` + 驼峰 | `SsiRect`, `SsiMessage` |

### 1.2 接口定义语法

本规范使用类似 C 的 IDL（Interface Definition Language）描述接口：

```c
/// @interface <接口名>
/// @version <版本号>
/// @mandatory <true|false>  // true = 组件必须实现
interface I组件名 {
    /// @brief <简要说明>
    /// @param <参数名> <方向> <类型> <说明>
    /// @return <类型> <说明>
    /// @error <错误码> <触发条件>
    int method_name(in type param1, out type param2);
};
```

### 1.3 接口级别

| 级别 | 标签 | 说明 |
|------|------|------|
| **MANDATORY** | 🅼 | 所有实现该接口的组件必须实现此方法 |
| **OPTIONAL** | 🅾 | 组件可根据需要选择实现 |
| **DEPRECATED** | 🅳 | 已废弃，仅用于向后兼容 |

---

## 2. SSI-CORE: 核心组件接口

所有系统组件的基接口。系统中每一个组件都必须实现此接口。

```c
/// @interface IComponent (SSI-CORE)
/// @version 1.0
/// @mandatory true
/// @note 所有组件的基接口，无例外
interface IComponent {

    // ──────────── 生命周期 🅼 ────────────

    /// @brief 初始化组件
    /// @param config 组件配置（JSON swbn 编码）
    /// @return SSI_OK 成功，SSI_ERR_INVALID 配置无效
    /// @error SSI_ERR_INVALID config 格式错误
    /// @error SSI_ERR_MEMORY 内存不足
    int init(in SsiBuffer config);

    /// @brief 启动组件
    /// @pre init() 已成功调用
    /// @return SSI_OK 成功
    /// @error SSI_ERR_BUSY 组件已在运行
    int start();

    /// @brief 暂停组件（保留状态）
    /// @return SSI_OK 成功
    /// @error SSI_ERR_NOT_SUPPORTED 不支持暂停
    int suspend();

    /// @brief 从暂停恢复
    /// @return SSI_OK 成功
    int resume();

    /// @brief 停止组件（释放资源）
    /// @return SSI_OK 成功
    int stop();

    /// @brief 销毁组件实例
    void destroy();

    // ──────────── 查询 🅼 ────────────

    /// @brief 获取组件当前状态
    /// @return SsiComponentState 当前状态
    SsiComponentState get_state();

    /// @brief 获取组件标识信息
    /// @return SsiComponentId 组件完整标识
    SsiComponentId get_id();

    /// @brief 获取组件运行时状态
    /// @param out 输出状态信息
    /// @return SSI_OK 成功
    int get_status(out SsiComponentStatus out);

    // ──────────── 通信 🅼 ────────────

    /// @brief 向组件发送消息
    /// @param msg 标准 SSI 消息
    /// @return SSI_OK 成功
    int send_message(in SsiMessage msg);

    /// @brief 从组件接收消息（阻塞）
    /// @param out 输出消息
    /// @param timeout_ms 超时毫秒（0=非阻塞）
    /// @return SSI_OK 成功，SSI_ERR_TIMEOUT 超时
    int receive_message(out SsiMessage out, in uint32_t timeout_ms);

    // ──────────── 事件 🅼 ────────────

    /// @brief 注册事件回调
    /// @param event_type 事件类型
    /// @param callback 回调函数指针
    /// @return SSI_OK 成功
    int on_event(in uint32_t event_type, in SsiEventCallback callback);

    /// @brief 触发事件
    /// @param event 事件数据
    /// @return SSI_OK 成功
    int emit_event(in SsiEvent event);
};

// ──────────── 数据结构 ────────────

typedef struct {
    uint8_t  uuid[16];            // RFC 4122 UUID
    char     name[64];            // 人类可读名称
    char     version[16];         // semver
    char     type[32];            // 组件类型标识
    uint32_t api_version;         // SSI API 版本
    char     vendor[64];          // 供应商
} SsiComponentId;

typedef enum {
    SSI_STATE_UNINITIALIZED = 0,
    SSI_STATE_INITIALIZING,
    SSI_STATE_READY,
    SSI_STATE_RUNNING,
    SSI_STATE_SUSPENDED,
    SSI_STATE_ERROR,
    SSI_STATE_TERMINATED
} SsiComponentState;

typedef struct {
    SsiComponentState state;
    uint64_t          uptime_ms;    // 运行时间
    uint64_t          memory_used;  // 已用内存
    uint64_t          memory_max;   // 最大可用内存
    uint32_t          error_count;  // 错误计数
    double            cpu_usage;    // CPU 使用率 0-1
} SsiComponentStatus;

typedef struct {
    uint32_t type;                  // 消息类型
    uint64_t id;                    // 消息 ID（唯一）
    uint64_t timestamp;             // 时间戳
    uint8_t  source_uuid[16];       // 源组件 UUID
    uint8_t  target_uuid[16];       // 目标组件 UUID
    SsiBuffer payload;              // swbn 编码的负载
    uint32_t priority;              // 0-255，越高越优先
} SsiMessage;

typedef void (*SsiEventCallback)(const SsiEvent* event, void* user_data);

typedef struct {
    uint32_t  type;
    uint64_t  timestamp;
    uint8_t   source_uuid[16];
    SsiBuffer data;
} SsiEvent;

typedef struct {
    uint32_t  size;   // Buffer 大小
    uint8_t*  data;   // swbn 编码的二进制数据
} SsiBuffer;

typedef struct {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
} SsiRect;
```

---

## 3. SSI-BR: 浏览器引擎接口

浏览器是系统的**原生渲染引擎和脚本执行引擎**。此接口定义了系统如何调用浏览器能力。

```c
/// @interface IBrowserEngine (SSI-BR)
/// @version 1.0
/// @mandatory true
/// @note 浏览器引擎组件必须实现此接口
interface IBrowserEngine : IComponent {

    // ──────────── 渲染 🅼 ────────────

    /// @brief 在指定区域渲染内容
    /// @param url 资源 URL（支持 http, file, wasm, data 协议）
    /// @param target 渲染目标区域（像素坐标）
    /// @param opts 渲染选项
    /// @return SSI_OK 成功
    int render(in string url, in SsiRect target, in SsiRenderOptions opts);

    /// @brief 合成多个渲染层
    /// @param layers 层数组
    /// @param count 层数量
    /// @param output 合成输出
    /// @return SSI_OK 成功
    int composite(in SsiRenderLayer[] layers, in uint32_t count, out SsiBitmap output);

    /// @brief 创建渲染上下文
    /// @param width 宽度
    /// @param height 高度
    /// @param config GPU 配置
    /// @return 上下文句柄
    SsiContextHandle create_context(in uint32_t width, in uint32_t height, in SsiGpuConfig config);

    /// @brief 销毁渲染上下文
    void destroy_context(in SsiContextHandle ctx);

    // ──────────── 脚本执行 🅼 ────────────

    /// @brief 执行脚本代码
    /// @param script 脚本代码（支持 JavaScript, WAT, WASM）
    /// @param ctx 执行上下文
    /// @param result 返回值
    /// @return SSI_OK 成功
    int execute_script(in string script, in SsiScriptContext ctx, out SsiValue result);

    /// @brief 编译脚本为字节码
    /// @param script 源代码
    /// @param lang 语言类型 (js/wat)
    /// @param bytecode 输出字节码
    /// @return SSI_OK 成功
    int compile_script(in string script, in string lang, out SsiBuffer bytecode);

    /// @brief 执行预编译字节码
    int execute_bytecode(in SsiBuffer bytecode, in SsiScriptContext ctx, out SsiValue result);

    /// @brief 创建隔离的脚本沙箱
    SsiSandboxHandle create_sandbox(in SsiSandboxConfig config);

    /// @brief 销毁脚本沙箱
    void destroy_sandbox(in SsiSandboxHandle handle);

    // ──────────── GPU 计算 🅾 ────────────

    /// @brief 执行 WebGPU 着色器
    int compute_shader(in string wgsl, in SsiBuffer input, out SsiBuffer output);

    /// @brief 分配 GPU 缓冲区
    SsiGpuBuffer alloc_gpu_buffer(in uint32_t size, in uint32_t usage);

    /// @brief 释放 GPU 缓冲区
    void free_gpu_buffer(in SsiGpuBuffer buf);

    // ──────────── 输入事件 🅼 ────────────

    /// @brief 注入输入事件
    int inject_input_event(in SsiInputEvent event);

    /// @brief 注册输入事件处理器
    int on_input(in SsiInputCallback callback);

    // ──────────── 网络代理 🅼 ────────────

    /// @brief 发起 HTTP 请求（通过 AI-TP 协议栈）
    int fetch(in SsiNetworkRequest req, out SsiNetworkResponse resp);

    /// @brief 创建 WebSocket 连接
    SsiWsHandle create_websocket(in string url, in string[] protocols);

    /// @brief 通过 WebSocket 发送数据
    int ws_send(in SsiWsHandle ws, in SsiBuffer data);

    /// @brief 接收 WebSocket 数据
    int ws_receive(in SsiWsHandle ws, out SsiBuffer data, in uint32_t timeout_ms);

    /// @brief 关闭 WebSocket 连接
    void ws_close(in SsiWsHandle ws);

    // ──────────── 存储代理 🅾 ────────────

    /// @brief 通过 OPFS 存储数据
    int storage_write(in string path, in SsiBuffer data);

    /// @brief 从 OPFS 读取数据
    int storage_read(in string path, out SsiBuffer data);

    /// @brief 列举存储目录
    int storage_list(in string dir, out string[] entries);
};

// ──────────── SSI-BR 数据结构 ────────────

typedef struct {
    uint32_t flags;              // 渲染标志位
    float    scale;              // DPI 缩放 (1.0 = 100%)
    uint8_t  alpha_mode;         // 0=opaque, 1=premultiplied, 2=straight
    bool     hardware_accel;     // 是否启用硬件加速
    uint32_t framerate_limit;    // 帧率上限 (0=无限制)
    string   user_agent;         // 自定义 UA
} SsiRenderOptions;

typedef struct {
    SsiRect     rect;
    SsiBitmap   bitmap;
    float       opacity;         // 0-1
    uint32_t    z_order;         // Z 排序
    string      source_url;      // 图层来源
} SsiRenderLayer;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint8_t* pixels;             // RGBA 像素数据
    uint32_t format;             // 像素格式枚举
} SsiBitmap;

typedef struct {
    int64_t  memory_limit;       // 沙箱内存上限
    uint32_t time_limit_ms;      // 执行时间上限
    string[] allowed_apis;       // 允许的 API 列表
    bool     allow_network;      // 是否允许网络
    bool     allow_fs;           // 是否允许文件系统
    string[] allowed_origins;    // 允许的源
} SsiSandboxConfig;

typedef struct {
    uint32_t type;               // 0=mouse, 1=touch, 2=keyboard, 3=gamepad
    uint32_t action;             // 0=down, 1=up, 2=move, 3=scroll
    int32_t  x, y;               // 坐标
    uint32_t button;             // 按键
    float    pressure;           // 触摸压力
    uint64_t timestamp;
} SsiInputEvent;

typedef struct {
    char     url[2048];          // 请求 URL
    char     method[8];          // GET/POST/PUT/DELETE
    SsiBuffer headers;           // swbn 编码的头部
    SsiBuffer body;              // 请求体
    uint32_t timeout_ms;         // 超时
} SsiNetworkRequest;

typedef struct {
    uint32_t status_code;        // HTTP 状态码
    SsiBuffer headers;
    SsiBuffer body;
    uint64_t elapsed_ms;
} SsiNetworkResponse;

typedef void* SsiContextHandle;
typedef void* SsiSandboxHandle;
typedef void* SsiWsHandle;
typedef void* SsiGpuBuffer;
typedef void (*SsiInputCallback)(const SsiInputEvent* event);
```

---

## 4. SSI-UI: 窗口系统接口

```c
/// @interface IWindowManager (SSI-UI)
/// @version 1.0
/// @mandatory true
interface IWindowManager : IComponent {

    // ──────────── 窗口管理 🅼 ────────────

    /// @brief 创建窗口
    /// @param config 窗口配置
    /// @return 窗口句柄
    SsiWindowHandle create_window(in SsiWindowConfig config);

    /// @brief 关闭窗口
    void destroy_window(in SsiWindowHandle window);

    /// @brief 设置窗口可见性
    void set_window_visible(in SsiWindowHandle window, in bool visible);

    /// @brief 设置窗口大小和位置
    void set_window_rect(in SsiWindowHandle window, in SsiRect rect);

    /// @brief 获取窗口信息
    SsiWindowInfo get_window_info(in SsiWindowHandle window);

    /// @brief 列举所有窗口
    SsiWindowHandle[] list_windows();

    // ──────────── 合成 🅼 ────────────

    /// @brief 提交一帧到窗口显示
    int commit_frame(in SsiWindowHandle window, in SsiBitmap frame);

    /// @brief 注册帧回调（VSync 同步）
    int on_frame(in SsiWindowHandle window, in SsiFrameCallback callback);

    // ──────────── 输入路由 🅼 ────────────

    /// @brief 将输入事件路由到目标窗口
    int route_input(in SsiInputEvent event, out SsiWindowHandle target);
};

typedef struct {
    string  title;
    SsiRect rect;
    uint32_t flags;              // 0=normal, 1=fullscreen, 2=borderless
    bool    resizable;
    bool    transparent;
    float   initial_opacity;
    string  icon_url;
} SsiWindowConfig;

typedef struct {
    SsiRect         rect;
    bool            visible;
    bool            focused;
    float           opacity;
    string          title;
    uint32_t        flags;
    uint64_t        frame_count;
    double          fps;
} SsiWindowInfo;

typedef void* SsiWindowHandle;
typedef void (*SsiFrameCallback)(uint64_t frame_id, uint64_t timestamp_us);
```

---

## 5. SSI-NET: 网络协议栈接口

```c
/// @interface INetworkStack (SSI-NET)
/// @version 1.0
/// @mandatory true
interface INetworkStack : IComponent {

    // ──────────── AI-TP 核心协议 🅼 ────────────

    /// @brief 建立 P2P 连接
    /// @param target 目标节点 ID (ai-tp:xxx)
    /// @param opts 连接选项
    /// @return 连接句柄
    SsiConnectionHandle connect(in string target, in SsiConnectOptions opts);

    /// @brief 监听传入连接
    /// @param address 本地地址
    /// @return 监听器句柄
    SsiListenerHandle listen(in string address);

    /// @brief 接受新连接
    SsiConnectionHandle accept(in SsiListenerHandle listener);

    /// @brief 发送数据
    int send(in SsiConnectionHandle conn, in SsiBuffer data);

    /// @brief 接收数据
    int receive(in SsiConnectionHandle conn, out SsiBuffer data, in uint32_t timeout_ms);

    /// @brief 关闭连接
    void close(in SsiConnectionHandle conn);

    // ──────────── 节点发现 🅼 ────────────

    /// @brief 发现网络中的节点
    SsiNodeInfo[] discover_nodes(in uint32_t max_results, in uint32_t timeout_ms);

    /// @brief 获取本地节点信息
    SsiNodeInfo get_local_node_info();

    /// @brief 解析 AI-TP 地址
    SsiNodeInfo resolve(in string ai_tp_address);

    // ──────────── NAT 穿透 🅼 ────────────

    /// @brief 执行 NAT 穿透
    /// @param target 目标节点
    /// @return 连接句柄（穿透成功后）
    SsiConnectionHandle nat_traversal(in SsiNodeInfo target);

    /// @brief 获取 NAT 类型
    SsiNatType get_nat_type();

    // ──────────── HTTP 兼容 🅼 ────────────

    /// @brief HTTP 请求（通过 AI-TP 网络）
    int http_request(in SsiNetworkRequest req, out SsiNetworkResponse resp);

    /// @brief 创建 WebSocket（通过 AI-TP）
    SsiWsHandle ws_connect(in string url);
};

typedef struct {
    uint32_t timeout_ms;         // 连接超时
    uint32_t encryption;         // 0=none, 1=auto, 2=required
    bool     use_relay;          // 是否使用中继
    bool     use_quic;           // 是否使用 QUIC
} SsiConnectOptions;

typedef enum {
    SSI_NAT_UNKNOWN,
    SSI_NAT_FULL_CONE,
    SSI_NAT_RESTRICTED_CONE,
    SSI_NAT_PORT_RESTRICTED,
    SSI_NAT_SYMMETRIC
} SsiNatType;

typedef struct {
    uint8_t  node_id[32];        // 公钥哈希
    char     ai_tp_address[64];  // ai-tp:xxx
    string[] addresses;          // IP:Port 列表
    uint32_t nat_type;           // SsiNatType
    uint64_t last_seen;          // 最后活跃时间
    double   latency_ms;         // 延迟
    uint32_t score;              // 可靠性评分 0-100
} SsiNodeInfo;

typedef void* SsiConnectionHandle;
typedef void* SsiListenerHandle;
```

---

## 6. SSI-DB: 存储引擎接口

```c
/// @interface IStorageEngine (SSI-DB)
/// @version 1.0
/// @mandatory true
interface IStorageEngine : IComponent {

    // ──────────── KV 存储 🅼 ────────────

    int put(in string key, in SsiBuffer value, in SsiStorageOptions opts);
    int get(in string key, out SsiBuffer value);
    int delete(in string key);
    int exists(in string key, out bool exists);

    // ──────────── P2P 存储 🅼 ────────────

    int p2p_store(in SsiBuffer data, out string content_hash);
    int p2p_retrieve(in string content_hash, out SsiBuffer data);
    int p2p_pin(in string content_hash);
    int p2p_unpin(in string content_hash);

    // ──────────── 存储证明 🅼 ────────────

    int proof_challenge(out SsiBuffer challenge);
    int proof_respond(in SsiBuffer challenge, out SsiBuffer response);
    int proof_verify(in string content_hash, in SsiBuffer challenge, in SsiBuffer response, out bool valid);

    // ──────────── 空间管理 🅼 ────────────

    SsiStorageInfo get_storage_info();
    int reclaim_space(in uint64_t target_bytes);
};

typedef struct {
    uint32_t ttl_seconds;        // 生存时间（0=永久）
    bool     p2p_replicate;      // 是否 P2P 复制
    uint32_t replication_factor; // 复制因子
    bool     encrypt;            // 是否加密
} SsiStorageOptions;

typedef struct {
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t free_bytes;
    uint64_t p2p_used_bytes;
    uint32_t p2p_peer_count;
} SsiStorageInfo;
```

---

## 7. SSI-AI: 计算引擎接口

```c
/// @interface IComputeEngine (SSI-AI)
/// @version 1.0
/// @mandatory true
interface IComputeEngine : IComponent {

    /// @brief 提交 AI 计算任务
    /// @param task 任务描述
    /// @return 任务 ID
    uint64_t submit_task(in SsiAiTask task);

    /// @brief 获取任务状态
    SsiAiTaskStatus get_task_status(in uint64_t task_id);

    /// @brief 取消任务
    int cancel_task(in uint64_t task_id);

    /// @brief 获取任务结果
    int get_task_result(in uint64_t task_id, out SsiBuffer result, in uint32_t timeout_ms);

    /// @brief 获取可用计算资源
    SsiComputeResources get_available_resources();

    /// @brief 注册本地模型
    int register_model(in string model_name, in SsiBuffer model_data);

    /// @brief 执行本地推理
    int inference(in string model_name, in SsiBuffer input, out SsiBuffer output);
};

typedef struct {
    string    type;              // "training", "inference", "generic"
    string    model_name;        // 模型名称
    SsiBuffer input;             // 输入数据
    uint64_t  priority;          // 优先级
    uint64_t  max_cost_tokens;    // 最大消耗 token
    string[]  required_capabilities; // 需要的计算能力
} SsiAiTask;

typedef enum {
    SSI_TASK_PENDING,
    SSI_TASK_RUNNING,
    SSI_TASK_COMPLETED,
    SSI_TASK_FAILED,
    SSI_TASK_CANCELLED
} SsiAiTaskStatus;

typedef struct {
    uint32_t cpu_cores;
    uint32_t gpu_count;
    uint64_t ram_bytes;
    uint64_t vram_bytes;
    float    compute_units;       // 抽象算力单位
} SsiComputeResources;
```

---

## 8. SSI-KRN: 内核运行时接口

```c
/// @interface IKernelRuntime (SSI-KRN)
/// @version 1.0
/// @mandatory true
interface IKernelRuntime : IComponent {

    // ──────────── WASM 模块管理 🅼 ────────────

    /// @brief 加载 WASM 模块
    /// @param path 模块路径或 URL
    /// @param config 加载配置
    /// @return 模块句柄
    SsiModuleHandle load_module(in string path, in SsiLoadConfig config);

    /// @brief 卸载模块
    void unload_module(in SsiModuleHandle module);

    /// @brief 调用模块导出的函数
    int call_function(in SsiModuleHandle module, in string function_name, in SsiBuffer args, out SsiBuffer result);

    /// @brief 获取模块信息
    SsiModuleInfo get_module_info(in SsiModuleHandle module);

    // ──────────── 进程管理 🅼 ────────────

    /// @brief 启动系统组件
    SsiProcessHandle spawn(in string component_type, in SsiBuffer config);

    /// @brief 终止进程
    void kill(in SsiProcessHandle process);

    /// @brief 获取进程列表
    SsiProcessInfo[] list_processes();

    // ──────────── 内存管理 🅼 ────────────

    void* allocate(in uint64_t size);
    void  deallocate(in void* ptr);
    SsiMemoryInfo get_memory_info();

    // ──────────── 时间 🅼 ────────────

    uint64_t get_time_us();
    void sleep(in uint32_t ms);
};

typedef struct {
    uint64_t    stack_size;       // 栈大小
    uint64_t    initial_memory;   // 初始内存
    uint64_t    max_memory;       // 最大内存
    bool        enable_threads;
    bool        enable_fs;
    string[]    allowed_imports;  // 允许的 WASI 导入
    SsiSandboxConfig sandbox;
} SsiLoadConfig;

typedef struct {
    string name;
    string version;
    uint64_t memory_used;
    uint64_t memory_max;
    uint32_t function_count;
    string[] exported_functions;
} SsiModuleInfo;

typedef struct {
    uint64_t total_physical;
    uint64_t available;
    uint64_t total_virtual;
    uint64_t used_by_wasm;
    uint32_t page_faults;
} SsiMemoryInfo;

typedef void* SsiModuleHandle;
typedef void* SsiProcessHandle;

typedef struct {
    uint64_t    pid;
    string      component_type;
    string      name;
    SsiComponentState state;
    uint64_t    memory_bytes;
    double      cpu_usage;
    uint64_t    started_at;
} SsiProcessInfo;
```

---

## 9. SSI-FS: 文件系统接口

```c
/// @interface IFileSystem (SSI-FS)
/// @version 1.0
/// @mandatory true
interface IFileSystem : IComponent {

    int open(in string path, in uint32_t flags, out SsiFileHandle file);
    int close(in SsiFileHandle file);
    int read(in SsiFileHandle file, out SsiBuffer data, in uint32_t size);
    int write(in SsiFileHandle file, in SsiBuffer data);
    int seek(in SsiFileHandle file, in int64_t offset, in uint32_t whence);
    int tell(in SsiFileHandle file, out uint64_t position);
    int stat(in string path, out SsiFileStat stat);
    int mkdir(in string path, in uint32_t mode);
    int readdir(in string path, out SsiDirEntry[] entries);
    int unlink(in string path);
    int rename(in string old_path, in string new_path);
    int mount(in string path, in string backend, in SsiBuffer config);
    int unmount(in string path);

    // ──────────── 虚拟文件系统 🅼 ────────────

    int create_memfs(in string name, in uint64_t max_size);
    int mount_opfs(in string path);
    int mount_idbfs(in string path, in string db_name);
};

typedef struct {
    uint64_t size;
    uint32_t mode;
    bool     is_directory;
    bool     is_file;
    uint64_t created_at;
    uint64_t modified_at;
} SsiFileStat;

typedef struct {
    string name;
    uint32_t type;              // 0=file, 1=dir
    uint64_t size;
} SsiDirEntry;

typedef void* SsiFileHandle;
```

---

## 10. SSI-HAL: 硬件抽象层接口

```c
/// @interface IHardwareAbstraction (SSI-HAL)
/// @version 1.0
/// @mandatory true
interface IHardwareAbstraction : IComponent {

    /// @brief 获取设备信息
    SsiDeviceInfo get_device_info();

    /// @brief 获取传感器数据
    int get_sensor_data(in string sensor_type, out SsiBuffer data);

    /// @brief 控制硬件功能（振动、LED 等）
    int set_hardware_state(in string control, in SsiBuffer value);

    /// @brief 获取电池信息
    SsiBatteryInfo get_battery_info();

    /// @brief 获取网络状态
    SsiNetworkStatus get_network_status();

    /// @brief 屏幕信息
    SsiDisplayInfo get_display_info();
};

typedef struct {
    string  platform;           // "browser", "native", "embedded"
    string  os;
    string  os_version;
    string  arch;
    string  device_model;
    string  device_vendor;
    uint32_t cpu_cores;
    uint64_t total_ram;
    uint64_t total_storage;
    bool    has_gpu;
    string  gpu_vendor;
    bool    battery_powered;
    bool    touch_screen;
} SsiDeviceInfo;

typedef struct {
    uint32_t level;             // 0-100
    uint32_t status;            // 0=unknown, 1=charging, 2=discharging, 3=full
    int32_t  temperature_celsius;
} SsiBatteryInfo;

typedef struct {
    bool    online;
    string  connection_type;    // "wifi", "cellular", "ethernet", "unknown"
    int32_t signal_strength;    // 0-100
    string  ip_address;
} SsiNetworkStatus;

typedef struct {
    uint32_t width;
    uint32_t height;
    float    dpi;
    uint32_t refresh_rate;
    uint32_t color_depth;
} SsiDisplayInfo;
```

---

## 11. SSI-SEC: 安全模块接口

```c
/// @interface ISecurityModule (SSI-SEC)
/// @version 1.0
/// @mandatory true
interface ISecurityModule : IComponent {

    // ──────────── 身份与密钥 🅼 ────────────

    /// @brief 生成本地身份密钥对
    int generate_identity(out SsiKeyPair keys);

    /// @brief 获取本地公钥
    SsiBuffer get_public_key();

    /// @brief 签名数据
    int sign(in SsiBuffer data, out SsiBuffer signature);

    /// @brief 验证签名
    int verify(in SsiBuffer data, in SsiBuffer signature, in SsiBuffer public_key, out bool valid);

    // ──────────── 加密 🅼 ────────────

    int encrypt(in SsiBuffer plaintext, in SsiBuffer key, out SsiBuffer ciphertext);
    int decrypt(in SsiBuffer ciphertext, in SsiBuffer key, out SsiBuffer plaintext);

    // ──────────── 权限管理 🅼 ────────────

    int check_permission(in uint8_t component_uuid[16], in string interface, in string operation, out bool allowed);
    int grant_permission(in uint8_t component_uuid[16], in string permission);
    int revoke_permission(in uint8_t component_uuid[16], in string permission);

    // ──────────── 审计 🅼 ────────────

    int log_event(in SsiSecurityEvent event);
    SsiSecurityEvent[] get_audit_log(in uint64_t since_timestamp, in uint32_t max_count);
};

typedef struct {
    SsiBuffer public_key;
    SsiBuffer private_key;
    uint32_t  algorithm;       // 0=Ed25519, 1=Secp256k1, 2=RSA4096
} SsiKeyPair;

typedef struct {
    uint64_t    timestamp;
    uint8_t     component_uuid[16];
    string      action;
    string      interface;
    string      operation;
    bool        allowed;
    string      reason;
} SsiSecurityEvent;
```

---

## 12. IPC 总线规范

### 12.1 总线架构

所有 SSI 接口的通信通过标准化 IPC 总线进行：

```
┌─────────────────────────────────────────────────────┐
│                  SSI Service Bus                      │
│                                                       │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐       │
│  │ 名称服务   │  │ 路由服务   │  │ 安全服务   │       │
│  │ (Registry)│  │ (Router)  │  │ (Security)│       │
│  └───────────┘  └───────────┘  └───────────┘       │
│                                                       │
│  ┌──────────────────────────────────────────────────┐│
│  │              消息队列 (Message Queue)              ││
│  │  每个组件一个独立队列，消息按优先级排序               ││
│  └──────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────┘
```

### 12.2 消息格式

```
消息帧格式（扁平二进制，无嵌套头）：

┌──────────────────────────────────────────────┐
│ 0                   1                   2     │
│ 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 ... │
├──────────────────────────────────────────────┤
│ Magic: 0x535349 (SSI\0)                      │
│ Version: u16                                  │
│ Flags: u16 (0x01=response, 0x02=error)        │
│ Type: u32                                     │
│ MessageID: u64                                │
│ CorrelationID: u64 (响应时为0)                 │
│ SourceUUID: 16 bytes                          │
│ TargetUUID: 16 bytes                          │
│ Priority: u8                                  │
│ TTL: u8 (跳数)                                │
│ PayloadLength: u32                            │
│ Payload: [PayloadLength] bytes (swbn 编码)     │
│ Checksum: 4 bytes (CRC32)                     │
└──────────────────────────────────────────────┘
总头大小: 64 bytes
```

### 12.3 总线操作

```c
/// IPC 总线接口
interface IServiceBus {

    /// @brief 注册组件到总线
    int register_component(in uint8_t uuid[16], in string type, in string[] interfaces);

    /// @brief 从总线注销组件
    void unregister_component(in uint8_t uuid[16]);

    /// @brief 发送单播消息
    int send_unicast(in SsiMessage msg);

    /// @brief 发送多播消息（按类型）
    int send_multicast(in SsiMessage msg, in string component_type);

    /// @brief 发送广播消息
    int send_broadcast(in SsiMessage msg);

    /// @brief 查询组件位置
    SsiEndpoint resolve_component(in uint8_t uuid[16]);
};

typedef struct {
    uint8_t  uuid[16];
    string   address;           // 总线内部地址
    uint32_t queue_depth;       // 队列深度
} SsiEndpoint;
```

### 12.4 数据序列化（swbn 编码）

所有跨组件数据使用 **Standardized WASM Binary Notation** (swbn) 编码：

```yaml
swbn 规则:
  1. 所有数值使用 Little-Endian
  2. 字符串使用 UTF-8 编码，前置 u32 长度
  3. Buffer 使用前置 u32 长度的字节序列
  4. 数组使用前置 u32 数量的元素序列
  5. 枚举使用 u32
  6. 布尔值使用 u8 (0/1)
  7. 可选值使用标志位 + 值
```

---

> **本规范是所有 AI-TP OS 组件的"宪法"。**  
> 任何不遵循 SSI 接口规范的组件不属于系统。

---

**规范版本**: 1.0.0  
**最后更新**: 2026-06-17  
**维护者**: [@ghshhf](https://github.com/ghshhf)
