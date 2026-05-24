#ifndef MEDIAMODECOORDINATOR_H
#define MEDIAMODECOORDINATOR_H

#include <QObject>
#include <QString>

/**
 * @brief 播放源类型枚举
 *
 * 与 ChatDialog::PlaySourceMode 一一对应，独立定义以解耦 UI 与协调层。
 */
enum class PlaySourceKind {
    ServerStream = 0,  // 服务器流（RTSP/HLS，通过 MediaControlServer）
    LocalFile    = 1,  // 本地文件
    LocalDevice  = 2   // 本地设备（摄像头/采集卡）
};

/**
 * @brief 协调器当前工作状态
 */
enum class CoordinatorState {
    Idle,           // 空闲，无任何播放/预览/录制活动
    Previewing,     // 摄像头预览中（本地设备采集画面显示，不推流）
    Recording,      // 预览 + 录制中（未来扩展）
    PlayingRemote   // 播放服务器流（走 MediaControlServer 协同逻辑）
};

/**
 * @brief 媒体模式协调器
 *
 * 职责：
 * - 管理播放源切换（服务器流 / 本地文件 / 摄像头设备）
 * - 强制执行互斥：摄像头预览 ↔ 服务器流播放，同一时刻只允许一种
 * - 管理录制开关（未来扩展）
 * - 通过信号通知 ChatDialog 执行实际操作，保持与 UI / Pipeline 的解耦
 *
 * 使用方式：
 * ChatDialog 持有唯一的 MediaModeCoordinator 实例，
 * UI 按钮点击 → ChatDialog 槽 → 调用 coordinator 方法 → coordinator 发信号 → ChatDialog 执行。
 */
class MediaModeCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit MediaModeCoordinator(QObject* parent = nullptr);
    ~MediaModeCoordinator() override = default;

    // ---- 查询接口 ----

    PlaySourceKind currentSourceKind() const noexcept { return _source_kind; }
    CoordinatorState currentState() const noexcept { return _state; }
    bool isCameraActive() const noexcept { return _state == CoordinatorState::Previewing
                                           || _state == CoordinatorState::Recording; }
    bool isRecording() const noexcept { return _state == CoordinatorState::Recording; }

    // ---- 播放源切换 ----

    /**
     * @brief 用户在下拉框中切换了播放来源
     * @param kind 新的来源类型
     * @return 切换是否成功（若当前正在摄像头预览中，切换到远程会先关闭摄像头）
     */
    bool switchSourceKind(PlaySourceKind kind);

    // ---- 摄像头操作 ----

    /** 请求打开摄像头预览 */
    void requestOpenCamera();

    /** 请求关闭摄像头（若正在录制先停录制） */
    void requestCloseCamera();

    // ---- 录制操作（当前仅 UI 开关，实现预留） ----

    /** 开始录制（仅在 Previewing 状态下有效） */
    void requestStartRecording();

    /** 停止录制（回到 Previewing 状态） */
    void requestStopRecording();

    // ---- 播放操作 ----

    /** 通知协调器：用户点击了播放按钮 */
    void notifyPlayRequested();

    /** 通知协调器：播放已停止（由 ChatDialog 在 Stop 完成后调用） */
    void notifyPlayStopped();

signals:
    // ---- 摄像头信号 → ChatDialog 负责调用 MediaPipeline ----
    void cameraOpenRequested(const QString& deviceUrl);
    void cameraCloseRequested();

    // ---- 录制信号（预留） ----
    void recordingStartRequested();
    void recordingStopRequested();

    // ---- 状态变更通知 → UI 更新 ----
    void stateChanged(CoordinatorState oldState, CoordinatorState newState);
    void sourceKindChanged(PlaySourceKind newKind);

    // ---- 错误/提示信息 ----
    void statusMessage(const QString& message);

private:
    /** 设置内部状态并发出 stateChanged 信号 */
    void setState(CoordinatorState newState);

    PlaySourceKind    _source_kind = PlaySourceKind::ServerStream;
    CoordinatorState  _state       = CoordinatorState::Idle;
};

#endif // MEDIAMODECOORDINATOR_H
