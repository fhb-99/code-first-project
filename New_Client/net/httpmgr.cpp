#include "httpmgr.h"

#include <QDebug>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

HttpMgr::HttpMgr()
{
    // Queued signal makes it safe across threads/modules.
    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish);
}

HttpMgr::~HttpMgr() {}

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
    const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    qDebug() << "[HttpMgr] POST" << url.toString()
             << "req_id=" << static_cast<int>(req_id)
             << "mod=" << static_cast<int>(mod)
             << "bytes=" << data.size();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.size()));

    auto self = shared_from_this();
    QNetworkReply *reply = _manager.post(request, data);

    // Make network problems visible fast (otherwise Qt may wait a long time).
    QTimer::singleShot(5000, reply, [reply, url]() {
        if (reply->isFinished()) {
            return;
        }
        qDebug() << "[HttpMgr] timeout, aborting:" << url.toString();
        reply->abort();
    });

    QObject::connect(reply, &QNetworkReply::finished, [reply, self, req_id, mod]() {
        const int http_status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[HttpMgr] HTTP error, status=" << http_status
                     << "qt_err=" << reply->error()
                     << reply->errorString();
            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater();
            return;
        }

        const QString res = reply->readAll();
        qDebug() << "[HttpMgr] HTTP OK, status=" << http_status << "len=" << res.size();

        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
        reply->deleteLater();
    });
}

void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    if (mod == Modules::REGISTERMOD) {
        emit sig_reg_mod_finish(id, res, err);
    }

    if (mod == Modules::RESETMOD) {
        emit sig_reset_mod_finish(id, res, err);
    }

    if (mod == Modules::LOGINMOD) {
        emit sig_login_mod_finish(id, res, err);
    }
}
