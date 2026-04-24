#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include "global.h"

static QString findConfigIni()
{
    const QString exeDir = QCoreApplication::applicationDirPath();

    // Common layouts:
    // 1) Qt Creator shadow-build:
    //      <build>/debug/New_Client.exe
    //      <build>/config.ini
    // 2) Source tree:
    //      <repo>/New-Client/New_Client/config/config.ini
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

    // Last resort: walk up a few levels and probe common names.
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
        // Be tolerant to different key casing after migrations.
        gateHost = settings.value("GateServer/host", settings.value("GateServer/Host")).toString().trimmed();
        gatePort = settings.value("GateServer/port", settings.value("GateServer/Port")).toString().trimmed();
        if (gateHost.isEmpty() || gatePort.isEmpty()) {
            qDebug() << "GateServer config missing. Keys:" << settings.allKeys();
        }
    }

    gate_url_prefix = "http://" + gateHost + ":" + gatePort;
    qDebug() << "Gate url prefix:" << gate_url_prefix;

    MainWindow w;
    w.show();
    return a.exec();
}

