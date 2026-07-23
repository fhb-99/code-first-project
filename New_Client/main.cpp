#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QDebug>

#include "global.h"
#include "mainwindow.h"

// ---------------------------------------------------------------------------
// 开发开关：true = 启动后直接进入聊天界面（不经过登录与 ChatServer 鉴权）。
// 正式发布或需要完整登录流程时改为 false。
// ---------------------------------------------------------------------------
static constexpr bool kDevSkipLoginEnterChat = false;

static QString findConfigIni()
{
    const QString exeDir = QCoreApplication::applicationDirPath();

    const QStringList candidates = {
        QDir(exeDir).filePath("config.ini"),
        QDir(exeDir).filePath("../config.ini"),
        QDir(exeDir).filePath("../../config.ini"),
        QDir(exeDir).filePath("config/config.ini"),
        QDir(exeDir).filePath("../config/config.ini"),
        QDir(exeDir).filePath("../../config/config.ini"),
    };

    for (const auto &p : candidates) {
        const QString cleaned = QDir::cleanPath(p);
        if (QFileInfo::exists(cleaned)) {
            return cleaned;
        }
    }

    QDir d(exeDir);
    for (int i = 0; i < 6; ++i) {
        const QString p1 = QDir::cleanPath(d.filePath("config.ini"));
        if (QFileInfo::exists(p1)) {
            return p1;
        }
        const QString p2 = QDir::cleanPath(d.filePath("config/config.ini"));
        if (QFileInfo::exists(p2)) {
            return p2;
        }
        const QString p3 = QDir::cleanPath(d.filePath("New_Client/config/config.ini"));
        if (QFileInfo::exists(p3)) {
            return p3;
        }
        if (!d.cdUp()) {
            break;
        }
    }

    return QString();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qss(":/style/stylesheet.qss");
    if (qss.open(QFile::ReadOnly)) {
        qDebug("open success");
        const QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    } else {
        qDebug("Open failed");
    }

    const QString configPath = findConfigIni();
    qDebug() << "Config path:" << (configPath.isEmpty() ? "<not found>" : configPath);

    QString gateHost;
    QString gatePort;

    if (!configPath.isEmpty()) {
        QSettings settings(configPath, QSettings::IniFormat);
        gateHost = settings.value("GateServer/host", settings.value("GateServer/Host")).toString().trimmed();
        gatePort = settings.value("GateServer/port", settings.value("GateServer/Port")).toString().trimmed();
        if (gateHost.isEmpty() || gatePort.isEmpty()) {
            qDebug() << "GateServer config missing. Keys:" << settings.allKeys();
        }
    }

    gate_url_prefix = "http://" + gateHost + ":" + gatePort;
    qDebug() << "Gate url prefix:" << gate_url_prefix;

    MainWindow_SetDevSkipLogin(kDevSkipLoginEnterChat);
    if (kDevSkipLoginEnterChat) {
        qDebug() << "Dev mode: skip login, open chat UI directly.";
    }

    MainWindow w;
    w.show();
    return a.exec();
}
