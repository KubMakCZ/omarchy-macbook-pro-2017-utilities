#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList profiles READ profiles NOTIFY changed)
    Q_PROPERTY(QVariantList buttons READ buttons NOTIFY changed)
    Q_PROPERTY(int selected READ selected NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(bool rendererAvailable READ rendererAvailable CONSTANT)
    Q_PROPERTY(QUrl preview READ preview NOTIFY previewChanged)
    Q_PROPERTY(QString previewError READ previewError NOTIFY previewChanged)
    Q_PROPERTY(bool previewFn READ previewFn WRITE setPreviewFn NOTIFY previewChanged)
    Q_PROPERTY(QString themeStyle READ themeStyle NOTIFY changed)
    Q_PROPERTY(QString apps READ apps NOTIFY changed)
    Q_PROPERTY(QVariantMap look READ look NOTIFY changed)
    Q_PROPERTY(QVariantMap saver READ saver NOTIFY changed)
    Q_PROPERTY(QVariantMap keys READ keys NOTIFY changed)
    Q_PROPERTY(QString applied READ appliedProfile NOTIFY appliedChanged)

public:
    explicit Backend(const QString &configHome = {}, QObject *parent = nullptr);
    QStringList profiles() const;
    QVariantList buttons() const;
    int selected() const { return m_selected; }
    QString message() const { return m_message; }
    bool rendererAvailable() const;
    QUrl preview() const { return m_preview; }
    QString previewError() const { return m_previewError; }
    bool previewFn() const { return m_previewFn; }
    QString themeStyle() const;
    Q_INVOKABLE bool setThemeStyle(const QString &style, bool everywhere);
    void setPreviewFn(bool enabled);
    QString apps() const;
    QVariantMap look() const;
    QVariantMap saver() const;
    QVariantMap keys() const;
    QString appliedProfile() const;
    QString manualProfile() const;
    QString nextProfile() const;
    QString targetForApp(const QString &windowClass) const;
    bool switchProfile(const QString &name, bool manual);
    Q_INVOKABLE void startPreview();
    Q_INVOKABLE bool select(int index);
    Q_INVOKABLE bool addProfile(const QString &name);
    Q_INVOKABLE bool deleteProfile();
    Q_INVOKABLE bool addButton(const QString &label, const QString &command);
    Q_INVOKABLE bool addStatus(const QString &kind, const QString &label, const QString &argument);
    Q_INVOKABLE bool addWidget(const QString &kind);
    Q_INVOKABLE bool addExamples();
    Q_INVOKABLE bool addSpacer();
    Q_INVOKABLE bool addControl(const QString &kind);
    Q_INVOKABLE bool addSceneButton(const QString &label, const QString &target);
    Q_INVOKABLE bool setApps(const QString &list);
    Q_INVOKABLE bool setLook(const QString &escLabel, const QString &escIcon, const QString &font, int idleDim);
    Q_INVOKABLE bool setItemIcon(int index, const QString &icon);
    Q_INVOKABLE bool setKeys(int radius, int opacity, const QString &animation, bool everywhere);
    Q_INVOKABLE bool setSaver(int seconds, const QString &style, const QString &text, const QString &image,
                              const QString &background, bool everywhere);
    Q_INVOKABLE bool editItem(int index, const QString &section, int width,
                              const QString &style, int interval, bool temperature);
    Q_INVOKABLE bool setThemeAuto(bool enabled);
    Q_INVOKABLE bool removeButton(int index);
    Q_INVOKABLE bool moveButton(int from, int to);
    Q_INVOKABLE bool apply();
    Q_INVOKABLE bool restoreOriginal();
    bool syncTheme();
    QJsonObject rendererConfig() const;
    QJsonObject profile(const QString &name) const;
    static QString validateProfile(const QJsonObject &profile);
    bool importProfile(const QJsonObject &profile, bool replace);
    bool selectProfile(const QString &name);
    bool applyProfile(const QString &name);
    QJsonObject exportProfile(const QString &name);

signals:
    void changed();
    void messageChanged();
    void previewChanged();
    void appliedChanged();

private:
    bool save(const QJsonArray &profiles, int selected, bool backup = false);
    bool profilesUnchanged();
    bool appendItem(const QJsonObject &item);
    bool write(bool quick, bool manual);
    QJsonObject state() const;
    bool fail(const QString &message);
    void setMessage(const QString &message);
    void updatePreview();
    QString m_home;
    QJsonArray m_profiles;
    int m_selected = 0;
    QString m_message;
    QTemporaryDir m_previewDir;
    QProcess m_previewProcess;
    QTimer m_previewTimer;
    QTimer m_previewTimeout;
    QUrl m_preview;
    QString m_previewError;
    bool m_previewFn = false;
    bool m_previewStarted = false;
    int m_previewRevision = 0;
    int m_previewRequested = 0;
    bool m_profilesOnDisk = false;
    QByteArray m_profilesHash;
};
