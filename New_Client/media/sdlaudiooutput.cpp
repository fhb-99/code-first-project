#include "sdlaudiooutput.h"

#include "global.h"

#include <QDebug>

#include <chrono>
#include <cmath>
#include <limits>

#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>

namespace {

/**
 * 从 AVFrame 取出「声道布局」的 64 位掩码，供 swr_alloc_set_opts 使用。
 * 旧版 FFmpeg：用 channel_layout；为 0 时用 channels 推断默认布局。
 */
[[nodiscard]] int64_t frameChannelLayoutMask(const AVFrame* f)
{
    if (!f)
        return 0;
    uint64_t layout = static_cast<uint64_t>(f->channel_layout);
    if (layout == 0 && f->channels > 0)
        layout = static_cast<uint64_t>(av_get_default_channel_layout(f->channels));
    return static_cast<int64_t>(layout);
}

/**
 * 估算一次 swr_convert 最多会产出多少「输出」采样点数，用于给输出缓冲区定上限。
 * - swr_get_delay：重采样器里还积压的输入延迟（按 in 的采样率计量）；
 * - av_rescale_rnd：把时间长度从输入采样率映射到输出采样率，向上取整防不够；
 * - +32：留一点余量，避免边界少算一帧。
 */
[[nodiscard]] int estimateOutSamples(SwrContext* swr,
                                     const AVFrame* in,
                                     int outSampleRate)
{
    if (!in || in->nb_samples <= 0 || !swr || in->sample_rate <= 0)
        return in ? in->nb_samples : 0;
    const int64_t delay = swr_get_delay(swr, in->sample_rate);
    return av_rescale_rnd(delay + static_cast<int64_t>(in->nb_samples),
                          outSampleRate, in->sample_rate, AV_ROUND_UP)
           + 32;
}

/**
 * 解码后 AVFrame 上与「呈现时刻」对应的 ticks（单位 = 调用方传入的 time_base）。
 * 优先 pts；没有则 best_effort_timestamp（部分流只有其一有效）。
 */
[[nodiscard]] int64_t framePresentationPtsTicks(const AVFrame* f)
{
    if (!f)
        return AV_NOPTS_VALUE;
    if (f->pts != AV_NOPTS_VALUE)
        return f->pts;
    return av_frame_get_best_effort_timestamp(f);
}

/** pts（以 tb 为单位的刻度）→ 媒体时间轴上的秒（与音视频对比用同一套） */
[[nodiscard]] double ptsTicksToSeconds(int64_t pts, AVRational tb)
{
    if (pts == AV_NOPTS_VALUE || tb.den <= 0)
        return std::numeric_limits<double>::quiet_NaN();
    return static_cast<double>(pts) * av_q2d(tb);
}

} // namespace

SdlAudioOutput::SdlAudioOutput(AVDecodeAbstract* decoder, QObject* parent)
    : QObject(parent)
    , decoder_(decoder)
    , audio_quit_(true) // true 表示「尚未启动工作线程」或「已停止」；见 init/shutdown
    , audio_paused_(true)
{
}

SdlAudioOutput::~SdlAudioOutput()
{
    shutdown();
}

void SdlAudioOutput::setDecoder(AVDecodeAbstract* decoder) noexcept
{
    decoder_.store(decoder, std::memory_order_release);
}

void SdlAudioOutput::detachDecoderAndWait(int timeoutMs) noexcept
{
    decoder_.store(nullptr, std::memory_order_release);
    const auto start = std::chrono::steady_clock::now();
    while (decoder_in_use_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (timeoutMs > 0) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now() - start)
                                     .count();
            if (elapsed >= timeoutMs)
                break;
        }
    }
}

int64_t SdlAudioOutput::sdlChannelsToAvLayout(int channels)
{
    if (channels <= 0)
        return static_cast<int64_t>(AV_CH_LAYOUT_STEREO);
    if (channels == 1)
        return static_cast<int64_t>(AV_CH_LAYOUT_MONO);
    if (channels == 2)
        return static_cast<int64_t>(AV_CH_LAYOUT_STEREO);
    // 多声道：交给 libavutil 按声道数给默认 bitmask
    return static_cast<int64_t>(av_get_default_channel_layout(channels));
}

AVSampleFormat SdlAudioOutput::sdlFormatToAv(Uint16 sdlFormat)
{
    switch (sdlFormat) {
        case AUDIO_S16SYS:
            return AV_SAMPLE_FMT_S16; // 交错 S16，与 swr 输出一致
        case AUDIO_F32SYS:
            return AV_SAMPLE_FMT_FLT; // 交错 float
        default:
            return AV_SAMPLE_FMT_NONE; // 需在 init 里拒绝，否则无法配置 swr
    }
}

bool SdlAudioOutput::recreateSwrIfNeeded(const AVFrame* frame)
{
    if (!frame)
        return false;

    // 解码器输出的「输入」参数
    const int64_t inLayout = frameChannelLayoutMask(frame);
    const AVSampleFormat inFmt = static_cast<AVSampleFormat>(frame->format);
    const int inRate = frame->sample_rate;

    if (inLayout == 0 || inRate <= 0 || inFmt == AV_SAMPLE_FMT_NONE) {
        qWarning() << "[SdlAudioOutput] invalid input frame audio params layout/rate/format";
        return false;
    }

    // 「输出」必须与 SDL 实际拿到的 obtained_spec_ 一致，否则播出来是噪声或崩溃
    const int64_t outLayout = sdlChannelsToAvLayout(obtained_spec_.channels);
    const AVSampleFormat outFmt = swr_out_fmt_;
    const int outRate = obtained_spec_.freq;

    // 输入、输出都没变则复用 SwrContext，避免每帧 alloc/free
    const bool unchanged = swr_ctx_
                           && swr_in_layout_ == inLayout && swr_in_fmt_ == inFmt
                           && swr_in_rate_ == inRate && swr_out_layout_ == outLayout
                           && swr_out_fmt_ == outFmt;
    if (unchanged)
        return true;

    swr_free(&swr_ctx_);
    // 经典 API：out_* 是「声卡要的数据」，in_* 是「解码器给我们的数据」
    swr_ctx_ = swr_alloc_set_opts(nullptr,
                                  outLayout, outFmt, outRate,
                                  inLayout, inFmt, inRate,
                                  0, nullptr);

    if (!swr_ctx_ || swr_init(swr_ctx_) < 0) {
        qWarning() << "[SdlAudioOutput] swr_alloc_set_opts/swr_init failed";
        swr_free(&swr_ctx_);
        swr_ctx_ = nullptr;
        return false;
    }

    swr_in_layout_ = inLayout;
    swr_in_fmt_ = inFmt;
    swr_in_rate_ = inRate;
    swr_out_layout_ = outLayout;
    swr_out_fmt_ = outFmt;

    return true;
}

bool SdlAudioOutput::init()
{
    // 只初始化 SDL 音频子系统，不要 SDL_INIT_VIDEO（与「不建 SDL 窗口」一致）
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qWarning() << "[SdlAudioOutput] SDL_Init(SDL_INIT_AUDIO) failed:" << SDL_GetError();
        return false;
    }
    sdl_audio_inited_ = true;

    // desired：希望声卡用的参数；具体能否满足看 obtained_spec_
    desired_spec_.freq = 44100;
    desired_spec_.format = AUDIO_S16SYS;
    desired_spec_.channels = 2;
    desired_spec_.samples = 1024; // 单次回调期望处理的「帧」大小（样本数维度），设备可能微调
    desired_spec_.callback = sdlAudioCallback;
    desired_spec_.userdata = this;

    // 第 4 个参数 0：允许 SDL 在 obtained 里返回与 desired 不完全一致的最终参数
    audio_device_id_ = SDL_OpenAudioDevice(nullptr, 0, &desired_spec_, &obtained_spec_, 0);
    if (audio_device_id_ == 0) {
        qWarning() << "[SdlAudioOutput] SDL_OpenAudioDevice failed:" << SDL_GetError();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        sdl_audio_inited_ = false;
        return false;
    }

    swr_out_fmt_ = sdlFormatToAv(obtained_spec_.format);
    if (swr_out_fmt_ == AV_SAMPLE_FMT_NONE) {
        qWarning() << "[SdlAudioOutput] Unsupported SDL_AudioSpec.format from device";
        SDL_CloseAudioDevice(audio_device_id_);
        audio_device_id_ = 0;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    swr_out_layout_ = sdlChannelsToAvLayout(obtained_spec_.channels);

    // 「每秒可从设备缓冲区混出去的字节数」——用于把 SDL 回调里实际混出去的 length 换成「过了多少播放时间」
    {
        const int bitsPerChan = SDL_AUDIO_BITSIZE(obtained_spec_.format);
        const int bytesPerSnap = obtained_spec_.channels * (bitsPerChan / 8); // 一个采样瞬间、所有声道
        output_bytes_per_sec_ = double(obtained_spec_.freq) * double(bytesPerSnap);
        if (output_bytes_per_sec_ <= 0.0)
            output_bytes_per_sec_ = 1.0;
    }

    resetPlaybackClock();

    // 第一帧到来前还不知道解码器的 inLayout/inFmt/inRate，此处不创建 swr_ctx_
    swr_ctx_ = nullptr;
    swr_in_layout_ = 0;
    swr_in_fmt_ = AV_SAMPLE_FMT_NONE;
    swr_in_rate_ = 0;

    qDebug() << "[SdlAudioOutput] SDL audio:"
             << "freq" << obtained_spec_.freq << "ch" << obtained_spec_.channels
             << "samples" << obtained_spec_.samples;

    // 启动「取帧 + 重采样」线程；真正开始播放由 MediaPipeline 调用 pause(false)
    startWorker();

    return true;
}

void SdlAudioOutput::startWorker()
{
    if (!sdl_audio_inited_)
        return;
    if (audio_thread_.joinable())
        return;
    audio_quit_.store(false, std::memory_order_release);
    audio_paused_.store(true, std::memory_order_release);
    decoder_in_use_.store(false, std::memory_order_release);
    audio_thread_ = std::thread(&SdlAudioOutput::audioThreadLoop, this);
}

void SdlAudioOutput::stopWorker()
{
    // Stop 只停工作线程，不关闭 SDL 设备/子系统；下次 Play 可 startWorker() 复用。
    audio_paused_.store(true, std::memory_order_release);
    if (audio_device_id_ != 0)
        SDL_PauseAudioDevice(audio_device_id_, 1);

    detachDecoderAndWait();

    if (!audio_thread_.joinable())
        return;
    audio_quit_.store(true, std::memory_order_release);
    audio_thread_.join();
    decoder_in_use_.store(false, std::memory_order_release);
}

void SdlAudioOutput::shutdown()
{
    if (!sdl_audio_inited_)
        return;

    // 先停 SDL 回调线程，避免 CloseAudioDevice 时 userdata 悬空回调
    audio_paused_.store(true, std::memory_order_release);
    if (audio_device_id_ != 0)
        SDL_PauseAudioDevice(audio_device_id_, 1);

    // 先停工作线程，不要再往 pcm_buffer_ 里写，再关声卡（回调随后也停）
    stopWorker();

    swr_free(&swr_ctx_);
    swr_ctx_ = nullptr;
    swr_in_layout_ = 0;
    swr_in_fmt_ = AV_SAMPLE_FMT_NONE;
    swr_in_rate_ = 0;
    swr_out_layout_ = 0;
    swr_out_fmt_ = AV_SAMPLE_FMT_NONE;

    if (audio_device_id_ != 0) {
        // 锁住设备，确保回调不会并发访问 pcm_buffer_ / clock
        SDL_LockAudioDevice(audio_device_id_);
        {
            std::lock_guard<std::mutex> lock(pcm_mutex_);
            pcm_buffer_.clear();
        }
        SDL_UnlockAudioDevice(audio_device_id_);

        SDL_CloseAudioDevice(audio_device_id_);
        audio_device_id_ = 0;
    }

    // 在 Windows + 部分声卡驱动上，SDL_QuitSubSystem(SDL_INIT_AUDIO) 偶发触发竞态崩溃
    //（即便已经 Pause/Close 过设备）。这里把 Quit 推迟到进程退出时再做（或干脆不做），
    // Stop/切歌场景只需 CloseAudioDevice 即可。
    sdl_audio_inited_ = false;

    resetPlaybackClock();
}

void SdlAudioOutput::setAudioTimeBase(AVRational tb)
{
    std::lock_guard<std::mutex> lock(clock_mutex_);
    audio_time_base_ = tb;
}

void SdlAudioOutput::resetPlaybackClock()
{
    std::lock_guard<std::mutex> lock(clock_mutex_);
    anchor_valid_ = false;
    anchor_pts_media_sec_ = 0.0;
    playback_elapsed_since_anchor_sec_ = 0.0;
}

void SdlAudioOutput::flushPcmAndResetClock()
{
    {
        std::lock_guard<std::mutex> lock(pcm_mutex_);
        pcm_buffer_.clear();
    }
    resetPlaybackClock();
}

double SdlAudioOutput::masterClockMediaSeconds() const
{
    std::lock_guard<std::mutex> lock(clock_mutex_);
    if (!anchor_valid_)
        return std::numeric_limits<double>::quiet_NaN();
    return anchor_pts_media_sec_ + playback_elapsed_since_anchor_sec_;
}

void SdlAudioOutput::pause(bool paused)
{
    audio_paused_.store(paused, std::memory_order_release);
    // 1 = 静音/暂停设备拉流；0 = 开始播放。与 audio_paused_ 双重保险（回调里也会判断原子变量）
    if (audio_device_id_ != 0)
        SDL_PauseAudioDevice(audio_device_id_, paused ? 1 : 0);
}

void SdlAudioOutput::setVolume(int volume)
{
    volume_ = qBound(0, volume, 100);
}

void SdlAudioOutput::sdlAudioCallback(void* userdata, Uint8* stream, int len)
{
    auto* self = static_cast<SdlAudioOutput*>(userdata);
    if (!self || self->audio_paused_.load(std::memory_order_acquire))
        return;

    // len：SDL 本帧要求填满的字节数（可能大于当前攒下的 PCM）
    SDL_memset(stream, 0, len);

    int amount = 0;
    {
        std::lock_guard<std::mutex> lock(self->pcm_mutex_);
        if (self->pcm_buffer_.isEmpty())
            return;

        amount = qMin(len, self->pcm_buffer_.size());
        const int mixVol = SDL_MIX_MAXVOLUME * self->volume_ / 100;
        SDL_MixAudioFormat(stream, reinterpret_cast<const Uint8*>(self->pcm_buffer_.constData()),
                           self->obtained_spec_.format, amount, mixVol);
        self->pcm_buffer_.remove(0, amount);
    }

    // 主钟：仅统计「已混到声卡缓冲区」的字节 → 换算成媒体时间轴上「已播过的时长」。
    // 须在释放 pcm_mutex_ 后再锁 clock_mutex_，全局避免 (clock → pcm) 与 (pcm → clock) 交错死锁。
    if (amount > 0 && self->output_bytes_per_sec_ > 0.0) {
        std::lock_guard<std::mutex> clockLock(self->clock_mutex_);
        if (self->anchor_valid_)
            self->playback_elapsed_since_anchor_sec_
                += static_cast<double>(amount) / self->output_bytes_per_sec_;
    }
}

void SdlAudioOutput::audioThreadLoop()
{
    qDebug() << "[SdlAudioOutput] Audio thread enter";
    while (!audio_quit_.load(std::memory_order_acquire)) {
        // pause=true 时 MediaPipeline 已 PauseAudioDevice；这里少取帧，降低空转占 CPU
        if (audio_paused_.load(std::memory_order_acquire)) {
            decoder_in_use_.store(false, std::memory_order_release);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        AVDecodeAbstract* decoder = decoder_.load(std::memory_order_acquire);
        if (!decoder) {
            decoder_in_use_.store(false, std::memory_order_release);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        decoder_in_use_.store(true, std::memory_order_release);
        AvFrameUniquePtr frame;
        if (!decoder->try_pop_audio_frame(frame)) {
            decoder_in_use_.store(false, std::memory_order_release);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        decoder_in_use_.store(false, std::memory_order_release);

        AVFrame* f = frame.get();
        if (!f || !recreateSwrIfNeeded(f))
            continue;

        // 为本次转换分配足够大的「交错」输出缓冲；大小按采样率换算 + 延迟
        const int dstMax = estimateOutSamples(swr_ctx_, f, obtained_spec_.freq);
        uint8_t* outBuf = nullptr;
        const int allocRet = av_samples_alloc(&outBuf, nullptr, obtained_spec_.channels,
                                                dstMax, swr_out_fmt_, 0);
        if (allocRet < 0 || !outBuf) {
            qWarning() << "[SdlAudioOutput] av_samples_alloc failed" << allocRet;
            continue;
        }

        // 平面格式解码：f->data[] 为多指针；swr 会按 inFmt 解释
        const uint8_t* inPlanes[AV_NUM_DATA_POINTERS] = {};
        for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i)
            inPlanes[i] = f->data[i];

        // 输出写进单块 outBuf（对 S16 立体声即为 LRLRLR... 交错）
        const int converted = swr_convert(swr_ctx_, &outBuf, dstMax, inPlanes, f->nb_samples);
        if (converted < 0) 
        {
            qWarning() << "[SdlAudioOutput] swr_convert failed" << converted;
            av_freep(&outBuf);
            continue;
        }

        int outBytes = av_samples_get_buffer_size(nullptr, obtained_spec_.channels, converted,
                                                  swr_out_fmt_, 1);
        if (outBytes <= 0) 
        {
            qWarning() << "[SdlAudioOutput] av_samples_get_buffer_size failed" << outBytes;
            av_freep(&outBuf);
            continue;
        }

        // 若队列顶太满，扔掉旧 PCM 并让主钟在下一帧重新锚定，避免滞后无限拉大
        bool backlogCleared = false;
        {
            constexpr int kMaxPcmBacklog = 2 * 1024 * 1024;
            std::lock_guard<std::mutex> lock(pcm_mutex_);
            if (pcm_buffer_.size() >= kMaxPcmBacklog) 
            {
                pcm_buffer_.clear();
                backlogCleared = true;
                qWarning() << "[SdlAudioOutput] pcm backlog overflow, cleared";
            }
        }
        if (backlogCleared)
            resetPlaybackClock();

        // 主钟锚点：第一块塞进缓冲的 PCM 对应「本条 AVFrame」起始的媒体时刻（秒）
        // + SDL 回调里按「已输出字节」累加 elapsed ⇒ masterClockMediaSeconds()
        {
            std::lock_guard<std::mutex> clk(clock_mutex_);
            if (!anchor_valid_ && audio_time_base_.den > 0) {
                const int64_t ticks = framePresentationPtsTicks(f);
                const double sec = ptsTicksToSeconds(ticks, audio_time_base_);
                if (!std::isnan(sec)) {
                    anchor_pts_media_sec_ = sec;
                    anchor_valid_ = true;
                    playback_elapsed_since_anchor_sec_ = 0.0;
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(pcm_mutex_);
            pcm_buffer_.append(reinterpret_cast<char*>(outBuf), outBytes);
        }
        av_freep(&outBuf);
    }
    qDebug() << "[SdlAudioOutput] Audio thread leave";
}
