#ifndef MEDIAPLAYERPAGE_H
#define MEDIAPLAYERPAGE_H

#include <QDialog>
#include <QString>

namespace Ui {
class mediaplayerpage;
}

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
};

#endif // MEDIAPLAYERPAGE_H
