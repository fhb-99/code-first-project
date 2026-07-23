#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H

#include "AVDecodeAbstract.h"

/**
 * 视频渲染抽象：与解码层解耦。MediaPipeline 从解码队列取出 AvFrameUniquePtr 后送入 present()。
 *
 * 工程内播放器 UI 已为视频区预留 QWidget（objectName: video_render_host）。
 * 实现类：QtGlVideoRenderer（OpenGL / QOpenGLWidget，见 video_renderer_gl_qtwidget）。
 * 音频不走本接口；由 SDL_Audio（详见文档《SDL处理音频-OpenGL处理视频.md》）单独处理。
 */
class VideoRenderer
{
public:
    VideoRenderer() = default;
    virtual ~VideoRenderer() = default;

    VideoRenderer(const VideoRenderer&) = delete;
    VideoRenderer& operator=(const VideoRenderer&) = delete;

    /** width/height 可为 0，表示沿用流第一帧尺寸（实现类自行约定） */
    virtual bool init(int width, int height) = 0;
    virtual void shutdown() = 0;

    /** 消费一整帧所有权；若在内部异步使用需自行 clone/move */
    virtual void present(AvFrameUniquePtr frame) = 0;
};

#endif // VIDEO_RENDERER_H
