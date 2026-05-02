#include "mediaplayerpage.h"
#include "ui_mediaplayerpage.h"
#include <QTime>

mediaplayerpage::mediaplayerpage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::mediaplayerpage)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Widget);
    ui->lb_time_current->setText("00:00:00");
    ui->lb_time_total->setText("00:00:00");
    ui->slider_progress->setValue(0);
    ui->slider_volume->setValue(60);
}

mediaplayerpage::~mediaplayerpage()
{
    delete ui;
}

void mediaplayerpage::SetStatusText(const QString &text)
{
    ui->lb_status_text->setText(text);
}

void mediaplayerpage::SetSessionText(const QString &text)
{
    ui->lb_session->setText(text);
}

void mediaplayerpage::SetStreamText(const QString &text)
{
    ui->lb_stream_text->setText(text);
}

void mediaplayerpage::SetCurrentStream(const QString &streamId, const QString &playUrl)
{
    _stream_id = streamId;
    _play_url = playUrl;
    SetStreamText(QString("Stream: %1").arg(streamId.isEmpty() ? "-" : streamId));
}

QString mediaplayerpage::CurrentStreamId() const
{
    return _stream_id;
}

QString mediaplayerpage::CurrentPlayUrl() const
{
    return _play_url;
}

QString mediaplayerpage::formatSeconds(int sec) const
{
    int h = sec / 3600;
    int m = (sec % 3600) / 60;
    int s = sec % 60;
    return QString("%1:%2:%3")
            .arg(h, 2, 10, QLatin1Char('0'))
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0'));
}

void mediaplayerpage::on_btn_play_clicked()
{
    SetStatusText("Status: Requesting Play...");
    emit sig_ui_play_clicked();
}

void mediaplayerpage::on_btn_pause_clicked()
{
    SetStatusText("Status: Paused");
    emit sig_ui_pause_clicked();
}

void mediaplayerpage::on_btn_stop_clicked()
{
    SetStatusText("Status: Requesting Stop...");
    emit sig_ui_stop_clicked();
}

void mediaplayerpage::on_slider_volume_valueChanged(int value)
{
    emit sig_ui_volume_changed(value);
}

void mediaplayerpage::on_slider_progress_sliderMoved(int position)
{
    const int sec = position;
    ui->lb_time_current->setText(formatSeconds(sec));
}

void mediaplayerpage::on_slider_progress_sliderReleased()
{
    emit sig_ui_seek_changed(ui->slider_progress->value());
}
