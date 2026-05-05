#include "video_renderer_gl_qtwidget.h"

#include <QDebug>

QtGlVideoRenderer::QtGlVideoRenderer(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMinimumHeight(1);
}

QtGlVideoRenderer::~QtGlVideoRenderer()
{
    shutdown();
}

bool QtGlVideoRenderer::init(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
    qDebug() << "[QtGlVideoRenderer] init — parent 建议为 video_render_host，再交给布局拉伸";
    return true;
}

void QtGlVideoRenderer::shutdown() {}

void QtGlVideoRenderer::present(AvFrameUniquePtr frame)
{
    if (!frame)
        return;
    Q_UNUSED(frame);
    // TODO: 缓存待显示帧 / 或生成纹理后 update()
    update();
}

void QtGlVideoRenderer::initializeGL()
{
    initializeOpenGLFunctions();
}

void QtGlVideoRenderer::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void QtGlVideoRenderer::paintGL()
{
    glClearColor(0.06f, 0.08f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}
