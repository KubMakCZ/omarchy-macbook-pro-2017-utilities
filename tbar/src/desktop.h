#pragma once

#include <QJsonObject>
#include <QTimer>
#include <QTranslator>

class Desktop : public QTranslator {
    Q_OBJECT
    Q_PROPERTY(QString accent READ accent NOTIFY changed)
    Q_PROPERTY(QString language READ language NOTIFY changed)
public:
    Desktop();
    QString accent() const { return m_accent; }
    QString language() const { return m_language; }
    bool isEmpty() const override { return false; }
    QString translate(const char *, const char *source, const char *, int = -1) const override;
signals:
    void changed();
private:
    void reload();
    QTimer m_timer;
    QString m_accent, m_language;
    QJsonObject m_strings;
};
