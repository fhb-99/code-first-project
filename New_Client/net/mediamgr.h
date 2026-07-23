#ifndef MEDIAMGR_H
#define MEDIAMGR_H

#include <QTcpSocket>
#include "singleton.h"
#include "global.h"
#include <functional>
#include <QObject>
#include <memory>
#include <QJsonArray>
#include <QMap>

class MediaMgr: public QObject, public Singleton<MediaMgr>,
        public std::enable_shared_from_this<MediaMgr>
{
    Q_OBJECT
public:
    ~MediaMgr();
private:
    friend class Singleton<MediaMgr>;
    MediaMgr();
    void initHandlers();
    void handleMsg(ReqId id, int len, QByteArray data);
    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    QByteArray _buffer;
    bool _b_recv_pending;
    quint16 _message_id;
    quint16 _message_len;
    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;
public slots:
    void slot_tcp_connect(ServerInfo);
    void slot_send_data(ReqId reqId, QByteArray data);
signals:
    void sig_con_success(bool bsuccess);
    void sig_send_data(ReqId reqId, QByteArray data);
    void sig_media_stream_list(QJsonArray streams);
    void sig_media_session_list(QJsonArray sessions);
    void sig_media_play_rsp(QJsonObject obj);
    void sig_media_stop_rsp(QJsonObject obj);
    void sig_media_sync_notify(QJsonObject obj);
    void sig_media_common_rsp(ReqId id, QJsonObject obj);
};

#endif // MEDIAMGR_H
