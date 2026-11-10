#include "backend.h"

#include <QDateTime>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QUrlQuery>
#include <QUuid>

namespace {
const QStringList widgets{"cpu", "memory", "network", "disk", "battery"};

QJsonObject widget(const QString &kind, const QString &label) {
    return {{"kind", "widget"}, {"label", label}, {"command", kind}};
}

QJsonObject button(const QString &label, const QString &command) {
    return {{"label", label}, {"command", command}};
}

bool validSnapshot(const QJsonObject &snapshot) {
    if (snapshot.value("version").toInt() != 1 || !snapshot.value("existed").isBool()
        || !snapshot.value("data").isString())
        return false;
    const QByteArray encoded = snapshot.value("data").toString().toLatin1();
    const auto decoded = QByteArray::fromBase64Encoding(encoded, QByteArray::AbortOnBase64DecodingErrors);
    return decoded && decoded.decoded.toBase64() == encoded
        && (snapshot.value("existed").toBool() || decoded.decoded.isEmpty());
}

bool writeJson(const QString &path, const QJsonObject &object, QString *error) {
    const QFileInfo info(path);
    if (info.isSymLink()) {
        *error = QObject::tr("The target is a symbolic link; refusing to overwrite it.");
        return false;
    }
    if (!QDir().mkpath(info.absolutePath())) {
        *error = QObject::tr("Could not create the configuration directory.");
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        *error = file.errorString();
        return false;
    }
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    const auto data = QJsonDocument(object).toJson();
    if (file.write(data) != data.size() || !file.commit()) {
        *error = file.errorString();
        return false;
    }
    return true;
}
}

Backend::Backend(const QString &configHome, QObject *parent)
    : QObject(parent), m_home(configHome.isEmpty()
          ? QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) : configHome) {
    m_previewTimer.setInterval(5000);
    m_previewTimeout.setSingleShot(true);
    m_previewTimeout.setInterval(15000);
    connect(&m_previewTimer, &QTimer::timeout, this, &Backend::updatePreview);
    connect(&m_previewTimeout, &QTimer::timeout, &m_previewProcess, &QProcess::kill);
    connect(this, &Backend::changed, this, [this] {
        ++m_previewRevision;
        if (m_previewStarted) updatePreview();
    });
    connect(&m_previewProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_previewTimeout.stop();
            m_previewError = tr("Could not start the renderer preview.");
            emit previewChanged();
        }
    });
    connect(&m_previewProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        m_previewTimeout.stop();
        if (m_previewRequested != m_previewRevision) {
            updatePreview();
            return;
        }
        if (status != QProcess::NormalExit || exitCode != 0) {
            m_previewError = tr("Could not render the preview.");
        } else {
            m_preview = QUrl::fromLocalFile(m_previewDir.path() + "/preview.png");
            QUrlQuery query;
            query.addQueryItem("frame", QString::number(QDateTime::currentMSecsSinceEpoch()));
            m_preview.setQuery(query);
            m_previewError.clear();
        }
        emit previewChanged();
    });
    QFile file(m_home + "/tbar/profiles.json");
    if (!file.exists()) {
        m_profiles = QJsonArray{QJsonObject{{"name", "Omarchy"}, {"buttons", QJsonArray{
            QJsonObject{{"label", "Keys"}, {"command", "omarchy menu keybindings"}},
            QJsonObject{{"label", "Menu"}, {"command", "omarchy menu toggle"}},
            QJsonObject{{"label", "Theme"}, {"command", "omarchy menu toggle appearance.theme"}},
            QJsonObject{{"label", "Agent"}, {"command", "omarchy agent"}},
            QJsonObject{{"label", "Lock"}, {"command", "omarchy system lock"}}
        }}}};
        return;
    }
    QJsonParseError parse;
    if (!file.open(QIODevice::ReadOnly)) {
        m_message = tr("Could not read profiles: %1").arg(file.errorString());
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse);
    const QJsonObject root = doc.object();
    if (parse.error != QJsonParseError::NoError || root.value("version").toInt() != 1
        || !root.value("profiles").isArray() || root.value("profiles").toArray().isEmpty()) {
        m_message = tr("Profiles are invalid. The file was not changed.");
        return;
    }
    const QJsonArray profiles = root.value("profiles").toArray();
    for (const auto &item : profiles) {
        const auto obj = item.toObject();
        if (!item.isObject() || obj.value("name").toString().trimmed().isEmpty()
            || !obj.value("buttons").isArray() || obj.value("buttons").toArray().size() > 12) {
            m_message = tr("Profiles are invalid. The file was not changed.");
            return;
        }
        for (const auto &button : obj.value("buttons").toArray()) {
            const auto obj = button.toObject();
            const QString kind = obj.value("kind").toString("button");
            const QString argument = obj.value("command").toString();
            if (!button.isObject() || obj.value("label").toString().trimmed().isEmpty()
                || !QStringList{"button", "internet", "temperature", "process", "widget"}.contains(kind)
                || (kind == "button" && argument.trimmed().isEmpty())
                || (kind == "widget" && !widgets.contains(argument))
                || (kind == "process" && !QRegularExpression("^[A-Za-z0-9._+-]{1,15}$").match(argument).hasMatch())) {
                m_message = tr("Profiles are invalid. The file was not changed.");
                return;
            }
        }
    }
    m_profiles = profiles;
    m_selected = qBound(0, root.value("selected").toInt(), int(m_profiles.size()) - 1);
}

QStringList Backend::profiles() const {
    QStringList result;
    for (const auto &profile : m_profiles)
        result.append(profile.toObject().value("name").toString());
    return result;
}

QVariantList Backend::buttons() const {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return {};
    return m_profiles[m_selected].toObject().value("buttons").toArray().toVariantList();
}

bool Backend::rendererAvailable() const {
    return !QStandardPaths::findExecutable("omarchy-glance").isEmpty();
}

void Backend::setPreviewFn(bool enabled) {
    if (m_previewFn == enabled) return;
    m_previewFn = enabled;
    ++m_previewRevision;
    emit previewChanged();
    if (m_previewStarted) updatePreview();
}

void Backend::startPreview() {
    if (m_previewStarted) return;
    m_previewStarted = true;
    m_previewTimer.start();
    updatePreview();
}

void Backend::updatePreview() {
    if (m_previewProcess.state() != QProcess::NotRunning || m_profiles.isEmpty()) return;
    const QString renderer = QStandardPaths::findExecutable("omarchy-glance");
    if (renderer.isEmpty() || !m_previewDir.isValid()) {
        m_previewError = tr("Renderer preview is unavailable.");
        emit previewChanged();
        return;
    }
    QString error;
    if (!writeJson(m_previewDir.path() + "/omarchy-glance/touchbar.json", rendererConfig(), &error)) {
        m_previewError = tr("Could not save preview configuration: %1").arg(error);
        emit previewChanged();
        return;
    }
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("XDG_CONFIG_HOME", m_previewDir.path());
    m_previewProcess.setProcessEnvironment(env);
    m_previewRequested = m_previewRevision;
    QStringList args{"preview", m_previewDir.path() + "/preview.png"};
    if (m_previewFn) args.append("fn");
    m_previewProcess.start(renderer, args);
    m_previewTimeout.start();
}

void Backend::setMessage(const QString &message) {
    m_message = message;
    emit messageChanged();
}

bool Backend::fail(const QString &message) {
    setMessage(message);
    return false;
}

bool Backend::save(const QJsonArray &profiles, int selected) {
    if (m_profiles.isEmpty())
        return fail(tr("Profiles are invalid. The file was not changed."));
    QString error;
    if (!writeJson(m_home + "/tbar/profiles.json",
                   {{"version", 1}, {"selected", selected}, {"profiles", profiles}}, &error))
        return fail(tr("Could not save profiles: %1").arg(error));
    m_profiles = profiles;
    m_selected = selected;
    setMessage(tr("Profiles saved."));
    emit changed();
    return true;
}

bool Backend::select(int index) {
    if (index < 0 || index >= m_profiles.size()) return false;
    return save(m_profiles, index);
}

bool Backend::addProfile(const QString &name) {
    const QString value = name.trimmed();
    if (value.isEmpty() || value.size() > 40 || profiles().contains(value))
        return fail(tr("Choose a unique profile name (up to 40 characters)."));
    QJsonArray updated = m_profiles;
    updated.append(QJsonObject{{"name", value}, {"buttons", QJsonArray{}}});
    return save(updated, int(updated.size()) - 1);
}

bool Backend::deleteProfile() {
    if (m_profiles.size() <= 1) return fail(tr("At least one profile is required."));
    QJsonArray updated = m_profiles;
    updated.removeAt(m_selected);
    return save(updated, qMin(m_selected, int(updated.size()) - 1));
}

bool Backend::addButton(const QString &label, const QString &command) {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return false;
    const QString title = label.trimmed(), action = command.trimmed();
    if (title.isEmpty() || title.size() > 16 || action.isEmpty() || action.size() > 1024)
        return fail(tr("Enter a label (up to 16 characters) and a command."));
    QJsonArray updated = m_profiles;
    QJsonObject profile = updated[m_selected].toObject();
    QJsonArray items = profile.value("buttons").toArray();
    if (items.size() >= 12) return fail(tr("A profile can have at most 12 items."));
    items.append(QJsonObject{{"label", title}, {"command", action}});
    profile.insert("buttons", items);
    updated[m_selected] = profile;
    return save(updated, m_selected);
}

bool Backend::addStatus(const QString &kind, const QString &label, const QString &argument) {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return false;
    if (!QStringList{"internet", "temperature", "process"}.contains(kind)
        || label.trimmed().isEmpty() || label.trimmed().size() > 16
        || (kind == "process" && !QRegularExpression("^[A-Za-z0-9._+-]{1,15}$").match(argument).hasMatch()))
        return fail(tr("Enter a label and, for an application, a process name."));
    QJsonArray updated = m_profiles;
    QJsonObject profile = updated[m_selected].toObject();
    QJsonArray items = profile.value("buttons").toArray();
    if (items.size() >= 12) return fail(tr("A profile can have at most 12 items."));
    items.append(QJsonObject{{"kind", kind}, {"label", label.trimmed()}, {"command", argument}});
    profile.insert("buttons", items);
    updated[m_selected] = profile;
    return save(updated, m_selected);
}

bool Backend::removeButton(int index) {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return false;
    QJsonArray updated = m_profiles;
    QJsonObject profile = updated[m_selected].toObject();
    QJsonArray items = profile.value("buttons").toArray();
    if (index < 0 || index >= items.size()) return false;
    items.removeAt(index);
    profile.insert("buttons", items);
    updated[m_selected] = profile;
    return save(updated, m_selected);
}

bool Backend::moveButton(int from, int to) {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return false;
    QJsonArray updated = m_profiles;
    QJsonObject profile = updated[m_selected].toObject();
    QJsonArray items = profile.value("buttons").toArray();
    if (from < 0 || to < 0 || from >= items.size() || to >= items.size()) return false;
    const auto value = items.takeAt(from);
    items.insert(to, value);
    profile.insert("buttons", items);
    updated[m_selected] = profile;
    return save(updated, m_selected);
}

bool Backend::addWidget(const QString &kind) {
    if (m_selected < 0 || m_selected >= m_profiles.size() || !widgets.contains(kind))
        return false;
    QJsonArray updated = m_profiles;
    QJsonObject profile = updated[m_selected].toObject();
    QJsonArray items = profile.value("buttons").toArray();
    if (items.size() >= 12) return fail(tr("A profile can have at most 12 items."));
    items.append(widget(kind, kind == "memory" ? "RAM" : kind.toUpper()));
    profile.insert("buttons", items);
    updated[m_selected] = profile;
    return save(updated, m_selected);
}

bool Backend::addExamples() {
    if (m_profiles.isEmpty()) return fail(tr("No valid profile is available."));
    const QJsonArray examples{
        QJsonObject{{"name", "Work"}, {"buttons", QJsonArray{
            button("Keys", "omarchy menu keybindings"), button("Menu", "omarchy menu toggle"),
            button("Theme", "omarchy menu toggle appearance.theme"), button("Agent", "omarchy agent"),
            widget("battery", "BAT")}}},
        QJsonObject{{"name", "Monitor"}, {"buttons", QJsonArray{
            widget("cpu", "CPU"), widget("memory", "RAM"), widget("network", "NET"),
            widget("disk", "DISK"), widget("battery", "BAT")}}},
        QJsonObject{{"name", "IT Demo"}, {"buttons", QJsonArray{
            button("Keys", "omarchy menu keybindings"), button("Agent", "omarchy agent"),
            button("Theme", "omarchy menu toggle appearance.theme"), widget("cpu", "CPU"),
            widget("memory", "RAM"), widget("network", "NET"), widget("battery", "BAT")}}}
    };
    QJsonArray updated = m_profiles;
    QStringList names = profiles();
    for (const auto &example : examples) {
        const QString name = example.toObject().value("name").toString();
        if (!names.contains(name)) {
            updated.append(example);
            names.append(name);
        }
    }
    if (updated.size() == m_profiles.size())
        return fail(tr("Example layouts already exist."));
    return save(updated, m_selected);
}

QJsonObject Backend::rendererConfig() const {
    QJsonArray left{QJsonObject{{"id", "glance.esc"}}};
    int index = 0;
    for (const auto &item : buttons()) {
        const auto button = item.toMap();
        QString binary = QCoreApplication::applicationFilePath();
        binary.replace("'", "'\\''");
        const QString kind = button.value("kind", "button").toString();
        const QString id = QString("tbar.%1").arg(index++);
        const QString encoded = QString::fromLatin1(button.value("command").toString().toUtf8().toBase64());
        if (kind == "widget") {
            left.append(QJsonObject{{"id", "glance." + button.value("command").toString()}});
        } else if (kind == "button") {
            const QString command = QString("'%1' action %2").arg(binary, encoded);
            left.append(QJsonObject{{"id", id}, {"type", "button"},
                                    {"label", button.value("label").toString()},
                                    {"exec", command}, {"width", 150}});
        } else {
            const QString label = QString::fromLatin1(button.value("label").toString().toUtf8().toBase64());
            const QString arg = encoded.isEmpty() ? "-" : encoded;
            left.append(QJsonObject{{"id", id}, {"type", "command"}, {"width", 150},
                                    {"interval", 5}, {"exec", QString("'%1' status %2 %3 %4")
                                         .arg(binary, kind, label, arg)}});
        }
    }
    QJsonArray fn{QJsonObject{{"id", "glance.esc"}}};
    for (int i = 1; i <= 12; ++i)
        fn.append(QJsonObject{{"id", QString("tbar.f%1").arg(i)}, {"type", "button"},
                              {"label", QString("F%1").arg(i)}, {"key", QString("f%1").arg(i)},
                              {"width", 140}});
    return {{"version", 1}, {"layers", QJsonObject{
        {"default", QJsonObject{{"left", left}}},
        {"fn", QJsonObject{{"left", fn}}}
    }}};
}

bool Backend::apply() {
    if (m_profiles.isEmpty()) return fail(tr("No valid profile is available."));
    const QString path = m_home + "/omarchy-glance/touchbar.json";
    const QString snapshotPath = m_home + "/tbar/original-renderer.json";
    if (QFileInfo(path).isSymLink())
        return fail(tr("The target is a symbolic link; refusing to overwrite it."));
    QJsonObject snapshot;
    QFile snapshotFile(snapshotPath);
    if (QFileInfo(snapshotPath).isSymLink())
        return fail(tr("The target is a symbolic link; refusing to overwrite it."));
    if (snapshotFile.exists()) {
        if (!snapshotFile.open(QIODevice::ReadOnly))
            return fail(tr("Could not read the original layout backup."));
        snapshot = QJsonDocument::fromJson(snapshotFile.readAll()).object();
        if (!validSnapshot(snapshot))
            return fail(tr("The original layout backup is invalid."));
    } else {
        const bool existed = QFile::exists(path);
        QByteArray original;
        if (existed) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                return fail(tr("Could not back up the existing renderer configuration."));
            original = file.readAll();
        }
        snapshot = {{"version", 1}, {"existed", existed},
                    {"data", QString::fromLatin1(original.toBase64())}};
        QString error;
        if (!writeJson(snapshotPath, snapshot, &error))
            return fail(tr("Could not save the original layout backup: %1").arg(error));
    }
    if (QFile::exists(path)) {
        const QString backup = path + ".backup-" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz")
                               + "-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!QFile::copy(path, backup))
            return fail(tr("Could not back up the existing renderer configuration."));
    }
    QString error;
    const auto config = rendererConfig();
    if (!writeJson(path, config, &error))
        return fail(tr("Could not apply the layout: %1").arg(error));
    snapshot.insert("appliedSha256", QString::fromLatin1(
        QCryptographicHash::hash(QJsonDocument(config).toJson(), QCryptographicHash::Sha256).toHex()));
    if (!writeJson(snapshotPath, snapshot, &error))
        return fail(tr("Layout applied, but the original backup could not be updated: %1").arg(error));
    setMessage(tr("Layout saved for omarchy-glance. Hardware support is required."));
    return true;
}

bool Backend::restoreOriginal() {
    const QString path = m_home + "/omarchy-glance/touchbar.json";
    const QString snapshotPath = m_home + "/tbar/original-renderer.json";
    if (QFileInfo(path).isSymLink() || QFileInfo(snapshotPath).isSymLink())
        return fail(tr("The target is a symbolic link; refusing to overwrite it."));
    QFile snapshotFile(snapshotPath), current(path);
    if (!snapshotFile.open(QIODevice::ReadOnly) || !current.open(QIODevice::ReadOnly))
        return fail(tr("No restorable original layout was found."));
    const auto snapshot = QJsonDocument::fromJson(snapshotFile.readAll()).object();
    const auto hash = QCryptographicHash::hash(current.readAll(), QCryptographicHash::Sha256).toHex();
    current.close();
    if (!validSnapshot(snapshot))
        return fail(tr("The original layout backup is invalid."));
    if (snapshot.value("appliedSha256").toString().toLatin1() != hash)
        return fail(tr("The current layout changed outside this app; it was not overwritten."));
    if (snapshot.value("existed").toBool()) {
        const auto data = QByteArray::fromBase64Encoding(snapshot.value("data").toString().toLatin1(),
                                                         QByteArray::AbortOnBase64DecodingErrors);
        if (!data) return fail(tr("The original layout backup is invalid."));
        QSaveFile output(path);
        if (!output.open(QIODevice::WriteOnly) || output.write(data.decoded) != data.decoded.size() || !output.commit())
            return fail(tr("Could not restore the original layout."));
    } else if (!QFile::remove(path)) {
        return fail(tr("Could not restore the original layout."));
    }
    setMessage(tr("Original renderer configuration restored."));
    return true;
}
