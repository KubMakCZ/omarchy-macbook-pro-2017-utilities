#include "desktop.h"
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QLocale>
#include <QRegularExpression>
#include <QStandardPaths>

Desktop::Desktop() {
    connect(&m_timer, &QTimer::timeout, this, &Desktop::reload);
    m_timer.start(1000);
    reload();
}

QString Desktop::translate(const char *, const char *source, const char *, int) const {
    return m_strings.value(QString::fromUtf8(source)).toString();
}

void Desktop::reload() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QFile lang(base + "/omarchy/language");
    QString language = qEnvironmentVariable("TBAR_LANGUAGE");
    if (language.isEmpty() && lang.open(QIODevice::ReadOnly))
        language = QString::fromUtf8(lang.readLine()).trimmed();
    if (!QRegularExpression("^[a-z]{2,3}(-[A-Za-z]{2})?$").match(language).hasMatch())
        language = "en";
    const QString state = qEnvironmentVariable("XDG_STATE_HOME", QDir::homePath() + "/.local/state");
    QFile colors(state + "/omarchy/current/theme/colors.toml");
    QString accent = "#FFD60A";
    if (colors.open(QIODevice::ReadOnly)) {
        const auto match = QRegularExpression(
            R"re(^\s*accent\s*=\s*["'](#[0-9a-fA-F]{6})["'])re",
            QRegularExpression::MultilineOption).match(QString::fromUtf8(colors.readAll()));
        if (match.hasMatch()) accent = match.captured(1);
    }
    if (language == m_language && accent == m_accent) return;
    if (language != m_language) {
        QFile catalog(":/i18n/" + language + ".json");
        if (!catalog.open(QIODevice::ReadOnly) && language.contains('-')) {
            catalog.setFileName(":/i18n/" + language.section('-', 0, 0) + ".json");
            if (!catalog.open(QIODevice::ReadOnly)) catalog.setFileName(QString());
        }
        m_strings = catalog.isOpen() ? QJsonDocument::fromJson(catalog.readAll()).object() : QJsonObject{};
        m_language = language;
        const QLocale locale(language);
        QLocale::setDefault(locale);
        QGuiApplication::setLayoutDirection(locale.textDirection());
    }
    m_accent = accent;
    emit changed();
}
