#ifndef STREAMCONTROLLER_H
#define STREAMCONTROLLER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include "usermgr.h"
#include "global.h"

class StreamController : public QObject
{
    Q_OBJECT
public:
    explicit StreamController(QObject *parent = nullptr);

    void RequestStreamList(const QString& keyword);
    void RequestSessionList();
    void CreateSession(const QString& sessionName);
    void JoinSession(const QString& sessionId);
    void PlayStream(const QString& streamId, const QString& sessionId);
    void StopStream(const QString& sessionId);

private:
    int _uid;

signals:
    void sig_streams_updated(QJsonArray streams);
    void sig_sessions_updated(QJsonArray sessions);
    void sig_play_started(QString streamId, QString playUrl, QString sessionId);
    void sig_play_stopped();
    void sig_sync_play(QString streamId, QString playUrl, QString sessionId);
    void sig_status(QString statusText);

private slots:
    void slot_on_media_stream_list(QJsonArray streams);
    void slot_on_media_session_list(QJsonArray sessions);
    void slot_on_media_play_rsp(QJsonObject obj);
    void slot_on_media_stop_rsp(QJsonObject obj);
    void slot_on_media_sync_notify(QJsonObject obj);
    void slot_on_media_common_rsp(ReqId id, QJsonObject obj);
};

#endif // STREAMCONTROLLER_H

