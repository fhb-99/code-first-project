#include "mediapipeline.h"
#include <QWidget>
#include <QDebug>

MediaPipeline::MediaPipeline(QObject *parent)
    : QObject(parent)
{
}

bool MediaPipeline::StartPlay(const QString &playUrl, QWidget *renderHost)
{
    Q_UNUSED(renderHost);
    qDebug() << "[MediaPipeline] start play url =" << playUrl;
    // TODO:
    // 1) avformat_open_input(playUrl) 建链并拉取压缩码流（H264/H265/AAC等）
    // 2) av_read_frame + avcodec_send_packet/receive_frame 解码为AVFrame
    // 3) 视频帧送入 renderHost 对应渲染器，音频帧送入音频设备
    return !playUrl.isEmpty();
}

void MediaPipeline::Pause(bool pause)
{
    qDebug() << "[MediaPipeline] pause =" << pause;
}

void MediaPipeline::Stop()
{
    qDebug() << "[MediaPipeline] stop";
}

void MediaPipeline::SeekMs(qint64 posMs)
{
    qDebug() << "[MediaPipeline] seek(ms) =" << posMs;
}

void MediaPipeline::SetVolume(int vol)
{
    qDebug() << "[MediaPipeline] volume =" << vol;
}

