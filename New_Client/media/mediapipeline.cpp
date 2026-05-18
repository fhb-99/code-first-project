#include "mediapipeline.h"

#include "decodepipeline.h"
#include "video_renderer_gl_qtwidget.h"
#include "sdlaudiooutput.h"

#include <QLabel>
#include <QDebug>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>
#include <limits>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/mathematics.h>
}

namespace {

/**
 * 将视频 AVFrame 的 pts（或 FFmpeg 推导的 best_effort）换算成「和媒体轴一致的秒」。
 * tb 必须为 DecodePipeline 里视频流的 time_base（与解码器写的是同一套刻度）。
 */
double frameVideoPtsSeconds(const AVFrame* f, AVRational tb)
{
    if (!f || tb.den <= 0)
        return std::numeric_limits<double>::quiet_NaN();

    int64_t ticks = AV_NOPTS_VALUE;
    if (f->pts != AV_NOPTS_VALUE)
        ticks = f->pts;
    else
        ticks = av_frame_get_best_effort_timestamp(f);

    if (ticks == AV_NOPTS_VALUE)
        return std::numeric_limits<double>::quiet_NaN();

    return static_cast<double>(ticks) * av_q2d(tb);
}

} // namespace



static int64_t positionMsFromAudio(SdlAudioOutput* audio_output)
{
    if(!audio_output) {
        return 0;
    }
    const double sec = audio_output->masterClockMediaSeconds();
    //锚点未建立
    if(std::isnan(sec)) {
        return 0;
    }
    if(sec < 0) {
        return 0;
    }
    return static_cast<int64_t>(sec * 1000 + 0.5);
}


MediaPipeline::MediaPipeline(QObject* parent)
    : QObject(parent)
{
    progress_timer = new QTimer(this);
    connect(progress_timer, &QTimer::timeout, this, &MediaPipeline::onUpdateProgressbar);
}

MediaPipeline::~MediaPipeline()
{
    Stop();
}

bool MediaPipeline::StartPlay(const QString& playUrl, QWidget* renderHost)
{
    if (playUrl.isEmpty() || !renderHost)
        return false;

    Stop();

    render_host_ = renderHost;

    auto* placeholder = renderHost->findChild<QLabel*>(QStringLiteral("lb_video_placeholder"));
    if (placeholder)
        placeholder->hide();

    auto* lay = qobject_cast<QVBoxLayout*>(renderHost->layout());
    if (!lay) {
        qWarning() << "[MediaPipeline] video_render_host has no QVBoxLayout";
        if (placeholder)
            placeholder->show();
        return false;
    }

    if (video_renderer_ && video_renderer_->parentWidget() != renderHost) {
        lay->removeWidget(video_renderer_);
        delete video_renderer_;
        video_renderer_ = nullptr;
    }
    if (!video_renderer_) {
        video_renderer_ = new QtGlVideoRenderer(renderHost);
        lay->addWidget(video_renderer_, 1);
    }
    video_renderer_->show();
    video_renderer_->init(0, 0);

    decoder_ = std::make_unique<DecodePipeline>();
    const int res = decoder_->start(playUrl.toStdString());
    if (res != 0) {
        qDebug() << "[MediaPipeline] decoder start failed, code =" << res;
        decoder_.reset();
        if (video_renderer_)
            video_renderer_->hide();
        if (placeholder)
            placeholder->show();
        return false;
    }

    audio_output_ = std::make_unique<SdlAudioOutput>(decoder_.get());
    if (!audio_output_->init()) {
        qWarning() << "[MediaPipeline] audio output init failed.";
        decoder_->stop();
        decoder_.reset();
        audio_output_.reset();
        if (video_renderer_)
            video_renderer_->hide();
        if (placeholder)
            placeholder->show();
        return false;
    }

    audio_output_->setAudioTimeBase(decoder_->audioTimeBase());
    audio_output_->flushPcmAndResetClock();
    pending_early_video_.reset();

    if (!pull_timer_) {
        pull_timer_ = new QTimer(this);
        connect(pull_timer_, &QTimer::timeout, this, &MediaPipeline::onPullVideoFrame);
    }
    paused_ = false;
    pull_timer_->start(33);
    audio_output_->pause(false);

    if (progress_timer)
        progress_timer->start(100);
    onUpdateProgressbar();

    return true;
}

void MediaPipeline::onPullVideoFrame()
{
    if (!decoder_ || !video_renderer_ || paused_)
        return;

    /*
     * 音频主时钟：SdlAudioOutput::masterClockMediaSeconds()
     *   ≈（首帧有效音频 PTS 秒）+（已混出去 PCM 字节 / 每秒字节）。
     *
     * 视频：pts → 秒后比较。
     *   • 太晚：丢（继续 pop 扔掉更陈旧者）。
     *   • 太早：挂 pending_early_video_，本周期不再往下 pop（FIFO 保序）。
     *   • 否则：可接受的帧里取「本条内最晚的」送去 present。
     *
     * 每轮定时器回调必须限制 try_pop 次数：若一次排空队列却只画最后一帧，
     * 等价于很短墙钟时间里跳过大量媒体时间，Seek 后主钟暂未建立时尤其会像「快进」。
     */

    constexpr double kLateDropSec = 0.18;
    constexpr double kEarlyHoldSec = 0.14;
    constexpr int kMaxPopsSynced = 5;
    constexpr int kMaxPopsNoClock = 1;

    const AVRational videoTb = decoder_->videoTimeBase();

    double audioNow = NAN;
    if (audio_output_)
        audioNow = audio_output_->masterClockMediaSeconds();

    const int maxPopsThisTick = std::isnan(audioNow) ? kMaxPopsNoClock : kMaxPopsSynced;
    int popCount = 0;

    AvFrameUniquePtr best;

    if (pending_early_video_) {
        const double pv = frameVideoPtsSeconds(pending_early_video_.get(), videoTb);

        if (std::isnan(audioNow) || std::isnan(pv))
            best = std::move(pending_early_video_);
        else if (pv + kLateDropSec < audioNow)
            pending_early_video_.reset();
        else if (pv <= audioNow + kEarlyHoldSec)
            best = std::move(pending_early_video_);
        else
            return;
    }

    AvFrameUniquePtr tmp;
    while (popCount < maxPopsThisTick && decoder_->try_pop_video_frame(tmp)) {
        ++popCount;
        const double vPts = frameVideoPtsSeconds(tmp.get(), videoTb);

        if (std::isnan(audioNow) || std::isnan(vPts)) {
            best = std::move(tmp);
            continue;
        }

        if (vPts + kLateDropSec < audioNow) {
            tmp.reset();
            continue;
        }

        if (vPts > audioNow + kEarlyHoldSec) {
            pending_early_video_ = std::move(tmp);
            break;
        }

        best = std::move(tmp);
    }

    if (best)
        video_renderer_->present(std::move(best));
}

void MediaPipeline::onUpdateProgressbar()
{
    if (!decoder_)
        return;
    const qint64 cur_time = static_cast<qint64>(positionMsFromAudio(audio_output_.get()));
    const qint64 total_time = static_cast<qint64>(decoder_->durationMs());
    emit sig_update_progressbar(cur_time, total_time);
}

void MediaPipeline::Pause(bool pause)
{
    paused_ = pause;
    if(progress_timer)
        progress_timer->stop();

    if (decoder_ && audio_output_ && !pause && progress_timer)
        progress_timer->start(100);

    if (pull_timer_)
        pull_timer_->stop();

    if (decoder_ && !pause)
        pull_timer_->start(33);

    if (audio_output_)
        audio_output_->pause(pause);

    if(decoder_)
        decoder_->pause(pause);
}

void MediaPipeline::Stop()
{
    if(progress_timer)
        progress_timer->stop();

    if (pull_timer_)
        pull_timer_->stop();

    pending_early_video_.reset();

    if (decoder_) {
        decoder_->stop();
        decoder_.reset();
    }

    if (video_renderer_)
        video_renderer_->shutdown();

    if (audio_output_) {
        audio_output_->shutdown();
        audio_output_.reset();
    }

    QWidget* host = render_host_.data();
    if (host) {
        if (auto* ph = host->findChild<QLabel*>(QStringLiteral("lb_video_placeholder")))
            ph->show();
        if (video_renderer_)
            video_renderer_->hide();
    }

    paused_ = false;
}

void MediaPipeline::SeekMs(qint64 posMs)
{
    if (audio_output_)
        audio_output_->flushPcmAndResetClock();
    pending_early_video_.reset();

    if (decoder_)
        (void)decoder_->seek(posMs);

    if (decoder_) {
        const qint64 cur_time = static_cast<qint64>(positionMsFromAudio(audio_output_.get()));
        const qint64 total_time = static_cast<qint64>(decoder_->durationMs());
        emit sig_update_progressbar(cur_time, total_time);
    }
}

void MediaPipeline::SetVolume(int vol)
{
    if (audio_output_)
        audio_output_->setVolume(vol);
}
