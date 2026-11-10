#include "../src/backend.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

class BackendTests : public QObject {
    Q_OBJECT
private slots:
    void profilesAndRenderer();
    void existingConfigIsBackedUp();
    void invalidProfilesArePreserved();
    void symbolicLinkIsNotOverwritten();
    void statusAndOriginalLayout();
    void changedLayoutIsNotRestored();
    void invalidSnapshotIsPreserved();
};

void BackendTests::profilesAndRenderer() {
    QTemporaryDir tmp;
    Backend model(tmp.path());
    QCOMPARE(model.profiles().size(), 1);
    QVERIFY(model.addProfile("Work"));
    QVERIFY(!model.addProfile("Work"));
    QVERIFY(model.addButton("Theme", "omarchy menu toggle appearance.theme"));
    QVERIFY(model.addButton("Keys", "omarchy menu keybindings"));
    QVERIFY(model.moveButton(1, 0));
    QCOMPARE(model.buttons().first().toMap().value("label").toString(), QString("Keys"));
    QVERIFY(model.apply());
    QFile file(tmp.path() + "/omarchy-glance/touchbar.json");
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto layers = QJsonDocument::fromJson(file.readAll()).object().value("layers").toObject();
    const auto buttons = layers.value("default").toObject().value("left").toArray();
    QCOMPARE(buttons.size(), 3);
    QCOMPARE(buttons.first().toObject().value("id").toString(), QString("glance.esc"));
    QVERIFY(buttons[1].toObject().value("exec").toString().contains(
        QString::fromLatin1(QByteArray("omarchy menu keybindings").toBase64())));
    QCOMPARE(layers.value("fn").toObject().value("left").toArray().size(), 13);
    Backend reload(tmp.path());
    QCOMPARE(reload.profiles().last(), QString("Work"));
    QCOMPARE(reload.buttons().size(), 2);
}

void BackendTests::existingConfigIsBackedUp() {
    QTemporaryDir tmp;
    QDir().mkpath(tmp.path() + "/omarchy-glance");
    QFile file(tmp.path() + "/omarchy-glance/touchbar.json");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("original");
    file.close();
    Backend model(tmp.path());
    QVERIFY(model.apply());
    const auto backups = QDir(tmp.path() + "/omarchy-glance").entryList({"touchbar.json.backup-*"});
    QCOMPARE(backups.size(), 1);
    QFile backup(tmp.path() + "/omarchy-glance/" + backups.first());
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QCOMPARE(backup.readAll(), QByteArray("original"));
}

void BackendTests::invalidProfilesArePreserved() {
    QTemporaryDir tmp;
    QDir().mkpath(tmp.path() + "/tbar");
    QFile file(tmp.path() + "/tbar/profiles.json");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("invalid");
    file.close();
    Backend model(tmp.path());
    QVERIFY(!model.addProfile("Test"));
    QVERIFY(!model.apply());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("invalid"));
}

void BackendTests::symbolicLinkIsNotOverwritten() {
    QTemporaryDir tmp;
    QDir().mkpath(tmp.path() + "/omarchy-glance");
    QFile original(tmp.path() + "/original");
    QVERIFY(original.open(QIODevice::WriteOnly));
    original.write("keep");
    original.close();
    QVERIFY(QFile::link(original.fileName(), tmp.path() + "/omarchy-glance/touchbar.json"));
    Backend model(tmp.path());
    QVERIFY(!model.apply());
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), QByteArray("keep"));
}

void BackendTests::statusAndOriginalLayout() {
    QTemporaryDir tmp;
    Backend model(tmp.path());
    QVERIFY(model.addStatus("internet", "Net", ""));
    QVERIFY(model.addStatus("temperature", "CPU", ""));
    QVERIFY(model.addStatus("process", "Browser", "firefox"));
    QVERIFY(!model.addStatus("process", "Oops", "firefox;id"));
    const auto items = model.rendererConfig().value("layers").toObject()
                           .value("default").toObject().value("left").toArray();
    const auto last = items.last().toObject();
    QCOMPARE(last.value("type").toString(), QString("command"));
    QCOMPARE(last.value("interval").toInt(), 5);
    QVERIFY(last.value("exec").toString().contains(" status process "));
    QVERIFY(model.apply());
    QVERIFY(model.restoreOriginal());
    QVERIFY(!QFile::exists(tmp.path() + "/omarchy-glance/touchbar.json"));
}

void BackendTests::changedLayoutIsNotRestored() {
    QTemporaryDir tmp;
    QDir().mkpath(tmp.path() + "/omarchy-glance");
    const QString path = tmp.path() + "/omarchy-glance/touchbar.json";
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("original");
    file.close();
    Backend model(tmp.path());
    QVERIFY(model.apply());
    QVERIFY(model.restoreOriginal());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("original"));
    file.close();
    QVERIFY(model.apply());
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("external change");
    file.close();
    QVERIFY(!model.restoreOriginal());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("external change"));
}

void BackendTests::invalidSnapshotIsPreserved() {
    QTemporaryDir tmp;
    QDir().mkpath(tmp.path() + "/omarchy-glance");
    QFile file(tmp.path() + "/omarchy-glance/touchbar.json");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("original");
    file.close();
    Backend model(tmp.path());
    QVERIFY(model.apply());
    QFile snapshot(tmp.path() + "/tbar/original-renderer.json");
    QVERIFY(snapshot.open(QIODevice::ReadOnly));
    const auto valid = QJsonDocument::fromJson(snapshot.readAll()).object();
    snapshot.close();
    for (const auto &bad : {QJsonValue(QJsonValue::Undefined), QJsonValue("!!!")}) {
        auto modified = valid;
        if (bad.isUndefined()) modified.remove("data");
        else modified.insert("data", bad);
        QVERIFY(snapshot.open(QIODevice::WriteOnly | QIODevice::Truncate));
        snapshot.write(QJsonDocument(modified).toJson());
        snapshot.close();
        QVERIFY(!model.restoreOriginal());
        QVERIFY(!model.apply());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll() != QByteArray("original"));
        file.close();
    }
}

QTEST_GUILESS_MAIN(BackendTests)
#include "tbar_tests.moc"
