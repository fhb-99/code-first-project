#include "streamcontroller.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "tcpmgr.h"

StreamController::StreamController(QObject *parent)
    : QObject(parent)
{
    auto tcp = TcpMgr::GetInstance().get();
    connect(tcp, &TcpMgr::sig_media_stream_list, this, &StreamController::slot_on_media_stream_list);
    connect(tcp, &TcpMgr::sig_media_session_list, this, &StreamController::slot_on_media_session_list);
    connect(tcp, &TcpMgr::sig_media_play_rsp, this, &StreamController::slot_on_media_play_rsp);
    connect(tcp, &TcpMgr::sig_media_stop_rsp, this, &StreamController::slot_on_media_stop_rsp);
    connect(tcp, &TcpMgr::sig_media_sync_notify, this, &StreamController::slot_on_media_sync_notify);
    connect(tcp, &TcpMgr::sig_media_common_rsp, this, &StreamController::slot_on_media_common_rsp);
}

void StreamController::RequestStreamList(const QString &keyword)
{
    QJsonObject obj;
    obj["keyword"] = keyword;
    emit TcpMgr::GetInstance()->sig_send_data(ID_MEDIA_LIST_REQ, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void StreamController::RequestSessionList()
{
    QJsonObject obj;
    emit TcpMgr::GetInstance()->sig_send_data(ID_MEDIA_SESSION_LIST_REQ, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void StreamController::CreateSession(const QString &sessionName)
{
    QJsonObject obj;
    obj["session_name"] = sessionName;
    emit TcpMgr::GetInstance()->sig_send_data(ID_MEDIA_CREATE_SESSION_REQ, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void StreamController::JoinSession(const QString &sessionId)
{
    QJsonObject obj;
    obj["session_id"] = sessionId;
    emit TcpMgr::GetInstance()->sig_send_data(ID_MEDIA_JOIN_SESSION_REQ, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void StreamController::PlayStream(const QString &streamId, const QString &sessionId)
{
    QJsonObject obj;
    obj["stream_id"] = streamId;
    obj["session_id"] = sessionId;
    emit TcpMgr::GetInstance()->sig_send_data(ID_MEDIA_PLAY_REQ, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void StreamController::StopStream(const QString &sessionId)
{
    QJsonObject obj;
    obj["session_id"] = sessionId;
    emit TcpMgr::GetInstance()->sig_send_data(ID_MEDIA_STOP_REQ, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void StreamController::slot_on_media_stream_list(QJsonArray streams)
{
    emit sig_streams_updated(streams);
}

void StreamController::slot_on_media_session_list(QJsonArray sessions)
{
    emit sig_sessions_updated(sessions);
}

void StreamController::slot_on_media_play_rsp(QJsonObject obj)
{
    if (obj.value("error").toInt(-1) != 0) {
        emit sig_status(QStringLiteral("播放请求失败"));
        return;
    }

    const QString streamId = obj.value("stream_id").toString();
    const QString playUrl = obj.value("play_url").toString();
    const QString sessionId = obj.value("session_id").toString();
    emit sig_play_started(streamId, playUrl, sessionId);
}

void StreamController::slot_on_media_stop_rsp(QJsonObject obj)
{
    if (obj.value("error").toInt(-1) == 0) {
        emit sig_play_stopped();
    } else {
        emit sig_status(QStringLiteral("停止播放失败"));
    }
}

void StreamController::slot_on_media_sync_notify(QJsonObject obj)
{
    if (obj.value("action").toString() != "play") {
        return;
    }
    emit sig_sync_play(obj.value("stream_id").toString(),
                       obj.value("play_url").toString(),
                       obj.value("session_id").toString());
}

void StreamController::slot_on_media_common_rsp(ReqId id, QJsonObject obj)
{
    Q_UNUSED(id);
    const int err = obj.value("error").toInt(0);
    const QString msg = obj.value("msg").toString();
    if (err != 0) {
        emit sig_status(msg.isEmpty() ? QStringLiteral("媒体请求失败") : msg);
    }
}

