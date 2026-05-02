#include "decodepipeline.h"

#include <algorithm>

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
    :fmt_ctx(nullptr), audio_dec_ctx(nullptr),
     video_dec_ctx(nullptr), packet(nullptr),
     audio_frame(nullptr), video_frame(nullptr)
{

}

DecodePipeline::~DecodePipeline()
{
    close();
}


int DecodePipeline::open(const std::string& url)
{
    // 注册所有设备，设备的输入输出
    avdevice_register_all();
    // 初始化网络协议
    avformat_network_init();

    AVInputFormat * ifmt = nullptr;  // 通过这个变量知道是网络流、本地文件还是摄像头
    std::string ifile = url;
    if(url.rfind("video=", 0) == 0)
    {
        //当前是win下的摄像头，dshow
        ifmt = av_find_input_format("dshow");
        if(!ifmt)
        {
            qDebug() << "find input_format failed";
            return -5;
        }
    }
    else if(looksLikeNetworkUrl(url))
    {
        //网络流不需要手动指定格式
        ifmt = nullptr;
        //顺带再设置rtsp的传输方式，以及超时时间
        std::string tmp = toLower(url);
        if(tmp.rfind("rtsp://", 0) == 0)
        {
            //传输方式设为tcp
            av_dict_set(&fmt_opts, "rtsp_transport", "tcp", 0);
        }
        //设置网络超时5s
        av_dict_set(&fmt_opts, "stimeout", "5000000", 0);
    }
    else
    {
        //本地文件同样
        ifmt = nullptr;
    }

    //设置所有类型都可以通用的缓冲区, 2MB
    av_dict_set(&fmt_opts, "buffer_size", "2048000", 0);

    //分配并且初始化
    fmt_ctx = avformat_alloc_context();
    if(!fmt_ctx)
    {
        qDebug() << "alloc format-context failed";
        return -1;
    }


    //打开输入流，赋给context
    //虽然在上下文参数中设置了阻塞时间，但仍可能一直阻塞，所以可以通过中断回调来兜底
    int res = avformat_open_input(&fmt_ctx, url.c_str(), ifmt, &fmt_opts);
    if(res < 0)
    {
        qDebug() << "avformat open failed";
        return res;
    }

    //查找流信息
    res = avformat_find_stream_info(fmt_ctx, nullptr);
    if(res != 0)
    {
        qDebug() << "can't find stream info";
        return res;
    }

    //查找音频流、视频流、字幕流索引
    audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    subtitle_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, nullptr, 0);



}


void DecodePipeline::close()
{

}
