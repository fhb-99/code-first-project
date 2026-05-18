#ifndef DECODEPIPELINE_H
#define DECODEPIPELINE_H

#include "ffmpeg_util.h"
#include "AVDecodeAbstract.h"
#include "global.h"
#include <atomic>
#include <thread>

// 负责解封装、解码

class DecodePipeline : public AVDecodeAbstract
{
public:
    DecodePipeline();
    virtual ~DecodePipeline();

    virtual int start(const std::string& url) {
        return start_decode_worker(url);
    }

    virtual void stop() {
        stop_decode_worker();
    }

    virtual void pause(bool flag);

    virtual int resume() { return 0; }

    virtual int seek(int64_t ms);

    /** 视频流的 time_base，用于 PTS → 秒的换算（音视频同步时用） */
    AVRational videoTimeBase() const
    {
        return {video_time_base_num, video_time_base_den};
    }
    /** 音频流的 time_base，与音频帧 pts 成套使用 */
    AVRational audioTimeBase() const { return {audio_time_base_num, audio_time_base_den}; }

    // open(url) + 在工作线程跑后续读包/解码（open_input/find_stream_info 仍会阻塞调用方）
    int start_decode_worker(const std::string& url);
    // 置退出标志 → join 线程 → 释放 FFmpeg（可重复调用）
    void stop_decode_worker();

    //封装总时长，ms， 未知或者无效为-1；
    int64_t durationMs() const;

private:
    int open(const std::string& url);   //解封装 + 解码初始化

    void close();  //仅释放 FFmpeg 资源（调用前必须已停止工作线程）
    void join_decode_worker();

    // open() 出错时按进度分别调用下列释放函数，避免直接调用 close()
    void releasePacketAndFrames();
    void releaseDecoderContexts();
    void releaseFormatContext();
    void releaseFmtOptsDict();

    // 工作线程入口：在此处实现 av_read_frame / send_packet / receive_frame …
    void decode_loop_worker();

    AVDictionary*       fmt_opts;         // 格式上下文参数（如RTSP传输方式、超时）
    //AVDictionary*       codec_opts;       // 解码器参数（如帧引用计数）
    AVFormatContext*    fmt_ctx;          //格式上下文
    AVCodecContext*     audio_dec_ctx;    //音频解码上下文
    AVCodecContext*     video_dec_ctx;    //视频解码上下文

    AVPacket* packet;   //解封装后得到的压缩数据包
    AVFrame* audio_frame; //解码后的原始音频帧
    AVFrame* video_frame; //解码后的原始视频帧

    int video_stream_index;   // 流索引（区分视频/音频/字幕流）
    int audio_stream_index;
    int subtitle_stream_index;

    // 时间基，FFmpeg时间戳转换：时间戳(ms) = pts * time_base.num / time_base.den * 1000）
    int video_time_base_num;
    int video_time_base_den;
    int audio_time_base_num;
    int audio_time_base_den;

    std::thread       decode_thread_;
    std::atomic<bool> decode_quit_;       // true 表示要求工作线程退出
    std::atomic<bool> video_pause{false};       // true 表示暂停（但是工作线程不停，只是停止读包）
};

#endif // DECODEPIPELINE_H
