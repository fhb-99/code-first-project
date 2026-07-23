# 分布式协同多媒体系统

基于微服务架构的桌面端 IM + 音视频协同播放系统。支持多人会话内同步播放 RTSP/本地文件/摄像头画面，Owner 独占控制权，服务端广播同步通知实现多端帧级同步。

## 技术栈

| 类别 | 技术 |
|------|------|
| 语言 | C++17 |
| 客户端框架 | Qt 5.12 |
| 音视频 | FFmpeg 4.x, OpenGL 3.3 (YUV 渲染), SDL2 (音频输出) |
| 服务端网络 | Boost.Asio (TCP) |
| 服务间通信 | gRPC, Protobuf |
| 数据库 | MySQL 8.0, Redis |
| 构建 | CMake, QMake |

## 架构概览

```
┌─────────────────────────────────────────────┐
│                  New_Client                  │
│  Qt UI + FFmpeg 播放管线 + OpenGL 渲染        │
│  StreamController (请求调度)                  │
│  MediaModeCoordinator (播放源状态机)          │
└──────┬──────────────┬───────────────────────┘
       │ TCP          │ TCP
       ▼              ▼
┌─────────────┐  ┌─────────────────────────────────┐
│  GateServer │  │       MediaControlServer         │
│  TCP 入口    │  │  会话管理 / Owner 鉴权 / 同步广播  │
└──────┬──────┘  └────────┬────────────────────────┘
       │                  │
       ▼                  ▼
┌─────────────┐  ┌─────────────────┐
│ ChatServer  │  │    Redis/MySQL  │
│ IM 消息转发  │  │  缓存 + 持久化   │
└──┬───┬──────┘  └─────────────────┘
   │   │ gRPC
   ▼   ▼
┌──────────┐  ┌──────────────┐
│ Status   │  │  VarifyServer │
│ Server   │  │  (Node.js)   │
└──────────┘  └──────────────┘
```

## 目录结构

```
├── GateServer/          # 网关服务 (Boost.Asio TCP)
├── ChatServer/          # 聊天服务 (IM + gRPC)
├── StatusServer/        # 用户状态服务 (gRPC)
├── VarifyServer/        # 邮件验证码服务 (Node.js + gRPC)
├── MediaControlServer/  # 媒体控制服务 (会话管理 + 协同调度)
│   ├── db/              #   数据库迁移脚本
│   ├── include/         #   头文件
│   └── src/             #   源码
├── MediaServer/         # 媒体资源 (数据库脚本)
├── New_Client/          # Qt 桌面客户端
│   ├── core/            #   全局定义、消息 ID
│   ├── media/           #   播放器核心 (FFmpeg/SDL/OpenGL)
│   ├── net/             #   网络层 (TCP 长连接)
│   └── ui/              #   界面层 (聊天/好友/播放器)
└── build-*/             # 构建输出
```

## 核心功能

### 播放器系统

- **多源输入**：本地文件 (`file:///`)、网络流 (`rtsp://` / `http(s)://`)、摄像头 (`video=` dshow / v4l2)
- **多线程解码**：`DecodePipeline` 独立工作线程，`av_read_frame` → `avcodec_send_packet` → `avcodec_receive_frame` 推拉模型
- **OpenGL YUV 渲染**：`QtGlVideoRenderer` 加载 YUV→RGB 片段着色器，共享纹理上传后再绘制
- **SDL2 音频**：独立音频线程，SDL 回调混音输出
- **音视频同步**：有音频流时以 SDL 主时钟为基准同步视频帧；无音频（摄像头等纯视频源）自动降级为实时拉帧模式

### 协同播放

- **会话模型**：用户创建或加入一个"放映室"，每个房间有独立的 media_session 记录
- **Owner 鉴权**：仅会话创建者拥有播放/暂停/停止/Seek 的控制权
- **同步广播**：Owner 操作 → 服务端查询 media_session_member 表 → 逐个推送 ID_MEDIA_SYNC_NOTIFY
- **状态同步**：新成员加入会话时获取当前播放位置 (current_pos_ms)，跳转到同一帧
- **版本控制**：sync_version 自增标记每次操作，客户端可检测是否丢失同步事件

### 网络协议

- **自定义 TCP 协议**：4 字节头 (`msg_id: 2B` + `msg_len: 2B`, BigEndian) + JSON 消息体
- **状态机解析**：循环读取缓冲区，一次 TCP 包粘带多条消息时逐条完整解析
- **服务端**：Boost.Asio 异步两阶段读取 (Head → Body)，单线程消息队列顺序处理

### IM 功能

- 用户注册/登录/密码重置
- 好友搜索/添加/认证
- 文本消息收发

### 摄像头预览

- 下拉框切换为"本地设备" → 点击播放 → 打开摄像头实时采集
- 复用 FFmpeg 解码管线 + OpenGL 渲染，无音频时自动跳过 SDL 初始化
- `MediaModeCoordinator` 状态机管理播放源互斥 (Idle / Previewing / Recording / PlayingRemote)

## 构建运行

### 服务端

```bash
# 以 MediaControlServer 为例
cd MediaControlServer/build
cmake ..
make -j4
./bin/MediaControlServer
```

依赖：Boost, gRPC, Protobuf, MySQL Connector C++, hiredis, redis-plus-plus, JsonCpp

### 客户端 (Windows)

- Qt Creator 打开 `New_Client/New_Client.pro`
- 配置 FFmpeg/SDL2 库路径
- MSVC 2017 64-bit 编译 (`/utf-8` 标志已配置)

### 数据库初始化

```bash
mysql -u root -p < MediaControlServer/db/mysql_media_migration.sql
```

## 数据库表设计

| 表名 | 用途 |
|------|------|
| `user` | 用户基础信息 |
| `friend` / `friend_apply` | 好友关系与申请 |
| `media_stream` | 可播放流目录 (stream_id, url, owner_id) |
| `media_session` | 协同会话元信息 (owner_id, current_stream_id, current_pos_ms, sync_version, state) |
| `media_session_member` | 会话成员 + 心跳时间 |
| `media_session_stream` | 会话与流绑定 + 在线人数统计 |
| `media_client_playing` | 客户端当前播放状态 |

## 消息协议 (ReqId)

| ID | 名称 | 方向 |
|----|------|------|
| 1020/1021 | MEDIA_LIST_REQ/RSP | C↔S |
| 1022/1023 | MEDIA_PLAY_REQ/RSP | C↔S |
| 1024/1025 | MEDIA_STOP_REQ/RSP | C↔S |
| 1026 | MEDIA_SYNC_NOTIFY | S→C 广播 |
| 1031/1032 | MEDIA_SESSION_LIST_REQ/RSP | C↔S |
| 1033/1034 | MEDIA_CREATE_SESSION_REQ/RSP | C↔S |
| 1035/1036 | MEDIA_JOIN_SESSION_REQ/RSP | C↔S |
| 1037/1038 | MEDIA_PAUSE_REQ/RSP | C↔S |
| 1029/1030 | MEDIA_HEARTBEAT_REQ/RSP | C↔S |

## License

MIT
