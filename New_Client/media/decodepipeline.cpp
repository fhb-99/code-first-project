#include "decodepipeline.h"
#include <QDebug>
#include <algorithm>
#include <system_error>

#include <libavutil/mathematics.h>
#include <libavutil/rational.h>

namespace {

// FFmpeg 新式解码循环（与旧的 avcodec_decode_video2 / packet 单次解码不同）：
//   avcodec_send_packet() 投喂压缩 AVPacket；
//   avcodec_receive_frame() 取出解压后的 AVFrame。
// “一包多帧”“多包一帧”都常见：每次 send 后应用 receive 循环抽到 EAGAIN/EOF，
// send 若在内部缓冲区满时会返回 AVERROR(EAGAIN)，须在重试前先 receive 排空输出。

constexpr char kVidTag[] = "video";
constexpr char kAudTag[] = "audio";

static void ffmpegLogErr(const char *where, int err)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
    av_strerror(err, buf, sizeof(buf)); // FFmpeg 错误码转可读字符串，便于日志排查
    qDebug() << "[DecodePipeline]" << where << "err:" << err << buf;
}

// FFmpeg interrupt callback：用于打断 av_read_frame 等阻塞调用，让 Stop/Seek 时 join 能及时返回。
static int ffmpegInterruptCb(void* opaque)
{
    auto * self = static_cast<DecodePipeline *>(opaque);
    if(!self)
    {
        return 0;
    }
    return self->decode_quit_.load(std::memory_order_acquire) ? 1 : 0;
}


// 解码器复用的 frm 不能直接入队：
// clone 后以 unique_ptr 交给音/视频两路队列，再释放本轮 decoder 侧的引用
// flag 为 true，代表入对的是视频帧。false，则为音频帧
static void push_frame_to_buffer(AVDecodeAbstract * out, AVFrame * frm, bool flag)
{
    AVFrame * frame = av_frame_clone(frm);
    if(!frame) {
        ffmpegLogErr(flag ? kVidTag : kAudTag, AVERROR(ENOMEM));
        av_frame_unref(frm);
        return;
    }
    av_frame_unref(frm);

    AvFrameUniquePtr target(frame);
    if(flag) {
        out->push_video_frame(std::move(target));
    }
    else {
        out->push_audio_frame(std::move(target));
    }
}

// 反复调用 avcodec_receive_frame：把当前解码器里已能输出的帧全部取尽。
// 返回值：0 表示得到一帧；AVERROR(EAGAIN) 表示需继续 send 压缩包才有新帧；
//         AVERROR_EOF 表示已无更多输出（常用于 flush 之后或解码结束）
static void drain_decoder_output(AVDecodeAbstract * out, AVCodecContext *ctx,
                                 AVFrame *frm, bool flag, const char *tag)
{
    for (;;) {
        const int r = avcodec_receive_frame(ctx, frm); // FFmpeg 解码输出 API
        if (r == AVERROR(EAGAIN) || r == AVERROR_EOF)
            return;
        if (r < 0) {
            ffmpegLogErr(tag, r);
            return;
        }
        
        push_frame_to_buffer(out, frm, flag);
    }
}

// avcodec_send_packet：把 demux 得到的压缩包交给解码器；pkt 可为 nullptr 且在 flush 阶段表示冲刷。
// 若内部已满则 AVERROR(EAGAIN)，必须先 drain 再重试 send，否则会死锁式卡住。
static bool send_packet_retry(AVDecodeAbstract * out, AVCodecContext *ctx, AVFrame *frm,
                              AVPacket *pkt, bool flag, const char *tag)
{
    for (;;) {
        const int sendRes = avcodec_send_packet(ctx, pkt); // FFmpeg 解码投递 API
        if (sendRes >= 0)
            return true;
        if (sendRes == AVERROR(EAGAIN)) {
            drain_decoder_output(out, ctx, frm, flag, tag);
            continue;
        }
        ffmpegLogErr(tag, sendRes);
        return false;
    }
}

// flush：解封装读到 EOF（或要主动结束一路解码）时对 avcodec：
//   avcodec_send_packet(ctx, nullptr) 通知解码器不再有输入；
//   再反复 avcodec_receive_frame 直到 AVERROR_EOF，把延迟在内部的帧全部收出。
static void flush_decoder(AVDecodeAbstract * out, AVCodecContext *ctx,
                          AVFrame *frm, bool flag, const char *tag)
{
    int flushSend = avcodec_send_packet(ctx, nullptr); // nullptr packet = 解码器冲刷信号
    if (flushSend < 0 && flushSend != AVERROR_EOF) {
        if (flushSend == AVERROR(EAGAIN)) {
            drain_decoder_output(out, ctx, frm, flag, tag);
            flushSend = avcodec_send_packet(ctx, nullptr);
        }
        if (flushSend < 0 && flushSend != AVERROR_EOF) {
            ffmpegLogErr(tag, flushSend);
            return;
        }
    }
    for (;;) {
        const int r = avcodec_receive_frame(ctx, frm);
        if (r == AVERROR_EOF)
            break;
        if (r == AVERROR(EAGAIN))
            continue;
        if (r < 0) {
            ffmpegLogErr(tag, r);
            break;
        }
        
        push_frame_to_buffer(out, frm, flag);
    }
}

} // namespace

//将字符串大写全部转为小写
static std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

//判断s是不是网络地址
static bool looksLikeNetworkUrl(const std::string& s)
{
    const std::string t = toLower(s);
    //HTTP/RTSP 协议头本身是不区分大小写的，但 C++ 字符串 rfind() 区分大小写！
    return t.rfind("rtsp://", 0) == 0
        || t.rfind("http://", 0) == 0
        || t.rfind("https://", 0) == 0
        || t.rfind("rtmp://", 0) == 0;
}

DecodePipeline::DecodePipeline()
    : fmt_opts(nullptr), fmt_ctx(nullptr), audio_dec_ctx(nullptr),
      video_dec_ctx(nullptr), packet(nullptr),
      audio_frame(nullptr), video_frame(nullptr),
      video_stream_index(-1), audio_stream_index(-1),
      subtitle_stream_index(-1),
      video_time_base_num(0), video_time_base_den(0),
      audio_time_base_num(0), audio_time_base_den(0),
      decode_quit_(true)
{
}

DecodePipeline::~DecodePipeline()
{
    stop_decode_worker();
}

void DecodePipeline::pause(bool flag)
{
    video_pause.store(flag, std::memory_order_release);
}

/**
 * Seek 到毫秒位置（相对媒体起始）。
 *
 * 步骤概要：
 *   1) 停读包线程（join_decode_worker），避免与 demux/dec 并行。
 *   2) 清空音视频输出队列 clear_buf。
 *   3) av_seek_frame 将 demux 读指针移到目标附近（时间戳转成「视频轨」time_base）。
 *   4) avcodec_flush_buffers 丢弃解码器内部残留，与新位置一致。
 *   5) 重新拉起 decode_loop_worker。
 *
 * 直播/不可 seek 的音源上会失败（返回负数），仍会重启线程以保持播放不断。
 */
int DecodePipeline::seek(int64_t ms)
{
    if (!fmt_ctx || video_stream_index < 0 || !video_dec_ctx || !audio_dec_ctx)
        return -1;

    if (ms < 0)
        ms = 0;

    join_decode_worker();

    clear_buf();

    AVStream* vst = fmt_ctx->streams[video_stream_index];
    // UI 毫秒 → 微秒，再换算到所选视频轨的时间基刻度（与 av_seek_frame 要求一致）
    const AVRational usTb = av_make_q(1, AV_TIME_BASE);
    const int64_t microseconds = ms * INT64_C(1000);
    const int64_t tsInStreamTb = av_rescale_q(microseconds, usTb, vst->time_base);

    int seekRet = av_seek_frame(fmt_ctx, video_stream_index, tsInStreamTb, AVSEEK_FLAG_BACKWARD);
    if (seekRet >= 0) {
        avcodec_flush_buffers(audio_dec_ctx);
        avcodec_flush_buffers(video_dec_ctx);
    } else {
        ffmpegLogErr("av_seek_frame", seekRet);
    }

    decode_quit_.store(false, std::memory_order_release);
    decode_thread_ = std::thread(&DecodePipeline::decode_loop_worker, this);

    return seekRet;
}

void DecodePipeline::releasePacketAndFrames()
{
    if (packet != nullptr) {
        av_packet_free(&packet); // 释放 AVPacket 结构体及其内部 data 引用
    }
    if (audio_frame != nullptr) {
        av_frame_free(&audio_frame); // 释放 AVFrame 及侧数据、引用计数缓冲
    }
    if (video_frame != nullptr) {
        av_frame_free(&video_frame);
    }
}

void DecodePipeline::releaseDecoderContexts()
{
    if (audio_dec_ctx != nullptr) {
        avcodec_free_context(&audio_dec_ctx); // 释放解码器状态与关联缓冲
    }
    if (video_dec_ctx != nullptr) {
        avcodec_free_context(&video_dec_ctx);
    }
}

void DecodePipeline::releaseFormatContext()
{
    if (fmt_ctx != nullptr) {
        avformat_close_input(&fmt_ctx);
        // close_input 内部会释放并成功后将 *指针 置为 nullptr，无须再调用 avformat_free_context
        fmt_ctx = nullptr;
    }
}

void DecodePipeline::releaseFmtOptsDict()
{
    if (fmt_opts != nullptr) {
        av_dict_free(&fmt_opts);
    }
}

void DecodePipeline::close()
{
    releasePacketAndFrames();
    releaseDecoderContexts();
    releaseFormatContext();
    releaseFmtOptsDict();
}

int DecodePipeline::open(const std::string& url)
{
    releasePacketAndFrames();
    releaseDecoderContexts();
    releaseFormatContext();
    releaseFmtOptsDict();

    // 1、libavdevice：注册 dshow 等输入设备（仅在使用摄像头等设备源时需要）
    avdevice_register_all();
    // 2、libavformat：网络协议初始化（RTSP/HTTP 等 URL 打开前调用一次）
    avformat_network_init();

    //3、判断数据流是文件还是网络流还是摄像头，来为AVInputFormat变量赋值，后续为打开输入流设置
    AVInputFormat* ifmt = nullptr;
    std::string ifile = url;
    if (url.rfind("video=", 0) == 0) {
        //当前是win下的摄像头，dshow
        ifmt = av_find_input_format("dshow");
        if (!ifmt) {
            qDebug() << "find input_format failed";
            releaseFmtOptsDict();
            return -5;
        }
    } else if (looksLikeNetworkUrl(url)) {
        //网络流不需要手动指定格式
        ifmt = nullptr;
        //顺带再设置rtsp的传输方式，以及超时时间
        std::string tmp = toLower(url);
        if (tmp.rfind("rtsp://", 0) == 0) {
            //传输方式设为tcp
            av_dict_set(&fmt_opts, "rtsp_transport", "tcp", 0);
        }
        //设置网络超时5s
        av_dict_set(&fmt_opts, "stimeout", "5000000", 0);
    } else {
        //本地文件同样
        ifmt = nullptr;
    }

    //设置所有类型都可以通用的缓冲区, 2MB
    av_dict_set(&fmt_opts, "buffer_size", "2048000", 0);

    //4、分配并且初始化（格式上下文由下方 avformat_open_input 一并创建并打开）
    fmt_ctx = avformat_alloc_context();
    if(!fmt_ctx) {
        releaseFmtOptsDict();
        return -5;
    }
    //设置中断函数
    fmt_ctx->interrupt_callback.callback = &ffmpegInterruptCb;
    fmt_ctx->interrupt_callback.opaque = this;

    // 5、avformat_open_input：创建并打开输入（文件/URL/设备），填充 AVFormatContext
    // 虽可在 AVDictionary 里设超时，仍可能长时间阻塞，生产环境建议配 interrupt_callback
    int res = avformat_open_input(&fmt_ctx, ifile.c_str(), ifmt, &fmt_opts);
    if (res < 0) {
        ffmpegLogErr("avformat_open_input", res);
        releaseFmtOptsDict();
        releaseFormatContext();
        return res;
    }
    releaseFmtOptsDict();

    // 6、avformat_find_stream_info：读取包头/试探解码，填满各 AVStream（时长、time_base、codecpar 等）
    res = avformat_find_stream_info(fmt_ctx, nullptr);
    if (res != 0) {
        qDebug() << "can't find stream info" << res;
        releaseFormatContext();
        return res;
    }

    // 7、av_find_best_stream：在已打开的 fmt_ctx 里按类型挑选“主”流，得到 stream_index
    audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    subtitle_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, nullptr, 0);

    if (audio_stream_index < 0) {
        qDebug() << "can not find audio stream";
        releaseFormatContext();
        return -10;
    }
    if (video_stream_index < 0) {
        qDebug() << "can not find video stream";
        releaseFormatContext();
        return -10;
    }
    if (subtitle_stream_index < 0) {
        qDebug() << "can not find subtitle stream";
    }

    //8、获取音频、视频信息(时间基、编码参数)
    AVStream* audio_stream = fmt_ctx->streams[audio_stream_index];
    AVStream* video_stream = fmt_ctx->streams[video_stream_index];

    //先缓存音频和视频的时间基
    audio_time_base_num = audio_stream->time_base.num;
    audio_time_base_den = audio_stream->time_base.den;
    video_time_base_num = video_stream->time_base.num;
    video_time_base_den = video_stream->time_base.den;

    //获得音频、视频得编码参数
    AVCodecParameters* audio_codec_param = audio_stream->codecpar;
    AVCodecParameters* video_codec_param = video_stream->codecpar;

    // 9、avcodec_find_decoder：由 codec_id 查找已链接进程序的解码器 AVCodec（软解示例）
    AVCodec* audio_codec = nullptr;
    AVCodec* video_codec = nullptr;
    audio_codec = avcodec_find_decoder(audio_codec_param->codec_id);
    if (!audio_codec) {
        qDebug() << "can not find audio_codec: " << avcodec_get_name(audio_codec_param->codec_id);
        releaseFormatContext();
        return -15;
    }
    video_codec = avcodec_find_decoder(video_codec_param->codec_id);
    if (!video_codec) {
        qDebug() << "can not find video_codec: " << avcodec_get_name(video_codec_param->codec_id);
        releaseFormatContext();
        return -15;
    }

    // 10、avcodec_alloc_context3：为指定 AVCodec 分配 AVCodecContext（真正执行解码的状态机）
    audio_dec_ctx = avcodec_alloc_context3(audio_codec);
    if (!audio_dec_ctx) {
        qDebug() << "alloc audio codec context failed";
        releaseFormatContext();
        return -20;
    }
    video_dec_ctx = avcodec_alloc_context3(video_codec);
    if (!video_dec_ctx) {
        qDebug() << "alloc video codec context failed";
        releaseDecoderContexts();
        releaseFormatContext();
        return -20;
    }

    // 11、avcodec_parameters_to_context：容器里的 codecpar → 解码器上下文（宽高、声道、extradata 等）
    int audio_res = avcodec_parameters_to_context(audio_dec_ctx, audio_codec_param);
    if (audio_res != 0) {
        qDebug() << "audio avcodec_parameters_to_context error: " << audio_res;
        releaseDecoderContexts();
        releaseFormatContext();
        return audio_res;
    }
    int video_res = avcodec_parameters_to_context(video_dec_ctx, video_codec_param);
    if (video_res != 0) {
        qDebug() << "video avcodec_parameters_to_context error: " << video_res;
        releaseDecoderContexts();
        releaseFormatContext();
        return video_res;
    }

    // 12、avcodec_open2：用选定的 codec 初始化硬件/线程选项等后真正开启解码实例
    audio_res = avcodec_open2(audio_dec_ctx, audio_codec, nullptr);
    if (audio_res != 0) {
        qDebug() << "open audio codec failed";
        releaseDecoderContexts();
        releaseFormatContext();
        return audio_res;
    }
    video_res = avcodec_open2(video_dec_ctx, video_codec, nullptr);
    if (video_res) {
        qDebug() << "open video codec failed";
        releaseDecoderContexts();
        releaseFormatContext();
        return video_res;
    }

    // 13、av_packet_alloc / av_frame_alloc：读包解码循环中重复使用的缓冲区（各一路 packet + 音视频各一帧）
    packet = av_packet_alloc();
    if (!packet) {
        qDebug() << "av_packet_alloc failed";
        releaseDecoderContexts();
        releaseFormatContext();
        return -25;
    }
    audio_frame = av_frame_alloc();
    if (!audio_frame) {
        qDebug() << "av_frame_alloc(audio) failed";
        releasePacketAndFrames();
        releaseDecoderContexts();
        releaseFormatContext();
        return -25;
    }
    video_frame = av_frame_alloc();
    if (!video_frame) {
        qDebug() << "av_frame_alloc(video) failed";
        releasePacketAndFrames();
        releaseDecoderContexts();
        releaseFormatContext();
        return -25;
    }

    return 0;
}

void DecodePipeline::join_decode_worker()
{
    decode_quit_.store(true, std::memory_order_release);
    // 必须在 join 前 clear：音频停掉后队列满，解码线程会阻塞在 push()；
    // clear 会 notify 条件变量，让线程醒来看到 decode_quit_ 后退出。
    clear_buf();
    if (!decode_thread_.joinable())
        return;
    try {
        decode_thread_.join();
    } catch (const std::system_error& e) {
        qWarning() << "[DecodePipeline] decode_thread_.join failed:" << e.what();
    }
}

void DecodePipeline::decode_loop_worker()
{
    qDebug() << "[DecodePipeline] decode thread enter";
    // 解复用 + 解码主循环：av_read_frame 取一条压缩包 → 按 stream_index 送给对应 avcodec
    // → send_packet_retry + drain_decoder_output；demux EOF 后对音/视频分别 flush_decoder。
    // 在 drain / flush 成功得到 AVFrame 后推入线程安全队列（需 clone/ref 再 unref 本帧缓冲）。
    while (!decode_quit_.load(std::memory_order_acquire)) {
        if (fmt_ctx == nullptr) {
            break;
        }
        if(video_pause.load(std::memory_order_acquire)) {
            // 如果ui点击了暂停按钮，也就是为true，在这里continue，避免执行读包函数
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // av_read_frame：从 AVFormatContext 读下一包压缩数据写入 packet（含 stream_index、pts 等）
        const int readRes = av_read_frame(fmt_ctx, packet);
        if (readRes < 0) {
            // 流结束：先 flush 两路解码器，再退出循环（与文件播完、部分网络流一致）
            if (readRes == AVERROR_EOF) {
                flush_decoder(this, video_dec_ctx, video_frame, true, kVidTag);
                flush_decoder(this, audio_dec_ctx, audio_frame, false, kAudTag);
                qDebug() << "[DecodePipeline] demux EOF";
                break;
            }
            // 被信号等打断：丢弃本包引用后重试读包
            if (readRes == AVERROR(EINTR)) {
                av_packet_unref(packet); // av_packet_unref：递减 packet 内部 buffer 引用，可复用 packet 外壳
                continue;
            }
            if (readRes == AVERROR_EXIT) {
                av_packet_unref(packet);
                break;
            }
            ffmpegLogErr("av_read_frame", readRes);
            break;
        }

        // 仅处理当前 open() 选中的主视频/音频轨；字幕等其它轨直接跳过并释放引用
        if (packet->stream_index == video_stream_index) {
            if (!send_packet_retry(this, video_dec_ctx, video_frame, packet, true, kVidTag)) {
                av_packet_unref(packet);
                break;
            }
            av_packet_unref(packet); // send 成功后压缩数据已由解码器引用或拷贝处理完，立即释放 packet 缓冲
            drain_decoder_output(this, video_dec_ctx, video_frame, true, kVidTag); // 本包可能对应多帧，须循环 receive
        } else if (packet->stream_index == audio_stream_index) {
            if (!send_packet_retry(this, audio_dec_ctx, audio_frame, packet, false, kAudTag)) {
                av_packet_unref(packet);
                break;
            }
            av_packet_unref(packet);
            drain_decoder_output(this, audio_dec_ctx, audio_frame, false, kAudTag);
        } else {
            av_packet_unref(packet);
        }
    }
    qDebug() << "[DecodePipeline] decode thread leave";
}

int DecodePipeline::start_decode_worker(const std::string& url)
{
    stop_decode_worker(); // 先结束旧线程并 close，避免重复 start 泄漏

    // stop_decode_worker() 会把 decode_quit_ 置 true；open() 内 interrupt_callback 会读该标志。
    // 若不在 open 前清零，avformat_open_input 会立刻以 AVERROR_EXIT 失败（常被误读成“找不到协议”）。
    decode_quit_.store(false, std::memory_order_release);

    int res = open(url); // open 内完成 demux + codec 打开与 packet/frame 分配（当前仍在调用线程执行）
    if (res != 0) {
        return res;
    }

    decode_thread_ = std::thread(&DecodePipeline::decode_loop_worker, this);
    return 0;
}

void DecodePipeline::stop_decode_worker()
{
    join_decode_worker(); // decode_quit_ → clear_buf(唤醒阻塞 push) → join
    close();
    qDebug() << "[DecodePipeline] decode thread leave";
}

int64_t DecodePipeline::durationMs() const
{
    if(!fmt_ctx) {
        return -1;
    }
    //判断输入流是否是直播流，因为直播流duration是AV_NOPTS_VALUE未知，表示无限
    if(fmt_ctx->duration == AV_NOPTS_VALUE) {
        return -1;
    }
    int64_t total_milliseconds = fmt_ctx->duration / (AV_TIME_BASE / 1000); // 毫秒
    if(total_milliseconds <= 0) {
        return -1;
    }
    return total_milliseconds;
}
