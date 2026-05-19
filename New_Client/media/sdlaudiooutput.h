#ifndef SDLAUDIOOUTPUT_H
#define SDLAUDIOOUTPUT_H

#include "ffmpeg_util.h"
#include "AVDecodeAbstract.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#include <QByteArray>
#include <QObject>
#include <atomic>
#include <mutex>
#include <thread>

/**
 * @brief SDL 音频输出（不接 SDL 窗口，只打开声卡）。
 *
 * 数据流（与文档《SDL处理音频-OpenGL处理视频.md》一致）：
 *   DecodePipeline 解码线程 → push_audio_frame → 队列
 *   → 本类里的 audioThreadLoop 用 try_pop_audio_frame 取 AVFrame
 *   → libswresample 转成与声卡一致的 PCM（声道布局 / 采样格式 / 采样率）
 *   → 写入 pcm_buffer_（字节流）
 *   → SDL 在独立「音频线程」里调 sdlAudioCallback，从 pcm_buffer_ 取数据填入 stream 播放。
 *
 * 线程约定：
 *   - audioThreadLoop：后台 std::thread，负责 swr + 写缓冲。
 *   - sdlAudioCallback：SDL 内部音频线程，负责读缓冲 + 混音到硬件 buffer。
 *   二者同时访问 pcm_buffer_，故必须用 pcm_mutex_ 保护。
 */
class SdlAudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit SdlAudioOutput(AVDecodeAbstract* decoder, QObject* parent = nullptr);
    ~SdlAudioOutput() override;

    bool init();
    void shutdown();

    // Stop/Play 生命周期：允许把“取帧+重采样”的工作线程停掉，下一次 Play 再拉起。
    // 这样 Stop=结束当前播放会话（业务语义更直观），同时避免重复初始化/退出 SDL 子系统。
    void startWorker();
    void stopWorker();

    // Stop/切歌场景：不销毁本类，不 close SDL 设备，只切换当前使用的解码器。
    // 允许传 nullptr，表示当前不从任何解码器取帧（线程会低频 sleep）。
    void setDecoder(AVDecodeAbstract* decoder) noexcept;
    // Stop 时用：确保音频线程不再使用旧 decoder 指针（避免 decoder 析构时 UAF 崩溃）。
    // timeoutMs 只是防止极端情况下卡死；正常应很快返回。
    void detachDecoderAndWait(int timeoutMs = 200) noexcept;

    void pause(bool paused);
    void setVolume(int volume); // UI 0–100，回调里会换算到 SDL_MIX_MAXVOLUME

    /** 设置音频 pts 所属的 time_base（与 DecodePipeline open 后的 audio 流一致） */
    void setAudioTimeBase(AVRational tb);

    /** 开始新一段播放或 Seek 前/后：重置主钟锚点与已播时长，并清空尚未混音的 PCM 队列 */
    void resetPlaybackClock();
    void flushPcmAndResetClock();

    /**
     * 以音频为主时钟时的「当前媒体时刻」（秒）。
     * 模型：首个有效音频 pts 转成秒 → 锚点；此后每从 pcm_buffer 真正混出去的字节按比例累加已过时间。
     * 锚点尚未建立时返回 NaN，调用方应按「无主钟」降级（例如无脑显示最新视频帧）。
     */
    [[nodiscard]] double masterClockMediaSeconds() const;

private:
    static void sdlAudioCallback(void* userdata, Uint8* stream, int len);
    void audioThreadLoop(); // 从解码队列取出 AVFrame 并重采样

    /** 按当前帧参数重建 SwrContext（格式/采样率/声道变了时必须重建） */
    bool recreateSwrIfNeeded(const AVFrame* frame);
    /** SDL 的声道数 → FFmpeg 的 channel_layout（int64_t 掩码） */
    [[nodiscard]] static int64_t sdlChannelsToAvLayout(int channels);
    /** SDL_AudioSpec.format（如 AUDIO_S16SYS）→ AVSampleFormat，供 swr 使用 */
    [[nodiscard]] static AVSampleFormat sdlFormatToAv(Uint16 sdlFormat);

private:
    std::atomic<AVDecodeAbstract*> decoder_{nullptr}; // 双队列在基类里，只读不拥有；允许 Stop 时置空
    std::atomic<bool> decoder_in_use_{false};

    SDL_AudioDeviceID audio_device_id_ = 0; // 0 表示未打开设备
    SwrContext* swr_ctx_ = nullptr;           // 重采样：解码帧格式 → SDL 设备格式

    QByteArray pcm_buffer_;            // 交错 PCM 字节队列（已对齐到设备格式）
    mutable std::mutex pcm_mutex_;   // 保护 pcm_buffer_：解码线程写、SDL 回调读

    int volume_ = 100;

    std::thread audio_thread_;
    std::atomic<bool> audio_quit_;   // true：要求 audioThreadLoop 退出
    std::atomic<bool> audio_paused_; // 与 SDL_PauseAudioDevice 一致；为 true 时回调直接 return

    SDL_AudioSpec desired_spec_{}; // 我们请求的参数（采样率/声道/格式）
    SDL_AudioSpec obtained_spec_{}; // 设备实际采用的参数（OpenAudio 后以此为准）

    bool sdl_audio_inited_{false}; // init() 成功后置 true；保证 shutdown() 幂等

    // 上一次 swr_init 时使用的「输入侧」参数，用于判断要不要重建 SwrContext
    //（FFmpeg 没有 swr_get_input_* 之类的 API，只能自己记）
    int64_t swr_in_layout_{ 0 };
    AVSampleFormat swr_in_fmt_{ AV_SAMPLE_FMT_NONE };
    int swr_in_rate_{ 0 };

    // 输出侧与 obtained_spec_ 对应，重建 swr 时也参与比较，避免 desired/设备被改后仍错误复用旧 ctx
    AVSampleFormat swr_out_fmt_{ AV_SAMPLE_FMT_NONE };
    int64_t swr_out_layout_{ 0 };

    // ----- 音频主时钟（仅供 MediaPipeline 与视频 PTS 对齐）-----

    mutable std::mutex clock_mutex_;      // 保护锚点与本机已累计播放时长
    AVRational audio_time_base_{0, 0};    // decode 管线里音频流的 time_base
    /** 第一段真正参与混音的逻辑：首帧有效音频 PTS 对应的媒体时间（秒） */
    double anchor_pts_media_sec_{0.0};
    /** 自锚点建立后，SDL 回调里实际混出去的字节所对应的秒数（设备输出节奏） */
    double playback_elapsed_since_anchor_sec_{0.0};
    bool anchor_valid_{false};

    /** 设备输出格式下「每秒占多少字节」，用于把回调里混出的 length 换成时间 */
    double output_bytes_per_sec_{0.0};
};

#endif // SDLAUDIOOUTPUT_H
