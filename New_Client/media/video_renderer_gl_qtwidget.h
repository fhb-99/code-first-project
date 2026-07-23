#ifndef VIDEO_RENDERER_GL_QTWIDGET_H
#define VIDEO_RENDERER_GL_QTWIDGET_H

#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <vector>

#include "ffmpeg_util.h"
#include "video_renderer.h"

/**
 * 嵌入 Qt 布局的视频渲染：父控件一般为 mediaplayerpage.ui 中的 video_render_host。
 *
 * swscale 将任意解码像素格式 **统一为平面 YUV420（AV_PIX_FMT_YUV420P）**，
 * 上传 Y/U/V 三张灰度纹理，在 fragment shader 中做 YUV→RGB（近似 BT.601）。
 * present() 须在控件所在线程调用（一般为 GUI 线程）。
 */
class QtGlVideoRenderer final : public QOpenGLWidget, protected QOpenGLFunctions, public VideoRenderer
{
    Q_OBJECT

public:
    explicit QtGlVideoRenderer(QWidget* parent = nullptr);
    ~QtGlVideoRenderer() override;

    bool init(int width, int height) override;
    void shutdown() override;
    void present(AvFrameUniquePtr frame) override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void releaseGlResources();
    bool rebuildSwsIfNeeded(const AVFrame* frame);
    void uploadYuv420Textures();

    QOpenGLShaderProgram prog_;
    GLuint tex_yuv_[3] = { 0, 0, 0 };

    SwsContext* sws_ctx_ = nullptr;
    int sws_in_w_ = 0;
    int sws_in_h_ = 0;
    AVPixelFormat sws_in_fmt_ = AV_PIX_FMT_NONE;

    int chroma_w_ = 0;
    int chroma_h_ = 0;

    std::vector<uint8_t> y_plane_;
    std::vector<uint8_t> u_plane_;
    std::vector<uint8_t> v_plane_;

    AvFrameUniquePtr pending_;
    AvFrameUniquePtr displayed_;

    bool gl_inited_ = false;
};

#endif // VIDEO_RENDERER_GL_QTWIDGET_H
