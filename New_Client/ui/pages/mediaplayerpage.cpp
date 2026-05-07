#include "mediaplayerpage.h"
#include "ui_mediaplayerpage.h"

#include <climits>
#include <QSlider>
#include <QWidget>
#include <QtGlobal>

mediaplayerpage::mediaplayerpage(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::mediaplayerpage)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Widget);
    ui->lb_time_current->setText(QStringLiteral("00:00:00"));
    ui->lb_time_total->setText(QStringLiteral("00:00:00"));
    ui->slider_progress->setMinimum(0);
    ui->slider_progress->setMaximum(1);
    ui->slider_progress->setValue(0);
    ui->slider_volume->setValue(60);

    connect(ui->slider_progress, &QSlider::sliderPressed, [this]() {
        slider_progress_dragging_ = true;
    });
    connect(ui->slider_progress, &QSlider::sliderReleased, [this]() {
        slider_progress_dragging_ = false;
    });
}

mediaplayerpage::~mediaplayerpage()
{
    delete ui;
}

void mediaplayerpage::SetStatusText(const QString& text)
{
    ui->lb_status_text->setText(text);
}

void mediaplayerpage::SetSessionText(const QString& text)
{
    ui->lb_session->setText(text);
}

void mediaplayerpage::SetStreamText(const QString& text)
{
    ui->lb_stream_text->setText(text);
}

void mediaplayerpage::SetCurrentStream(const QString& streamId, const QString& playUrl)
{
    _stream_id = streamId;
    _play_url = playUrl;
    SetStreamText(QStringLiteral("Stream: %1").arg(streamId.isEmpty() ? QStringLiteral("-") : streamId));
}

QString mediaplayerpage::CurrentStreamId() const
{
    return _stream_id;
}

QString mediaplayerpage::CurrentPlayUrl() const
{
    return _play_url;
}

QWidget* mediaplayerpage::videoRenderHostWidget() const
{
    return ui->video_render_host;
}

QString mediaplayerpage::formatSeconds(int sec) const
{
    if (sec < 0)
        sec = 0;
    const int h = sec / 3600;
    const int m = (sec % 3600) / 60;
    const int s = sec % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}

void mediaplayerpage::resetPlaybackTimelineUi()
{
    slider_progress_dragging_ = false;
    ui->slider_progress->blockSignals(true);
    ui->slider_progress->setMinimum(0);
    ui->slider_progress->setMaximum(1);
    ui->slider_progress->setValue(0);
    ui->lb_time_current->setText(QStringLiteral("00:00:00"));
    ui->lb_time_total->setText(QStringLiteral("00:00:00"));
    ui->slider_progress->blockSignals(false);
}

void mediaplayerpage::syncProgressFromPipeline(qint64 positionMs, qint64 durationMs)
{
    if (slider_progress_dragging_)
        return;

    const int posSec = static_cast<int>(qBound<qint64>(0LL, positionMs / 1000,
                                                      static_cast<qint64>(INT_MAX)));

    ui->slider_progress->blockSignals(true);

    if (durationMs <= 0) {
        ui->slider_progress->setMinimum(0);
        ui->slider_progress->setMaximum(1);
        ui->slider_progress->setValue(0);
        ui->lb_time_total->setText(QStringLiteral("--:--"));
    } else {
        const int durSec = static_cast<int>(qBound<qint64>(1LL, durationMs / 1000,
                                                           static_cast<qint64>(INT_MAX)));
        ui->slider_progress->setMinimum(0);
        ui->slider_progress->setMaximum(durSec);
        ui->slider_progress->setValue(qMin(posSec, durSec));
        ui->lb_time_total->setText(formatSeconds(durSec));
    }

    ui->lb_time_current->setText(formatSeconds(posSec));

    ui->slider_progress->blockSignals(false);
}

void mediaplayerpage::on_btn_play_clicked()
{
    SetStatusText(QStringLiteral("Status: Requesting Play…"));
    emit sig_ui_play_clicked();
}

void mediaplayerpage::on_btn_pause_clicked()
{
    SetStatusText(QStringLiteral("Status: Paused"));
    emit sig_ui_pause_clicked();
}

void mediaplayerpage::on_btn_stop_clicked()
{
    SetStatusText(QStringLiteral("Status: Requesting Stop…"));
    emit sig_ui_stop_clicked();
}

void mediaplayerpage::on_slider_volume_valueChanged(int value)
{
    emit sig_ui_volume_changed(value);
}

void mediaplayerpage::on_slider_progress_sliderMoved(int position)
{
    ui->lb_time_current->setText(formatSeconds(position));
}

void mediaplayerpage::on_slider_progress_sliderReleased()
{
    emit sig_ui_seek_changed(ui->slider_progress->value());
}
