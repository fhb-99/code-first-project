#ifndef AVDECODEABSTRACT_H
#define AVDECODEABSTRACT_H

#include <memory>
#include <string>

#include "ffmpeg_util.h"
#include "framebuffer.h"


// 解码输出帧独占所有权封装：队列与消费端仅用 unique_ptr，避免与解码器重用的 AVFrame* 缓冲区冲突；
// 通常入队前应 av_frame_clone(解码器侧的 frame)。
struct AvFrameDeleter {
    void operator()(AVFrame* p) const noexcept
    {
        if (p) {
            AVFrame* f = p;
            av_frame_free(&f);
        }
    }
};

using AvFrameUniquePtr = std::unique_ptr<AVFrame, AvFrameDeleter>;

class AVDecodeAbstract
{
public:
    AVDecodeAbstract() = default;
    virtual ~AVDecodeAbstract() = default;

    AVDecodeAbstract(const AVDecodeAbstract&) = delete;
    AVDecodeAbstract& operator=(const AVDecodeAbstract&) = delete;

    virtual int start(const std::string& url) = 0;
    virtual void stop() = 0;
    virtual void pause(bool flag) = 0;
    virtual int resume() = 0;

    virtual int seek(int64_t ms) { return 0; }

    void push_video_frame(AvFrameUniquePtr frame);
    void push_audio_frame(AvFrameUniquePtr frame);

    AvFrameUniquePtr pop_video_frame();
    AvFrameUniquePtr pop_audio_frame();

    bool try_pop_video_frame(AvFrameUniquePtr& out);
    bool try_pop_audio_frame(AvFrameUniquePtr& out);

    /** 排空音/视频两路缓冲（常用于 stop / seek 前） */
    void clear_buf();

private:
    FrameBuffer<AvFrameUniquePtr> video_frame_buf_;
    FrameBuffer<AvFrameUniquePtr> audio_frame_buf_;
};

inline void AVDecodeAbstract::push_video_frame(AvFrameUniquePtr frame)
{
    if (frame)
        video_frame_buf_.push(std::move(frame));
}

inline void AVDecodeAbstract::push_audio_frame(AvFrameUniquePtr frame)
{
    if (frame)
        audio_frame_buf_.push(std::move(frame));
}

inline AvFrameUniquePtr AVDecodeAbstract::pop_video_frame()
{
    return video_frame_buf_.pop();
}

inline AvFrameUniquePtr AVDecodeAbstract::pop_audio_frame()
{
    return audio_frame_buf_.pop();
}

inline bool AVDecodeAbstract::try_pop_video_frame(AvFrameUniquePtr& out)
{
    return video_frame_buf_.try_pop(out);
}

inline bool AVDecodeAbstract::try_pop_audio_frame(AvFrameUniquePtr& out)
{
    return audio_frame_buf_.try_pop(out);
}

inline void AVDecodeAbstract::clear_buf()
{
    video_frame_buf_.clear();
    audio_frame_buf_.clear();
}

#endif // AVDECODEABSTRACT_H
