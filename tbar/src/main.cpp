#include "backend.h"
#include "desktop.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>

int main(int argc, char **argv) {
    if (argc > 1 && (QByteArray(argv[1]) == "check" || QByteArray(argv[1]) == "export"
                     || QByteArray(argv[1]) == "action"
                     || QByteArray(argv[1]) == "status"
                     || QByteArray(argv[1]) == "--preview"))
        qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    app.setApplicationName("tbar");
    app.setDesktopFileName("tbar");
    QQuickStyle::setStyle("Material");
    Desktop desktop;
    app.installTranslator(&desktop);
    Backend backend;
    const QStringList arguments = app.arguments();
    if (arguments.size() == 5 && arguments[1] == "status") {
        const auto label = QByteArray::fromBase64Encoding(arguments[3].toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
        const auto arg = QByteArray::fromBase64Encoding(arguments[4] == "-" ? QByteArray() : arguments[4].toLatin1(),
                                                         QByteArray::AbortOnBase64DecodingErrors);
        if (!label || !arg || label.decoded.contains('\n') || label.decoded.contains('\0')) return 1;
        const QString kind = arguments[2], value = QString::fromUtf8(arg.decoded);
        QString status = "?";
        if (kind == "internet") {
            QProcess query;
            query.start("nmcli", {"networking", "connectivity"});
            if (query.waitForFinished(1500) && query.exitStatus() == QProcess::NormalExit && query.exitCode() == 0) {
                const auto result = query.readAllStandardOutput().trimmed();
                if (result == "full") status = "online";
                else if (result == "none") status = "offline";
                else if (result == "limited" || result == "portal") status = "limited";
            }
        } else if (kind == "temperature") {
            const QDir root("/sys/class/hwmon");
            for (const auto &name : root.entryList({"hwmon*"}, QDir::Dirs)) {
                const QString dir = root.filePath(name);
                QFile sensor(dir + "/name");
                if (!sensor.open(QIODevice::ReadOnly) || sensor.readAll().trimmed() != "coretemp") continue;
                QFile temperature(dir + "/temp1_input");
                bool ok = false;
                if (temperature.open(QIODevice::ReadOnly)) {
                    const int milli = temperature.readAll().trimmed().toInt(&ok);
                    if (ok) status = QString::number(milli / 1000) + " C";
                }
                break;
            }
        } else if (kind == "process" && QRegularExpression("^[A-Za-z0-9._+-]{1,15}$").match(value).hasMatch()) {
            QProcess query;
            query.start("pgrep", {"-x", "--", value});
            if (query.waitForFinished(1500) && query.exitStatus() == QProcess::NormalExit) {
                if (query.exitCode() == 0) status = "running";
                else if (query.exitCode() == 1) status = "stopped";
            }
        } else {
            return 1;
        }
        QTextStream(stdout) << QString::fromUtf8(label.decoded) << ": " << status << '\n';
        return 0;
    }
    if (arguments.size() == 3 && arguments[1] == "action") {
        QProcess shellLock;
        shellLock.start("omarchy-shell", {"lock", "isLocked"});
        if (!shellLock.waitForFinished(1500) || shellLock.exitStatus() != QProcess::NormalExit
            || shellLock.exitCode() != 0 || shellLock.readAllStandardOutput().trimmed() != "false")
            return 1;
        QProcess compositorLock;
        compositorLock.start("omarchy-hyprland-session-locked");
        if (!compositorLock.waitForFinished(1500) || compositorLock.exitStatus() != QProcess::NormalExit
            || compositorLock.exitCode() != 1)
            return 1;
        const auto decoded = QByteArray::fromBase64Encoding(arguments[2].toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded || decoded.decoded.trimmed().isEmpty() || decoded.decoded.contains('\0'))
            return 1;
        return QProcess::startDetached("/bin/sh", {"-c", QString::fromUtf8(decoded.decoded)}) ? 0 : 1;
    }
    if (arguments.size() > 1 && (arguments[1] == "check" || arguments[1] == "export")) {
        QJsonObject result;
        if (arguments[1] == "export") {
            if (backend.profiles().isEmpty()) {
                QTextStream(stderr) << backend.message() << '\n';
                return 1;
            }
            result = backend.rendererConfig();
        } else {
            result = {{"profiles", QJsonArray::fromStringList(backend.profiles())},
                      {"rendererAvailable", backend.rendererAvailable()},
                      {"error", backend.profiles().isEmpty() ? backend.message() : QString()}};
        }
        QTextStream(stdout) << QJsonDocument(result).toJson(QJsonDocument::Compact) << '\n';
        return backend.profiles().isEmpty() ? 1 : 0;
    }
    QQmlApplicationEngine engine;
    QObject::connect(&desktop, &Desktop::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("desktop", &desktop);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    if (arguments.size() == 3 && arguments[1] == "--preview") {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        if (!window) return 1;
        const QString target = arguments[2];
        QTimer::singleShot(700, &app, [window, target] {
            QCoreApplication::exit(window->grabWindow().save(target) ? 0 : 1);
        });
    }
    return app.exec();
}
