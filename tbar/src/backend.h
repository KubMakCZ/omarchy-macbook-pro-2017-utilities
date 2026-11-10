#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>

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
    void setPreviewFn(bool enabled);
    Q_INVOKABLE void startPreview();
    Q_INVOKABLE bool select(int index);
    Q_INVOKABLE bool addProfile(const QString &name);
    Q_INVOKABLE bool deleteProfile();
    Q_INVOKABLE bool addButton(const QString &label, const QString &command);
    Q_INVOKABLE bool addStatus(const QString &kind, const QString &label, const QString &argument);
    Q_INVOKABLE bool addWidget(const QString &kind);
    Q_INVOKABLE bool addExamples();
    Q_INVOKABLE bool removeButton(int index);
    Q_INVOKABLE bool moveButton(int from, int to);
    Q_INVOKABLE bool apply();
    Q_INVOKABLE bool restoreOriginal();
    QJsonObject rendererConfig() const;

signals:
    void changed();
    void messageChanged();
    void previewChanged();

private:
    bool save(const QJsonArray &profiles, int selected);
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
};
