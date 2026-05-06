#ifndef MEDIAPIPELINE_H
#define MEDIAPIPELINE_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <memory>
#include "AVDecodeAbstract.h"

class QWidget;
class QTimer;
class DecodePipeline;
class QtGlVideoRenderer;
class SdlAudioOutput;

// MediaPipeline：DecodePipeline 解码 + QtGlVideoRenderer 呈现（GUI 线程定时 try_pop 视频帧）。
class MediaPipeline : public QObject
{
    Q_OBJECT
public:
    explicit MediaPipeline(QObject *parent = nullptr);
    ~MediaPipeline() override;

    bool StartPlay(const QString& playUrl, QWidget* renderHost);
    void Pause(bool pause);
    void Stop();
    void SeekMs(qint64 posMs);
    void SetVolume(int vol);

private slots:
    void onPullVideoFrame();

private:
    std::unique_ptr<DecodePipeline> decoder_;
    QtGlVideoRenderer* video_renderer_{nullptr};
    QTimer* pull_timer_{nullptr};
    QPointer<QWidget> render_host_;
    bool paused_{false};
    std::unique_ptr<SdlAudioOutput> audio_output_;

    /**
     * 视频 FIFO 里最前面的帧若 PTS 远大于音频主钟，不能直接丢掉（还须按顺序播）。
     * 先攒在此，等音频主钟追上再参与与队列后续帧的合成。
     */
    AvFrameUniquePtr pending_early_video_;
};

#endif // MEDIAPIPELINE_H
