#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList profiles READ profiles NOTIFY changed)
    Q_PROPERTY(QVariantList buttons READ buttons NOTIFY changed)
    Q_PROPERTY(int selected READ selected NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(bool rendererAvailable READ rendererAvailable CONSTANT)

public:
    explicit Backend(const QString &configHome = {}, QObject *parent = nullptr);
    QStringList profiles() const;
    QVariantList buttons() const;
    int selected() const { return m_selected; }
    QString message() const { return m_message; }
    bool rendererAvailable() const;
    Q_INVOKABLE bool select(int index);
    Q_INVOKABLE bool addProfile(const QString &name);
    Q_INVOKABLE bool deleteProfile();
    Q_INVOKABLE bool addButton(const QString &label, const QString &command);
    Q_INVOKABLE bool addStatus(const QString &kind, const QString &label, const QString &argument);
    Q_INVOKABLE bool removeButton(int index);
    Q_INVOKABLE bool moveButton(int from, int to);
    Q_INVOKABLE bool apply();
    Q_INVOKABLE bool restoreOriginal();
    QJsonObject rendererConfig() const;

signals:
    void changed();
    void messageChanged();

private:
    bool save(const QJsonArray &profiles, int selected);
    bool fail(const QString &message);
    void setMessage(const QString &message);
    QString m_home;
    QJsonArray m_profiles;
    int m_selected = 0;
    QString m_message;
};
