#include "decodepipeline.h"

#include <QDebug>
#include <algorithm>
#include <chrono>

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

void DecodePipeline::releasePacketAndFrames()
{
    if (packet != nullptr) {
        av_packet_free(&packet);
    }
    if (audio_frame != nullptr) {
        av_frame_free(&audio_frame);
    }
    if (video_frame != nullptr) {
        av_frame_free(&video_frame);
    }
}

void DecodePipeline::releaseDecoderContexts()
{
    if (audio_dec_ctx != nullptr) {
        avcodec_free_context(&audio_dec_ctx);
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

    // 1、注册所有设备，设备的输入输出
    avdevice_register_all();
    // 2、初始化网络协议
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
    fmt_ctx = nullptr;

    //5、打开输入流，赋给context
    //虽然在上下文参数中设置了阻塞时间，但仍可能一直阻塞，所以可以通过中断回调来兜底
    int res = avformat_open_input(&fmt_ctx, ifile.c_str(), ifmt, &fmt_opts);
    if (res < 0) {
        qDebug() << "avformat open failed" << res;
        releaseFmtOptsDict();
        releaseFormatContext();
        return res;
    }
    releaseFmtOptsDict();

    //6、查找流信息
    res = avformat_find_stream_info(fmt_ctx, nullptr);
    if (res != 0) {
        qDebug() << "can't find stream info" << res;
        releaseFormatContext();
        return res;
    }

    //7、查找音频流、视频流、字幕流索引
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

    //9、初始化解码器(现在默认是软解)
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

    //10、分配解码上下文
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

    //11、拷贝编码参数到解码上下文
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

    //12、打开解码器
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

    //13、分配数据包、帧内存
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
    if (decode_thread_.joinable()) {
        decode_thread_.join();
    }
}

void DecodePipeline::decode_loop_worker()
{
    qDebug() << "[DecodePipeline] decode thread enter";
    // TODO: av_read_frame -> send_packet/receive_frame，再推入音视频线程安全队列
    while (!decode_quit_.load(std::memory_order_acquire)) {
        if (fmt_ctx == nullptr) {
            break;
        }
        // 占位：实现真实读包解码后删掉 sleep（read_frame 会阻塞）
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    qDebug() << "[DecodePipeline] decode thread leave";
}

int DecodePipeline::start_decode_worker(const std::string& url)
{
    stop_decode_worker();

    int res = open(url);
    if (res != 0) {
        return res;
    }

    decode_quit_.store(false, std::memory_order_release);
    decode_thread_ = std::thread(&DecodePipeline::decode_loop_worker, this);
    return 0;
}

void DecodePipeline::stop_decode_worker()
{
    join_decode_worker();
    close();
}
