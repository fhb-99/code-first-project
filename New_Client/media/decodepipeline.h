#ifndef DECODEPIPELINE_H
#define DECODEPIPELINE_H

#include "ffmpeg_util.h"
#include "global.h"
#include <Thread>
#include <mutex>

// 负责解封装、解码

class DecodePipeline : public std::thread
{
public:
    DecodePipeline();
    ~DecodePipeline();


private:

    int open(const std::string& url);   //解封装 + 解码初始化
    void close();  //释放资源

    AVDictionary*   fmt_opts;         // 格式上下文参数（如RTSP传输方式、超时）
    AVDictionary*   codec_opts;       // 解码器参数（如帧引用计数）
    AVFormatContext * fmt_ctx;        //格式上下文
    AVCodecContext * audio_dec_ctx;   //音频解码上下文
    AVCodecContext * video_dec_ctx;   //视频解码上下文

    AVPacket * packet;  //解封装后得到的压缩数据包
    AVFrame * audio_frame;    //解码后的原始音频帧
    AVFrame * video_frame;    //解码后的原始视频帧

    // 流索引（区分视频/音频/字幕流）
    int video_stream_index;
    int audio_stream_index;
    int subtitle_stream_index;
};

#endif // DECODEPIPELINE_H
