#include "MediaModeCoordinator.h"
#include <QDebug>

MediaModeCoordinator::MediaModeCoordinator(QObject* parent)
    : QObject(parent)
{
}

void MediaModeCoordinator::setState(CoordinatorState newState)
{
    if (_state == newState)
        return;

    const CoordinatorState old = _state;
    _state = newState;
    qDebug() << "[MediaModeCoordinator] state:" << static_cast<int>(old)
             << "->" << static_cast<int>(newState);
    emit stateChanged(old, newState);
}

// ============================================================================
// 播放源切换
// ============================================================================
bool MediaModeCoordinator::switchSourceKind(PlaySourceKind kind)
{
    if (_source_kind == kind)
        return true;

    // 若当前正在摄像头预览且要切换到远程，先关闭摄像头
    if (isCameraActive() && kind != PlaySourceKind::LocalDevice)
    {
        qDebug() << "[MediaModeCoordinator] closing camera before switching source";
        requestCloseCamera();
    }

    // 若当前正在播放远程流且要切换到摄像头，先停止播放
    if (_state == CoordinatorState::PlayingRemote && kind == PlaySourceKind::LocalDevice)
    {
        qDebug() << "[MediaModeCoordinator] stopping remote before switching to camera";
        emit statusMessage(QStringLiteral("请先停止服务器播放再切换到本地设备"));
        // 此处不自动停止，由 UI 侧决定是否先调用 stop
        return false;
    }

    _source_kind = kind;
    qDebug() << "[MediaModeCoordinator] source kind switched to:" << static_cast<int>(kind);
    emit sourceKindChanged(kind);
    return true;
}

// ============================================================================
// 摄像头操作
// ============================================================================
void MediaModeCoordinator::requestOpenCamera()
{
    if (_state == CoordinatorState::PlayingRemote)
    {
        qDebug() << "[MediaModeCoordinator] cannot open camera while playing remote stream";
        emit statusMessage(QStringLiteral("请先停止服务器播放再打开摄像头"));
        return;
    }

    if (isCameraActive())
    {
        qDebug() << "[MediaModeCoordinator] camera already active, ignoring";
        return;
    }

    // 根据平台构造摄像头设备 URL：
    //   Windows dshow: "video=<设备名>"（常见："Integrated Camera", "USB Camera"）
    //   Linux   v4l2 : "/dev/video0"
    // 可用 ffmpeg -list_devices true -f dshow -i dummy 查看本机可用设备
#ifdef Q_OS_WIN
    const QString deviceUrl = QStringLiteral("video=Integrated Camera");
#elif defined(Q_OS_LINUX)
    const QString deviceUrl = QStringLiteral("/dev/video0");
#else
    const QString deviceUrl;  // 空字符串 = 平台不支持
#endif

    if (deviceUrl.isEmpty()) {
        emit statusMessage(QStringLiteral("当前平台不支持摄像头"));
        return;
    }

    setState(CoordinatorState::Previewing);
    emit cameraOpenRequested(deviceUrl);
}

void MediaModeCoordinator::requestCloseCamera()
{
    if (!isCameraActive())
        return;

    // 若正在录制，先停录制
    if (_state == CoordinatorState::Recording)
    {
        requestStopRecording();
    }

    setState(CoordinatorState::Idle);
    emit cameraCloseRequested();
}

// ============================================================================
// 录制操作（预留框架）
// ============================================================================
void MediaModeCoordinator::requestStartRecording()
{
    if (_state != CoordinatorState::Previewing)
    {
        qDebug() << "[MediaModeCoordinator] recording only available in Previewing state";
        emit statusMessage(QStringLiteral("请先打开摄像头再录制"));
        return;
    }

    setState(CoordinatorState::Recording);
    emit recordingStartRequested();
}

void MediaModeCoordinator::requestStopRecording()
{
    if (_state != CoordinatorState::Recording)
        return;

    setState(CoordinatorState::Previewing);
    emit recordingStopRequested();
}

// ============================================================================
// 播放操作
// ============================================================================
void MediaModeCoordinator::notifyPlayRequested()
{
    if (_source_kind == PlaySourceKind::LocalDevice)
    {
        // 本地设备：摄像头已通过 requestOpenCamera 打开，这里只是标记状态
        // 实际视频采集由 MediaPipeline 在 cameraOpen 后接管
        return;
    }

    if (_source_kind == PlaySourceKind::ServerStream)
    {
        setState(CoordinatorState::PlayingRemote);
    }
}

void MediaModeCoordinator::notifyPlayStopped()
{
    if (_state == CoordinatorState::PlayingRemote)
    {
        setState(CoordinatorState::Idle);
    }
}
