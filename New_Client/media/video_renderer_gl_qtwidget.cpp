#include "video_renderer_gl_qtwidget.h"

#include <QDebug>
#include <QOpenGLShader>

namespace {

static const char* kVsGlsl120 = R"GLSL(
attribute vec2 a_pos;
attribute vec2 a_uv;
varying vec2 v_uv;
uniform mat4 u_mvp;
void main() {
    gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
    v_uv = a_uv;
}
)GLSL";

// YUV420P：三张独立纹理（全帧 UV 坐标采样），shader 内 BT.601 近似（领域 swing）
static const char* kFsYuv420Glsl120 = R"GLSL(
uniform sampler2D u_tex_y;
uniform sampler2D u_tex_u;
uniform sampler2D u_tex_v;
varying vec2 v_uv;
void main() {
    vec2 st = v_uv;
    float Y = texture2D(u_tex_y, st).r;
    float U = texture2D(u_tex_u, st).r - 0.5;
    float V = texture2D(u_tex_v, st).r - 0.5;
    float R = Y + 1.402 * V;
    float G = Y - 0.344136 * U - 0.714136 * V;
    float B = Y + 1.772 * U;
    gl_FragColor = vec4(clamp(vec3(R, G, B), 0.0, 1.0), 1.0);
}
)GLSL";

} // namespace

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
    return true;
}

void QtGlVideoRenderer::releaseGlResources()
{
    if (!gl_inited_)
        return;

    makeCurrent();

    if (tex_yuv_[0]) {
        glDeleteTextures(3, tex_yuv_);
        tex_yuv_[0] = tex_yuv_[1] = tex_yuv_[2] = 0;
    }
    prog_.removeAllShaders();

    sws_freeContext(sws_ctx_);
    sws_ctx_ = nullptr;
    sws_in_w_ = sws_in_h_ = 0;
    sws_in_fmt_ = AV_PIX_FMT_NONE;
    chroma_w_ = chroma_h_ = 0;

    y_plane_.clear();
    u_plane_.clear();
    v_plane_.clear();
    pending_.reset();
    displayed_.reset();

    doneCurrent();
    gl_inited_ = false;
}

void QtGlVideoRenderer::shutdown()
{
    releaseGlResources();
}

void QtGlVideoRenderer::present(AvFrameUniquePtr frame)
{
    if(!gl_inited_)
        return;
    if (!frame)
        return;
    pending_ = std::move(frame);
    update();
}

bool QtGlVideoRenderer::rebuildSwsIfNeeded(const AVFrame* frame)
{
    const int w = frame->width;
    const int h = frame->height;
    const AVPixelFormat fmt = static_cast<AVPixelFormat>(frame->format);

    if (w <= 0 || h <= 0 || fmt == AV_PIX_FMT_NONE) {
        qWarning() << "[QtGlVideoRenderer] invalid frame size/format";
        return false;
    }

    if (sws_ctx_ && sws_in_w_ == w && sws_in_h_ == h && sws_in_fmt_ == fmt)
        return true;

    sws_freeContext(sws_ctx_);
    sws_ctx_ = nullptr;

    sws_ctx_ = sws_getContext(w, h, fmt, w, h, AV_PIX_FMT_YUV420P,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws_ctx_) {
        qWarning() << "[QtGlVideoRenderer] sws_getContext failed fmt=" << fmt;
        sws_in_fmt_ = AV_PIX_FMT_NONE;
        return false;
    }

    sws_in_w_ = w;
    sws_in_h_ = h;
    sws_in_fmt_ = fmt;

    chroma_w_ = (w + 1) / 2;
    chroma_h_ = (h + 1) / 2;

    y_plane_.resize(static_cast<size_t>(w) * static_cast<size_t>(h));
    u_plane_.resize(static_cast<size_t>(chroma_w_) * static_cast<size_t>(chroma_h_));
    v_plane_.resize(static_cast<size_t>(chroma_w_) * static_cast<size_t>(chroma_h_));
    return true;
}

void QtGlVideoRenderer::uploadYuv420Textures()
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBindTexture(GL_TEXTURE_2D, tex_yuv_[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, sws_in_w_, sws_in_h_, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, y_plane_.data());

    glBindTexture(GL_TEXTURE_2D, tex_yuv_[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, chroma_w_, chroma_h_, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, u_plane_.data());

    glBindTexture(GL_TEXTURE_2D, tex_yuv_[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, chroma_w_, chroma_h_, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, v_plane_.data());

    glBindTexture(GL_TEXTURE_2D, 0);
}

void QtGlVideoRenderer::initializeGL()
{
    //加载OpenGL函数（必须最先被调用）
    initializeOpenGLFunctions();

    glDisable(GL_DEPTH_TEST);  //当前渲染是2D渲染，不需要深度测试，也就是3D的前后遮挡判断
    glDisable(GL_BLEND);       //关闭混合模式，视频画面不做透明叠加

    glGenTextures(3, tex_yuv_);  //因为当前设置了视频格式是YUV420,所以需要三个纹理平面
    if (!tex_yuv_[0]) {
        qWarning() << "[QtGlVideoRenderer] glGenTextures failed";
        return;
    }

    prog_.removeAllShaders();  //清除残留的着色器
    //添加顶点着色器
    if (!prog_.addShaderFromSourceCode(QOpenGLShader::Vertex, kVsGlsl120)) {
        qWarning() << "[QtGlVideoRenderer] vertex shader:" << prog_.log();
        return;
    }
    //片段着色器
    if (!prog_.addShaderFromSourceCode(QOpenGLShader::Fragment, kFsYuv420Glsl120)) {
        qWarning() << "[QtGlVideoRenderer] fragment shader:" << prog_.log();
        return;
    }
    //绑定顶点属性位置
    prog_.bindAttributeLocation("a_pos", 0);
    prog_.bindAttributeLocation("a_uv", 1);
    //确定链接成功
    if (!prog_.link()) {
        qWarning() << "[QtGlVideoRenderer] link:" << prog_.log();
        return;
    }

    gl_inited_ = true;
}

void QtGlVideoRenderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void QtGlVideoRenderer::paintGL()
{
    initializeOpenGLFunctions();

    glClearColor(0.06f, 0.08f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!gl_inited_ || !prog_.isLinked())
        return;

    if (pending_)
        displayed_ = std::move(pending_);

    if (!displayed_)
        return;

    AVFrame* fr = displayed_.get();
    if (!rebuildSwsIfNeeded(fr))
        return;

    uint8_t* dstSlice[4] = {
        y_plane_.data(),
        u_plane_.data(),
        v_plane_.data(),
        nullptr,
    };
    const int dstStride[4] = {
        sws_in_w_,
        chroma_w_,
        chroma_w_,
        0,
    };

    const int scaled = sws_scale(sws_ctx_, fr->data, fr->linesize, 0, sws_in_h_,
                               dstSlice, dstStride);
    if (scaled <= 0) {
        qWarning() << "[QtGlVideoRenderer] sws_scale failed";
        return;
    }

    uploadYuv420Textures();

    const float vw = static_cast<float>(sws_in_w_);
    const float vh = static_cast<float>(sws_in_h_);
    const float cw = static_cast<float>(width());
    const float ch = static_cast<float>(height());
    if (cw <= 0.f || ch <= 0.f)
        return;

    const float scale = qMin(cw / vw, ch / vh);
    const float dw = vw * scale;
    const float dh = vh * scale;
    const float x0 = (cw - dw) * 0.5f;
    const float y0 = (ch - dh) * 0.5f;
    const float x1 = x0 + dw;
    const float y1 = y0 + dh;

    QMatrix4x4 mvp;
    mvp.ortho(0.f, cw, ch, 0.f, -1.f, 1.f);

    const GLfloat verts[] = {
        x0, y0,  0.f, 0.f,
        x1, y0,  1.f, 0.f,
        x0, y1,  0.f, 1.f,
        x1, y1,  1.f, 1.f,
    };

    prog_.bind();
    prog_.setUniformValue("u_mvp", mvp);
    prog_.setUniformValue("u_tex_y", 0);
    prog_.setUniformValue("u_tex_u", 1);
    prog_.setUniformValue("u_tex_v", 2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_yuv_[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_yuv_[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex_yuv_[2]);

    prog_.enableAttributeArray(0);
    prog_.enableAttributeArray(1);
    prog_.setAttributeArray(0, GL_FLOAT, verts, 2, 4 * sizeof(GLfloat));
    prog_.setAttributeArray(1, GL_FLOAT, verts + 2, 2, 4 * sizeof(GLfloat));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    prog_.disableAttributeArray(1);
    prog_.disableAttributeArray(0);
    prog_.release();

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
