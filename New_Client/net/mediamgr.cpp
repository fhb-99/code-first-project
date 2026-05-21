#include "mediamgr.h"
#include <QAbstractSocket>
#include <QDataStream>
#include <QDebug>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>

MediaMgr::MediaMgr():_host(""),_port(0),_b_recv_pending(false),_message_id(0),_message_len(0)
{
    _socket.setProxy(QNetworkProxy::NoProxy);

    QObject::connect(&_socket, &QTcpSocket::connected, [&]() {
        qDebug() << "Connected to media control server!";
        emit sig_con_success(true);
    });

    QObject::connect(&_socket, &QTcpSocket::readyRead, [&]() {
        _buffer.append(_socket.readAll());

        QDataStream stream(&_buffer, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_5_0);

        forever {
            if(!_b_recv_pending){
                if (_buffer.size() < static_cast<int>(sizeof(quint16) * 2)) {
                    return;
                }

                stream >> _message_id >> _message_len;
                _buffer = _buffer.mid(sizeof(quint16) * 2);
                qDebug() << "Media Message ID:" << _message_id << ", Length:" << _message_len;
            }

            if(_buffer.size() < _message_len){
                _b_recv_pending = true;
                return;
            }

            _b_recv_pending = false;
            QByteArray messageBody = _buffer.mid(0, _message_len);
            _buffer = _buffer.mid(_message_len);
            handleMsg(ReqId(_message_id), _message_len, messageBody);
        }
    });

    QObject::connect(&_socket, static_cast<void (QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error),
                     [&](QTcpSocket::SocketError socketError) {
        qDebug() << "Media Error:" << _socket.errorString();
        switch (socketError) {
        case QTcpSocket::ConnectionRefusedError:
        case QTcpSocket::HostNotFoundError:
        case QTcpSocket::SocketTimeoutError:
            emit sig_con_success(false);
            break;
        default:
            break;
        }
    });

    QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
        qDebug() << "Disconnected from media control server.";
    });

    QObject::connect(this, &MediaMgr::sig_send_data, this, &MediaMgr::slot_send_data);
    initHandlers();
}

MediaMgr::~MediaMgr()
{
}

void MediaMgr::initHandlers()
{
    _handlers.insert(ID_MEDIA_LIST_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is:" << id;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            emit sig_media_stream_list(QJsonArray());
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        qDebug() << "data jsonobj is" << jsonObj;

        if (!jsonObj.contains("error")) {
            qDebug() << "media list rsp missing error field";
            emit sig_media_stream_list(QJsonArray());
            return;
        }

        const int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            qDebug() << "media list rsp failed, error is" << error;
            emit sig_media_stream_list(QJsonArray());
            return;
        }

        QJsonArray streams;
        if (jsonObj.contains("media_list") && jsonObj["media_list"].isArray()) {
            streams = jsonObj["media_list"].toArray();
        } 
        else if (jsonObj.contains("media_play_info") && jsonObj["media_play_info"].isString()) {
            const QString url = jsonObj["media_play_info"].toString();
            if (!url.isEmpty()) {
                QJsonObject item;
                item["stream_id"] = QStringLiteral("cached_last");
                item["name"] = QStringLiteral("cached_last");
                item["url"] = url;
                streams.append(item);
            }
        }

        qDebug() << "media list count:" << streams.size();
        for (const auto& v : streams) {
            const QJsonObject item = v.toObject();
            qDebug() << " stream_id:" << item.value("stream_id").toString()
                     << " url:" << item.value("url").toString();
        }

        emit sig_media_stream_list(streams);
    });

    _handlers.insert(ID_MEDIA_SESSION_LIST_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is:" << id;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            emit sig_media_session_list(QJsonArray());
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            emit sig_media_session_list(QJsonArray());
            return;
        }

        const int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            qDebug() << "media session list rsp failed, error is" << error;
            emit sig_media_session_list(QJsonArray());
            return;
        }

        QJsonArray sessions;
        if (jsonObj.contains("sessions") && jsonObj["sessions"].isArray()) {
            sessions = jsonObj["sessions"].toArray();
        }
        emit sig_media_session_list(sessions);
    });

    _handlers.insert(ID_MEDIA_PLAY_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is:" << id;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            return;
        }

        const int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            qDebug() << "media play rsp failed, error is" << error;
            return;
        }


        emit sig_media_play_rsp(jsonObj);
    });

    _handlers.insert(ID_MEDIA_STOP_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is:" << id;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            return;
        }

        const int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            qDebug() << "media stop rsp failed, error is" << error;
            return;
        }

        emit sig_media_stop_rsp(jsonObj);
    });

    _handlers.insert(ID_MEDIA_SYNC_NOTIFY, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is:" << id;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        emit sig_media_sync_notify(jsonObj);
    });

    _handlers.insert(ID_MEDIA_CREATE_SESSION_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (!jsonDoc.isNull() && jsonDoc.isObject()) {
            emit sig_media_common_rsp(id, jsonDoc.object());
        }
    });

    _handlers.insert(ID_MEDIA_JOIN_SESSION_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (!jsonDoc.isNull() && jsonDoc.isObject()) {
            emit sig_media_common_rsp(id, jsonDoc.object());
        }
    });
}

void MediaMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    auto find_iter = _handlers.find(id);
    if(find_iter == _handlers.end()){
        qDebug() << "not found media id [" << id << "] to handle";
        return;
    }

    find_iter.value()(id, len, data);
}

void MediaMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug() << "receive media tcp connect signal";
    _host = si.Host;
    _port = static_cast<uint16_t>(si.Port.toUInt());
    _socket.connectToHost(si.Host, _port);
}

void MediaMgr::slot_send_data(ReqId reqId, QByteArray dataBytes)
{
    uint16_t id = reqId;
    quint16 len = static_cast<quint16>(dataBytes.length());

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << id << len;
    block.append(dataBytes);
    _socket.write(block);
    qDebug() << "media mgr send byte data is " << block;
}
