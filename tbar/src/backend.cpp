#include "backend.h"

#include <QDateTime>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QUrlQuery>
#include <QUuid>

namespace {
const QStringList widgets{"cpu", "memory", "network", "disk", "battery"};
const QStringList sections{"left", "center", "right"};
const QStringList controls{"volume", "brightness", "keyboard", "media", "mic"};
const QStringList kinds{"button", "internet", "temperature", "process", "widget", "spacer",
                        "control", "scene", "scene-next"};
const QRegularExpression appClass("^[A-Za-z0-9._+-]{1,64}$");

// Theme styles: "auto" keeps the bar black with subtle keys, "tinted" also
// colours the bar, and the rest paint the keys in that colour of the theme.
const QStringList vivid{"accent", "red", "orange", "yellow", "green", "cyan", "blue", "magenta"};
const QStringList themeStyles = QStringList{"auto", "tinted"} + vivid;

const QRegularExpression hexColor("^#[0-9a-fA-F]{6}$");

bool validTheme(const QString &style) {
    return themeStyles.contains(style) || hexColor.match(style).hasMatch();
}

// A glyph (usually from the Nerd Font) or a short text without control characters.
bool validText(const QJsonValue &value, int max) {
    const QString text = value.toString();
    if (!value.isString() || text.trimmed().isEmpty() || text.size() > max) return false;
    for (const QChar c : text)
        if (c.category() == QChar::Other_Control) return false;
    return true;
}

QJsonObject themeColors(const QString &style) {
    if (hexColor.match(style).hasMatch()) {
        // A hand-picked key colour: black bar, and black or white text by brightness.
        const int r = style.mid(1, 2).toInt(nullptr, 16), g = style.mid(3, 2).toInt(nullptr, 16),
                  b = style.mid(5, 2).toInt(nullptr, 16);
        const bool light = 299 * r + 587 * g + 114 * b > 140000;
        return {{"background", "#000000"}, {"key", style.toLower()},
                {"keyPressed", light ? "#ffffff" : "#808080"}, {"text", light ? "#000000" : "#ffffff"}};
    }
    const QString state = qEnvironmentVariable("XDG_STATE_HOME", QDir::homePath() + "/.local/state");
    QFile file(state + "/omarchy/current/theme/colors.toml");
    QJsonObject colors;
    if (!file.open(QIODevice::ReadOnly)) return colors;
    const QString text = QString::fromUtf8(file.readAll());
    QList<QPair<QString, QString>> keys{{"lighter_background", "key"}, {"accent", "keyPressed"},
                                        {"foreground", "text"}, {"red", "urgent"}};
    if (style == "tinted")
        keys = {{"background", "background"}, {"selection", "key"}, {"accent", "keyPressed"},
                {"foreground", "text"}, {"red", "urgent"}};
    else if (vivid.contains(style))
        keys = {{"darker_background", "background"}, {style, "key"}, {"foreground", "keyPressed"},
                {"darker_background", "text"}, {"red", "urgent"}};
    for (const auto &key : keys) {
        const QRegularExpression re("^\\s*" + key.first + "\\s*=\\s*[\"'](#[0-9a-fA-F]{6})[\"']",
                                    QRegularExpression::MultilineOption);
        const auto match = re.match(text);
        if (match.hasMatch()) colors.insert(key.second, match.captured(1));
    }
    return colors;
}

QByteArray hash(const QByteArray &data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
}

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
    const QByteArray profileData = file.readAll();
    m_profilesOnDisk = true;
    m_profilesHash = hash(profileData);
    const QJsonDocument doc = QJsonDocument::fromJson(profileData, &parse);
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
            || !obj.value("buttons").isArray() || obj.value("buttons").toArray().size() > 12
            || (obj.contains("theme") && !validTheme(obj.value("theme").toString()))
            || (obj.contains("apps") && !obj.value("apps").isArray())) {
            m_message = tr("Profiles are invalid. The file was not changed.");
            return;
        }
        for (const auto &button : obj.value("buttons").toArray()) {
            const auto obj = button.toObject();
            const QString kind = obj.value("kind").toString("button");
            const QString argument = obj.value("command").toString();
            if (!button.isObject() || obj.value("label").toString().trimmed().isEmpty()
                || !kinds.contains(kind)
                || ((kind == "button" || kind == "scene") && argument.trimmed().isEmpty())
                || (kind == "control" && !controls.contains(argument))
                || (kind == "widget" && !widgets.contains(argument))
                || (obj.contains("section") && !sections.contains(obj.value("section").toString()))
                || (obj.contains("width") && (!obj.value("width").isDouble()
                    || obj.value("width").toInt() < 10 || obj.value("width").toInt() > 500))
                || (obj.contains("style") && !QStringList{"dots", "bars"}.contains(obj.value("style").toString()))
                || (obj.contains("interval") && (!obj.value("interval").isDouble()
                    || obj.value("interval").toInt() < 1 || obj.value("interval").toInt() > 60))
                || (obj.contains("temperature") && !obj.value("temperature").isBool())
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

QString Backend::themeStyle() const {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return {};
    return m_profiles[m_selected].toObject().value("theme").toString();
}

bool Backend::setThemeStyle(const QString &style, bool everywhere) {
    if (m_selected < 0 || m_selected >= m_profiles.size()
        || (!style.isEmpty() && !validTheme(style))) return false;
    QJsonArray updated = m_profiles;
    for (int index = 0; index < updated.size(); ++index) {
        if (!everywhere && index != m_selected) continue;
        QJsonObject profile = updated[index].toObject();
        if (style.isEmpty()) profile.remove("theme");
        else profile.insert("theme", style);
        updated[index] = profile;
    }
    return save(updated, m_selected, everywhere);
}

bool Backend::setThemeAuto(bool enabled) { return setThemeStyle(enabled ? "auto" : "", false); }

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

bool Backend::profilesUnchanged() {
    const QString path = m_home + "/tbar/profiles.json";
    if (QFileInfo(path).isSymLink())
        return fail(tr("The target is a symbolic link; refusing to overwrite it."));
    QFile current(path);
    if (current.exists() != m_profilesOnDisk)
        return fail(tr("Profiles changed outside this app; reload before saving."));
    if (m_profilesOnDisk) {
        if (!current.open(QIODevice::ReadOnly)) return fail(tr("Could not read profiles: %1").arg(current.errorString()));
        const auto currentHash = hash(current.readAll());
        current.close();
        if (currentHash != m_profilesHash)
            return fail(tr("Profiles changed outside this app; reload before saving."));
    }
    return true;
}

bool Backend::save(const QJsonArray &profiles, int selected, bool backup) {
    if (m_profiles.isEmpty())
        return fail(tr("Profiles are invalid. The file was not changed."));
    const QString path = m_home + "/tbar/profiles.json";
    if (!QDir().mkpath(m_home + "/tbar")) return fail(tr("Could not create the configuration directory."));
    QLockFile lock(m_home + "/tbar/profiles.lock");
    if (!lock.tryLock(1000)) return fail(tr("Profiles are busy."));
    if (!profilesUnchanged()) return false;
    if (backup && m_profilesOnDisk) {
        const QString copy = path + ".backup-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!QFile::copy(path, copy)) return fail(tr("Could not back up profiles."));
    }
    QString error;
    const QJsonObject data{{"version", 1}, {"selected", selected}, {"profiles", profiles}};
    if (!writeJson(path, data, &error))
        return fail(tr("Could not save profiles: %1").arg(error));
    m_profilesOnDisk = true;
    m_profilesHash = hash(QJsonDocument(data).toJson());
    m_profiles = profiles;
    m_selected = selected;
    setMessage(tr("Profiles saved."));
    emit changed();
    return true;
}

QJsonObject Backend::profile(const QString &name) const {
    for (const auto &value : m_profiles)
        if (value.toObject().value("name").toString() == name) return value.toObject();
    return {};
}

QString Backend::validateProfile(const QJsonObject &profile) {
    const QStringList profileKeys{"name", "theme", "apps", "buttons", "escLabel", "escIcon", "font", "idleDim"};
    for (auto it = profile.begin(); it != profile.end(); ++it)
        if (!profileKeys.contains(it.key())) return "Unknown profile field: " + it.key();
    if (!profile.value("name").isString() || profile.value("name").toString().trimmed().isEmpty()
        || profile.value("name").toString().size() > 40
        || profile.value("name").toString() != profile.value("name").toString().trimmed())
        return "name must be a non-empty trimmed string of at most 40 characters";
    if (profile.contains("theme") && (!profile.value("theme").isString()
        || !validTheme(profile.value("theme").toString())))
        return "theme must be #rrggbb or one of: " + themeStyles.join(", ");
    if (profile.contains("escLabel") && !validText(profile.value("escLabel"), 16))
        return "escLabel must be a single line of 1-16 characters";
    if (profile.contains("escIcon") && !validText(profile.value("escIcon"), 4))
        return "escIcon must be a glyph of 1-4 characters";
    if (profile.contains("font") && !validText(profile.value("font"), 64))
        return "font must be a font family name of 1-64 characters";
    if (profile.contains("idleDim") && (!profile.value("idleDim").isDouble()
        || profile.value("idleDim").toDouble() != profile.value("idleDim").toInt()
        || profile.value("idleDim").toInt() < 0 || profile.value("idleDim").toInt() > 3600))
        return "idleDim must be an integer from 0 to 3600 seconds";
    if (profile.contains("apps")) {
        if (!profile.value("apps").isArray() || profile.value("apps").toArray().size() > 8)
            return "apps must be an array of at most 8 window classes";
        for (const auto &app : profile.value("apps").toArray())
            if (!app.isString() || !appClass.match(app.toString()).hasMatch())
                return "apps items must be window classes of 1-64 characters from A-Za-z0-9._+-";
    }
    if (!profile.value("buttons").isArray() || profile.value("buttons").toArray().size() > 12)
        return "buttons must be an array of at most 12 items";
    const QStringList itemKeys{"label", "kind", "command", "section", "width", "style", "interval",
                               "temperature", "icon"};
    int index = 0;
    for (const auto &value : profile.value("buttons").toArray()) {
        const QString prefix = QString("buttons[%1]: ").arg(index++);
        if (!value.isObject()) return prefix + "must be an object";
        const QJsonObject item = value.toObject();
        for (auto it = item.begin(); it != item.end(); ++it)
            if (!itemKeys.contains(it.key())) return prefix + "unknown field " + it.key();
        if (!item.value("label").isString() || item.value("label").toString().trimmed().isEmpty()
            || item.value("label").toString().contains('\0') || item.value("label").toString().contains('\n')
            || item.value("label").toString().contains('\r')
            || item.value("label").toString().size() > 16)
            return prefix + "label must be a non-empty string of at most 16 characters";
        if (item.contains("kind") && !item.value("kind").isString()) return prefix + "kind must be a string";
        const QString kind = item.value("kind").toString("button");
        if (!kinds.contains(kind))
            return prefix + "unknown kind";
        if (item.contains("command") && !item.value("command").isString()) return prefix + "command must be a string";
        const QString command = item.value("command").toString();
        if (command.contains('\0') || command.size() > 1024 || (kind == "button" && command.trimmed().isEmpty()))
            return prefix + "button command must be non-empty and at most 1024 characters";
        if (kind == "widget" && !widgets.contains(command)) return prefix + "unknown widget";
        if (item.contains("icon") && (!QStringList{"button", "scene", "scene-next"}.contains(kind)
            || !validText(item.value("icon"), 4)))
            return prefix + "icon must be a glyph of 1-4 characters on a button or scene switch";
        if (kind == "control" && !controls.contains(command))
            return prefix + "control must be volume, brightness, keyboard, media or mic";
        if (kind == "scene" && (command.trimmed().isEmpty() || command != command.trimmed() || command.size() > 40))
            return prefix + "scene command must be the name of a scene";
        if (kind == "scene-next" && !command.isEmpty())
            return prefix + "scene-next takes no command";
        if (kind == "process" && !QRegularExpression("^[A-Za-z0-9._+-]{1,15}$").match(command).hasMatch())
            return prefix + "invalid process name";
        if (item.contains("section") && !sections.contains(item.value("section").toString()))
            return prefix + "section must be left, center or right";
        if (item.contains("width") && (!item.value("width").isDouble()
            || item.value("width").toInt() < 10 || item.value("width").toInt() > 500))
            return prefix + "width must be an integer from 10 to 500";
        if (item.contains("style") && (kind != "widget"
            || !QStringList{"dots", "bars"}.contains(item.value("style").toString())))
            return prefix + "style must be dots or bars on a widget";
        if (item.contains("interval") && (kind != "widget" || !item.value("interval").isDouble()
            || item.value("interval").toInt() < 1 || item.value("interval").toInt() > 60))
            return prefix + "interval must be an integer from 1 to 60 on a widget";
        if (item.contains("temperature") && (kind != "widget" || command != "cpu"
            || !item.value("temperature").isBool()))
            return prefix + "temperature must be boolean on a CPU widget";
    }
    return {};
}

bool Backend::importProfile(const QJsonObject &value, bool replace) {
    const QString error = validateProfile(value);
    if (!error.isEmpty()) return fail(error);
    const QString name = value.value("name").toString();
    const int index = profiles().indexOf(name);
    if (index >= 0 && !replace) return fail(tr("Layout already exists; use --replace to overwrite it."));
    QJsonArray updated = m_profiles;
    if (index >= 0) updated[index] = value;
    else updated.append(value);
    return save(updated, m_selected, index >= 0);
}

bool Backend::selectProfile(const QString &name) {
    const int index = profiles().indexOf(name);
    return index < 0 ? fail(tr("Layout not found.")) : select(index);
}

bool Backend::applyProfile(const QString &name) {
    const int index = profiles().indexOf(name);
    if (index < 0) return fail(tr("Layout not found."));
    const int previous = m_selected;
    m_selected = index;
    const auto restore = qScopeGuard([&] { m_selected = previous; });
    return apply();
}

QJsonObject Backend::exportProfile(const QString &name) {
    const int index = profiles().indexOf(name);
    if (index < 0) { fail(tr("Layout not found.")); return {}; }
    const int previous = m_selected;
    m_selected = index;
    const auto restore = qScopeGuard([&] { m_selected = previous; });
    return rendererConfig();
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

bool Backend::addSpacer() {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return false;
    QJsonArray updated = m_profiles;
    QJsonObject profile = updated[m_selected].toObject();
    QJsonArray items = profile.value("buttons").toArray();
    if (items.size() >= 12) return fail(tr("A profile can have at most 12 items."));
    items.append(QJsonObject{{"kind", "spacer"}, {"label", "Spacer"}, {"command", ""}, {"width", 40}});
    profile.insert("buttons", items);
    updated[m_selected] = profile;
    return save(updated, m_selected);
}

bool Backend::editItem(int index, const QString &section, int width,
                       const QString &style, int interval, bool temperature) {
    if (m_selected < 0 || m_selected >= m_profiles.size() || !sections.contains(section)
        || width < 10 || width > 500 || !QStringList{"dots", "bars"}.contains(style)
        || interval < 1 || interval > 60) return false;
    QJsonArray updated = m_profiles;
    QJsonObject profile = updated[m_selected].toObject();
    QJsonArray items = profile.value("buttons").toArray();
    if (index < 0 || index >= items.size()) return false;
    QJsonObject item = items[index].toObject();
    item.insert("section", section);
    item.insert("width", width);
    if (item.value("kind").toString() == "widget") {
        item.insert("style", style);
        item.insert("interval", interval);
        if (item.value("command").toString() == "cpu") item.insert("temperature", temperature);
    }
    items[index] = item;
    profile.insert("buttons", items);
    updated[m_selected] = profile;
    return save(updated, m_selected);
}

bool Backend::appendItem(const QJsonObject &item) {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return false;
    QJsonArray updated = m_profiles;
    QJsonObject profile = updated[m_selected].toObject();
    QJsonArray items = profile.value("buttons").toArray();
    if (items.size() >= 12) return fail(tr("A profile can have at most 12 items."));
    items.append(item);
    profile.insert("buttons", items);
    updated[m_selected] = profile;
    return save(updated, m_selected);
}

bool Backend::addControl(const QString &kind) {
    if (!controls.contains(kind)) return false;
    const QStringList labels{"Volume", "Brightness", "Keyboard", "Media", "Mic"};
    return appendItem({{"kind", "control"}, {"label", labels[controls.indexOf(kind)]}, {"command", kind}});
}

bool Backend::addSceneButton(const QString &label, const QString &target) {
    const QString title = label.trimmed();
    if (title.isEmpty() || title.size() > 16 || (!target.isEmpty() && !profiles().contains(target)))
        return fail(tr("Enter a label (up to 16 characters) and choose a layout."));
    if (target.isEmpty()) return appendItem({{"kind", "scene-next"}, {"label", title}});
    return appendItem({{"kind", "scene"}, {"label", title}, {"command", target}});
}

QString Backend::apps() const {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return {};
    QStringList names;
    for (const auto &app : m_profiles[m_selected].toObject().value("apps").toArray())
        names.append(app.toString());
    return names.join(", ");
}

QVariantMap Backend::look() const {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return {};
    const QJsonObject scene = m_profiles[m_selected].toObject();
    return {{"escLabel", scene.value("escLabel").toString()}, {"escIcon", scene.value("escIcon").toString()},
            {"font", scene.value("font").toString()}, {"idleDim", scene.value("idleDim").toInt()}};
}

bool Backend::setLook(const QString &escLabel, const QString &escIcon, const QString &font, int idleDim) {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return false;
    QJsonArray updated = m_profiles;
    QJsonObject scene = updated[m_selected].toObject();
    const QList<QPair<QString, QString>> texts{{"escLabel", escLabel.trimmed()}, {"escIcon", escIcon.trimmed()},
                                               {"font", font.trimmed()}};
    for (const auto &text : texts) {
        if (text.second.isEmpty()) scene.remove(text.first);
        else scene.insert(text.first, text.second);
    }
    if (idleDim > 0) scene.insert("idleDim", idleDim);
    else scene.remove("idleDim");
    const QString error = validateProfile(scene);
    if (!error.isEmpty()) return fail(tr("Check the Escape text or icon, the font and the dimming time."));
    updated[m_selected] = scene;
    return save(updated, m_selected);
}

bool Backend::setItemIcon(int index, const QString &icon) {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return false;
    QJsonArray updated = m_profiles;
    QJsonObject scene = updated[m_selected].toObject();
    QJsonArray items = scene.value("buttons").toArray();
    if (index < 0 || index >= items.size()) return false;
    QJsonObject item = items[index].toObject();
    if (item.value("icon").toString() == icon.trimmed()) return true;
    if (icon.trimmed().isEmpty()) item.remove("icon");
    else item.insert("icon", icon.trimmed());
    items[index] = item;
    scene.insert("buttons", items);
    if (!validateProfile(scene).isEmpty()) return fail(tr("An icon is one glyph on a button or layout switch."));
    updated[m_selected] = scene;
    return save(updated, m_selected);
}

bool Backend::setApps(const QString &list) {
    if (m_selected < 0 || m_selected >= m_profiles.size()) return false;
    QJsonArray names;
    for (const auto &part : list.split(',', Qt::SkipEmptyParts)) {
        const QString name = part.trimmed();
        if (name.isEmpty()) continue;
        if (!appClass.match(name).hasMatch() || names.size() >= 8)
            return fail(tr("Enter at most 8 window classes separated by commas, e.g. firefox."));
        if (!names.contains(name)) names.append(name);
    }
    QJsonArray updated = m_profiles;
    QJsonObject profile = updated[m_selected].toObject();
    if (names.isEmpty()) profile.remove("apps");
    else profile.insert("apps", names);
    updated[m_selected] = profile;
    return save(updated, m_selected);
}

QJsonObject Backend::state() const {
    QFile file(m_home + "/tbar/state.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

QString Backend::appliedProfile() const { return state().value("applied").toString(); }

QString Backend::manualProfile() const { return state().value("manual").toString(); }

QString Backend::nextProfile() const {
    const QStringList names = profiles();
    if (names.isEmpty()) return {};
    const int index = names.indexOf(appliedProfile());
    return index < 0 ? names.value(m_selected) : names[(index + 1) % names.size()];
}

QString Backend::targetForApp(const QString &windowClass) const {
    QString target;
    for (const auto &value : m_profiles) {
        for (const auto &app : value.toObject().value("apps").toArray())
            if (!windowClass.isEmpty() && app.toString().compare(windowClass, Qt::CaseInsensitive) == 0)
                target = value.toObject().value("name").toString();
        if (!target.isEmpty()) break;
    }
    if (target.isEmpty()) target = manualProfile();
    if (target.isEmpty()) target = profiles().value(m_selected);
    if (!profiles().contains(target) || target == appliedProfile()) return {};
    return target;
}

bool Backend::switchProfile(const QString &name, bool manual) {
    const int index = profiles().indexOf(name);
    if (index < 0) return fail(tr("Layout not found."));
    const int previous = m_selected;
    m_selected = index;
    const auto restore = qScopeGuard([&] { m_selected = previous; });
    return write(true, manual);
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
    const QJsonObject scene = m_selected >= 0 && m_selected < m_profiles.size()
        ? m_profiles[m_selected].toObject() : QJsonObject();
    QJsonObject esc{{"id", "glance.esc"}};
    if (scene.contains("escIcon"))
        esc = {{"id", "tbar.esc"}, {"type", "button"}, {"key", "esc"}, {"icon", scene.value("escIcon")}, {"width", 140}};
    else if (scene.contains("escLabel"))
        esc = {{"id", "tbar.esc"}, {"type", "button"}, {"key", "esc"}, {"label", scene.value("escLabel")}, {"width", 140}};
    // An icon replaces the text of a button.
    const auto face = [](const QVariantMap &item) {
        return item.contains("icon") ? QPair<QString, QJsonValue>{"icon", item.value("icon").toString()}
                                     : QPair<QString, QJsonValue>{"label", item.value("label").toString()};
    };
    QJsonArray left{esc}, center, right;
    int index = 0;
    for (const auto &item : buttons()) {
        const auto button = item.toMap();
        QString binary = QCoreApplication::applicationFilePath();
        binary.replace("'", "'\\''");
        const QString kind = button.value("kind", "button").toString();
        const QString id = QString("tbar.%1").arg(index++);
        const QString encoded = QString::fromLatin1(button.value("command").toString().toUtf8().toBase64());
        QJsonObject spec;
        const QString section = button.value("section", "left").toString();
        QJsonArray &target = section == "center" ? center : section == "right" ? right : left;
        if (kind == "control") {
            const QString what = button.value("command").toString();
            const auto sized = [&](QJsonObject item) {
                if (button.contains("width")) item.insert("width", button.value("width").toInt());
                return item;
            };
            const auto pair = [&](char32_t icon, const QString &down, const QString &up) {
                const QString glyph = QString::fromUcs4(&icon, 1);
                const int width = button.value("width", 100).toInt();
                target.append(QJsonObject{{"id", id + ".down"}, {"type", "button"}, {"icon", glyph},
                                          {"iconSize", 20}, {"exec", down}, {"repeat", true}, {"width", width}});
                target.append(QJsonObject{{"id", id + ".up"}, {"type", "button"}, {"icon", glyph},
                                          {"iconSize", 32}, {"exec", up}, {"repeat", true}, {"width", width}});
            };
            if (what == "volume") {
                for (const QString preset : {"glance.mute", "glance.volume-down", "glance.volume-up"})
                    target.append(sized(QJsonObject{{"id", preset}}));
            } else if (what == "brightness") {
                pair(0xF00DE, "omarchy-brightness-display 5%-", "omarchy-brightness-display +5%");
            } else if (what == "keyboard") {
                pair(0xF030C, "omarchy-brightness-keyboard down", "omarchy-brightness-keyboard up");
            } else {
                target.append(QJsonObject{{"id", "glance." + what}});
            }
            continue;
        }
        if (kind == "scene" || kind == "scene-next") {
            target.append(QJsonObject{{"id", id}, {"type", "button"}, {face(button).first, face(button).second},
                                      {"exec", kind == "scene" ? QString("'%1' scene %2").arg(binary, encoded)
                                                               : QString("'%1' scene-next").arg(binary)},
                                      {"width", button.value("width", 150).toInt()}});
            continue;
        }
        if (kind == "widget") {
            spec = {{"id", "glance." + button.value("command").toString()}};
            if (button.contains("width")) spec.insert("graphWidth", button.value("width").toInt());
            if (button.contains("style")) spec.insert("style", button.value("style").toString());
            if (button.contains("interval")) spec.insert("interval", button.value("interval").toInt());
            if (button.contains("temperature")) spec.insert("temperature", button.value("temperature").toBool());
        } else if (kind == "spacer") {
            spec = {{"id", "glance.spacer"}, {"size", button.value("width", 40).toInt()}};
        } else if (kind == "button") {
            const QString command = QString("'%1' action %2").arg(binary, encoded);
            spec = QJsonObject{{"id", id}, {"type", "button"},
                                    {face(button).first, face(button).second},
                                    {"exec", command}, {"width", button.value("width", 150).toInt()}};
        } else {
            const QString label = QString::fromLatin1(button.value("label").toString().toUtf8().toBase64());
            const QString arg = encoded.isEmpty() ? "-" : encoded;
            spec = QJsonObject{{"id", id}, {"type", "command"}, {"width", button.value("width", 150).toInt()},
                                    {"interval", 5}, {"exec", QString("'%1' status %2 %3 %4")
                                         .arg(binary, kind, label, arg)}};
        }
        target.append(spec);
    }
    QJsonArray fn{esc};
    for (int i = 1; i <= 12; ++i)
        fn.append(QJsonObject{{"id", QString("tbar.f%1").arg(i)}, {"type", "button"},
                              {"label", QString("F%1").arg(i)}, {"key", QString("f%1").arg(i)},
                              {"width", 140}});
    QJsonObject result{{"version", 1}, {"layers", QJsonObject{
        {"default", QJsonObject{{"left", left}, {"center", center}, {"right", right}}},
        {"fn", QJsonObject{{"left", fn}}}
    }}};
    if (scene.contains("font")) result.insert("font", scene.value("font"));
    if (scene.contains("idleDim")) result.insert("idleDimSeconds", scene.value("idleDim"));
    if (!themeStyle().isEmpty()) {
        result.insert("tbar", QJsonObject{{"theme", themeStyle()}});
        result.insert("colors", themeColors(themeStyle()));
    }
    return result;
}

bool Backend::apply() { return write(false, true); }

// quick: switch between tbar's own layouts without another timestamped backup;
// refuses unless the renderer file is exactly what tbar last wrote.
bool Backend::write(bool quick, bool manual) {
    if (m_profiles.isEmpty()) return fail(tr("No valid profile is available."));
    if (!QDir().mkpath(m_home + "/tbar")) return fail(tr("Could not create the configuration directory."));
    QLockFile profilesLock(m_home + "/tbar/profiles.lock");
    if (!profilesLock.tryLock(1000)) return fail(tr("Profiles are busy."));
    if (!profilesUnchanged()) return false;
    QLockFile lock(m_home + "/tbar/renderer.lock");
    if (!lock.tryLock(1000)) return fail(tr("The renderer configuration is busy."));
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
    } else if (quick) {
        return fail(tr("Apply a layout once before switching layouts."));
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
    if (quick) {
        QFile current(path);
        if (!current.open(QIODevice::ReadOnly)
            || snapshot.value("appliedSha256").toString().toLatin1() != hash(current.readAll()))
            return fail(tr("The current layout changed outside this app; it was not overwritten."));
    } else if (QFile::exists(path)) {
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
        hash(QJsonDocument(config).toJson())));
    if (!writeJson(snapshotPath, snapshot, &error))
        return fail(tr("Layout applied, but the original backup could not be updated: %1").arg(error));
    const QString name = profiles().value(m_selected);
    const QString previousManual = manualProfile();
    if (!writeJson(m_home + "/tbar/state.json", {{"version", 1}, {"applied", name},
                   {"manual", manual ? name : previousManual}}, &error))
        return fail(tr("Layout applied, but its state could not be saved: %1").arg(error));
    emit appliedChanged();
    setMessage(tr("Layout saved for omarchy-glance. Hardware support is required."));
    return true;
}

bool Backend::restoreOriginal() {
    if (!QDir().mkpath(m_home + "/tbar")) return fail(tr("Could not create the configuration directory."));
    QLockFile lock(m_home + "/tbar/renderer.lock");
    if (!lock.tryLock(1000)) return fail(tr("The renderer configuration is busy."));
    const QString path = m_home + "/omarchy-glance/touchbar.json";
    const QString snapshotPath = m_home + "/tbar/original-renderer.json";
    if (QFileInfo(path).isSymLink() || QFileInfo(snapshotPath).isSymLink())
        return fail(tr("The target is a symbolic link; refusing to overwrite it."));
    QFile snapshotFile(snapshotPath), current(path);
    if (!snapshotFile.open(QIODevice::ReadOnly) || !current.open(QIODevice::ReadOnly))
        return fail(tr("No restorable original layout was found."));
    const auto snapshot = QJsonDocument::fromJson(snapshotFile.readAll()).object();
    const auto currentHash = hash(current.readAll());
    current.close();
    if (!validSnapshot(snapshot))
        return fail(tr("The original layout backup is invalid."));
    if (snapshot.value("appliedSha256").toString().toLatin1() != currentHash)
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
    QFile::remove(m_home + "/tbar/state.json");
    emit appliedChanged();
    setMessage(tr("Original renderer configuration restored."));
    return true;
}

bool Backend::syncTheme() {
    const QString path = m_home + "/omarchy-glance/touchbar.json";
    const QString snapshotPath = m_home + "/tbar/original-renderer.json";
    if (!QFile::exists(path)) return true;
    if (QFileInfo(path).isSymLink() || QFileInfo(snapshotPath).isSymLink())
        return fail(tr("The target is a symbolic link; refusing to overwrite it."));
    if (!QDir().mkpath(m_home + "/tbar")) return fail(tr("Could not create the configuration directory."));
    QLockFile lock(m_home + "/tbar/renderer.lock");
    if (!lock.tryLock(1000)) return fail(tr("The renderer configuration is busy."));
    QFile current(path);
    if (!current.open(QIODevice::ReadOnly)) return fail(tr("Could not read the renderer configuration."));
    const QByteArray data = current.readAll();
    current.close();
    QJsonParseError error;
    QJsonObject config = QJsonDocument::fromJson(data, &error).object();
    if (error.error != QJsonParseError::NoError) return fail(tr("The renderer configuration is invalid."));
    const QString style = config.value("tbar").toObject().value("theme").toString();
    if (!themeStyles.contains(style)) return true;  // a fixed colour does not follow the theme
    QFile snapshotFile(snapshotPath);
    if (!snapshotFile.open(QIODevice::ReadOnly)) return fail(tr("Could not read the original layout backup."));
    QJsonObject snapshot = QJsonDocument::fromJson(snapshotFile.readAll()).object();
    snapshotFile.close();
    if (!validSnapshot(snapshot)) return fail(tr("The original layout backup is invalid."));
    if (snapshot.value("appliedSha256").toString().toLatin1() != hash(data))
        return fail(tr("The current layout changed outside this app; it was not overwritten."));
    const QJsonObject colors = themeColors(style);
    if (colors.isEmpty()) return fail(tr("Could not read the Omarchy theme colors."));
    if (config.value("colors").toObject() == colors) return true;
    config.insert("colors", colors);
    QString writeError;
    if (!writeJson(path, config, &writeError))
        return fail(tr("Could not apply the layout: %1").arg(writeError));
    snapshot.insert("appliedSha256", QString::fromLatin1(hash(QJsonDocument(config).toJson())));
    if (!writeJson(snapshotPath, snapshot, &writeError))
        return fail(tr("Layout applied, but the original backup could not be updated: %1").arg(writeError));
    return true;
}
