#ifndef MEDIAPLAYERPAGE_H
#define MEDIAPLAYERPAGE_H

#include <QDialog>
#include <QString>

namespace Ui {
class mediaplayerpage;
}

class QWidget;

class mediaplayerpage : public QDialog
{
    Q_OBJECT

public:
    explicit mediaplayerpage(QWidget *parent = nullptr);
    ~mediaplayerpage();
    void SetStatusText(const QString& text);
    void SetSessionText(const QString& text);
    void SetStreamText(const QString& text);
    void SetCurrentStream(const QString& streamId, const QString& playUrl);
    QString CurrentStreamId() const;
    QString CurrentPlayUrl() const;
    /** 与 MediaPipeline::StartPlay 的渲染父控件一致（*.ui 中 video_render_host） */
    QWidget* videoRenderHostWidget() const;
    /** 停止播放后将进度与时间标签复原 */
    void resetPlaybackTimelineUi();
    /** hasSession/decoding 是否可用；paused 为管线侧 MediaPipeline::Pause(true) */
    void updatePauseToggleUi(bool hasActiveSession, bool playbackPaused);

public slots:
    /** MediaPipeline::sig_update_progressbar → 刷新当前/总时长与滑块（拖拽期间不覆盖） */
    void syncProgressFromPipeline(qint64 positionMs, qint64 durationMs);

signals:
    void sig_ui_play_clicked();
    void sig_ui_pause_clicked();
    void sig_ui_stop_clicked();
    void sig_ui_seek_changed(int value);
    void sig_ui_volume_changed(int value);

private:
    QString formatSeconds(int sec) const;
private slots:
    void on_btn_play_clicked();
    void on_btn_pause_clicked();
    void on_btn_stop_clicked();
    void on_slider_volume_valueChanged(int value);
    void on_slider_progress_sliderMoved(int position);
    void on_slider_progress_sliderReleased();

private:
    QString _stream_id;
    QString _play_url;
    Ui::mediaplayerpage *ui;
    /** 拖拽进度条时不应用管线回写，避免与用户操作打架 */
    bool slider_progress_dragging_{ false };
};

#endif // MEDIAPLAYERPAGE_H
