#ifndef VIDEO_RENDERER_GL_QTWIDGET_H
#define VIDEO_RENDERER_GL_QTWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include "video_renderer.h"

/**
 * 嵌入 Qt 布局的视频渲染区：父控件一般为 mediaplayerpage.ui 中的 video_render_host。
 * OpenGL 上下文由 QOpenGLWidget 管理，无需 SDL 另建窗口。
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
};

#endif // VIDEO_RENDERER_GL_QTWIDGET_H
