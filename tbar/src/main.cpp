#include "backend.h"
#include "desktop.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

static int cliError(const QString &message, bool json) {
    if (json)
        QTextStream(stderr) << QJsonDocument(QJsonObject{{"error", message}}).toJson(QJsonDocument::Compact) << '\n';
    else
        QTextStream(stderr) << message << '\n';
    return 1;
}

static void printJson(const QJsonObject &object) {
    QTextStream(stdout) << QJsonDocument(object).toJson(QJsonDocument::Compact) << '\n';
}

static QString cliHelp() {
    return "Usage: tbar [open|help]\n"
           "       tbar list [--json]\n"
           "       tbar show NAME [--json]\n"
           "       tbar schema [--json]\n"
           "       tbar validate FILE [--json]\n"
           "       tbar import FILE [--replace] [--json]\n"
           "       tbar select NAME [--json]\n"
           "       tbar export [NAME] [--json]\n"
           "       tbar apply NAME [--json]\n"
           "       tbar restore [--json]\n"
           "       tbar check [--json]\n"
           "       tbar skill install [--json]\n"
           "FILE is a single profile JSON object; use - for standard input.\n";
}

int main(int argc, char **argv) {
    if (argc > 1 && QByteArray(argv[1]) != "open") {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        qunsetenv("QT_QPA_PLATFORMTHEME");
        qunsetenv("QT_IM_MODULE");
    }
    QGuiApplication app(argc, argv);
    app.setApplicationName("tbar");
    app.setDesktopFileName("tbar");
    QQuickStyle::setStyle("Material");
    Desktop desktop;
    app.installTranslator(&desktop);
    Backend backend;
    const QStringList arguments = app.arguments();
    if (arguments.size() == 2 && arguments[1] == "sync-theme") {
        if (backend.syncTheme()) return 0;
        QTextStream(stderr) << backend.message() << '\n';
        return 1;
    }
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
    if (arguments.size() > 1 && arguments[1] != "open" && arguments[1] != "--preview") {
        QStringList args = arguments.mid(1);
        const bool json = args.removeAll("--json") > 0;
        const QString command = args.takeFirst();
        if ((command == "help" || command == "--help" || command == "-h") && args.isEmpty()) {
            QTextStream(stdout) << cliHelp();
            return 0;
        }
        if (command == "schema" && args.isEmpty()) {
            QFile file(":/profile-schema.json");
            if (!file.open(QIODevice::ReadOnly)) return cliError("Could not read the bundled schema.", json);
            QTextStream(stdout) << file.readAll();
            return 0;
        }
        if (command == "skill" && args == QStringList{"install"}) {
            const QString dir = QDir::homePath() + "/.agents/skills/tbar";
            if (QFileInfo(dir).isSymLink()) return cliError("Skill directory is a symbolic link.", json);
            if (!QDir().mkpath(dir)) return cliError("Could not create the skill directory.", json);
            QFile source(":/agent-skill.md"), target(dir + "/SKILL.md");
            if (!source.open(QIODevice::ReadOnly)) return cliError("Could not read the bundled skill.", json);
            if (!target.open(QIODevice::WriteOnly | QIODevice::NewOnly))
                return cliError("Could not install skill (it may already exist): " + target.errorString(), json);
            const QByteArray data = source.readAll();
            if (target.write(data) != data.size() || !target.flush()) {
                target.remove();
                return cliError("Could not write the skill.", json);
            }
            printJson({{"ok", true}, {"path", target.fileName()}});
            return 0;
        }
        if (command == "validate" || command == "import") {
            const bool replace = command == "import" && args.removeAll("--replace") > 0;
            if (args.size() != 1) return cliError(cliHelp(), json);
            QFile file;
            if (args[0] == "-") {
                if (!file.open(stdin, QIODevice::ReadOnly)) return cliError("Could not read standard input.", json);
            } else {
                file.setFileName(args[0]);
                if (!file.open(QIODevice::ReadOnly)) return cliError("Could not read file: " + file.errorString(), json);
            }
            const QByteArray data = file.read(1024 * 1024 + 1);
            if (data.size() > 1024 * 1024) return cliError("Profile exceeds 1 MiB.", json);
            if (file.error() != QFileDevice::NoError) return cliError("Could not read profile.", json);
            QJsonParseError parse;
            const auto doc = QJsonDocument::fromJson(data, &parse);
            if (parse.error != QJsonParseError::NoError || !doc.isObject())
                return cliError("Expected a single JSON profile object.", json);
            const QJsonObject value = doc.object();
            const QString error = Backend::validateProfile(value);
            if (!error.isEmpty()) return cliError(error, json);
            if (command == "import" && !backend.importProfile(value, replace))
                return cliError(backend.message(), json);
            printJson({{"ok", true}, {"name", value.value("name")}});
            return 0;
        }
        if (command == "check" && args.isEmpty()) {
            printJson({{"profiles", QJsonArray::fromStringList(backend.profiles())},
                       {"rendererAvailable", backend.rendererAvailable()},
                       {"error", backend.profiles().isEmpty() ? backend.message() : QString()}});
            return backend.profiles().isEmpty() ? 1 : 0;
        }
        if (backend.profiles().isEmpty()) return cliError(backend.message(), json);
        if (command == "list" && args.isEmpty()) {
            printJson({{"profiles", QJsonArray::fromStringList(backend.profiles())},
                       {"selected", backend.profiles().value(backend.selected())}});
            return 0;
        }
        if (command == "show" && args.size() == 1) {
            const auto value = backend.profile(args[0]);
            if (value.isEmpty()) return cliError("Layout not found.", json);
            printJson(value);
            return 0;
        }
        if (command == "export" && args.size() <= 1) {
            const auto value = args.isEmpty() ? backend.rendererConfig() : backend.exportProfile(args[0]);
            if (value.isEmpty()) return cliError(backend.message(), json);
            printJson(value);
            return 0;
        }
        if ((command == "select" || command == "apply") && args.size() == 1) {
            const bool ok = command == "select" ? backend.selectProfile(args[0]) : backend.applyProfile(args[0]);
            if (!ok) return cliError(backend.message(), json);
            printJson({{"ok", true}, {"name", args[0]}});
            return 0;
        }
        if (command == "restore" && args.isEmpty()) {
            if (!backend.restoreOriginal()) return cliError(backend.message(), json);
            printJson({{"ok", true}});
            return 0;
        }
        return cliError(cliHelp(), json);
    }
    if (arguments.size() > 1 && !((arguments.size() == 2 && arguments[1] == "open")
                                    || (arguments.size() == 3 && arguments[1] == "--preview")))
        return cliError(cliHelp(), false);
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
        QTimer::singleShot(2500, &app, [window, target] {
            QCoreApplication::exit(window->grabWindow().save(target) ? 0 : 1);
        });
    }
    return app.exec();
}
