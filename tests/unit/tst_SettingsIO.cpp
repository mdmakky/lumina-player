#include "utilities/SettingsIO.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

using namespace Lumina::SettingsIO;

class TstSettingsIO : public QObject
{
    Q_OBJECT
private slots:
    void roundTripSkipsPrivateData()
    {
        QTemporaryDir dir;
        QSettings a(dir.filePath("a.ini"), QSettings::IniFormat);
        a.setValue("interface/theme", "oled");
        a.setValue("playback/seekStep", 10);
        a.setValue("playback/resume", true);
        a.setValue("shortcuts/mute", "Ctrl+M");
        a.setValue("resume/abc123", 42.5);
        a.setValue("recentFiles", QStringList{"/a.mkv"});
        a.setValue("history/migrated", true);
        a.setValue("window/geometry", "x");
        a.setValue("library/folders", QStringList{"/m"});

        const QByteArray json = exportJson(a);
        QVERIFY(!json.contains("resume/abc123"));
        QVERIFY(!json.contains("recentFiles"));
        QVERIFY(!json.contains("library/"));

        QSettings b(dir.filePath("b.ini"), QSettings::IniFormat);
        QString err;
        QCOMPARE(importJson(b, json, &err), 4);
        QCOMPARE(b.value("interface/theme").toString(), QString("oled"));
        QCOMPARE(b.value("playback/seekStep").toInt(), 10);
        QCOMPARE(b.value("playback/resume").toBool(), true);
        QCOMPARE(b.value("shortcuts/mute").toString(), QString("Ctrl+M"));
        QVERIFY(!b.contains("resume/abc123"));
    }

    void rejectsGarbage()
    {
        QTemporaryDir dir;
        QSettings s(dir.filePath("c.ini"), QSettings::IniFormat);
        QString err;
        QCOMPARE(importJson(s, "not json", &err), -1);
        QVERIFY(!err.isEmpty());
        QCOMPARE(importJson(s, R"({"hello":1})", &err), -1);
        QCOMPARE(importJson(s, R"({"app":"lumina-player-settings","version":99,"settings":{}})", &err), -1);
        QVERIFY(s.allKeys().isEmpty());
    }

    void importIgnoresExcludedAndBadKeys()
    {
        QTemporaryDir dir;
        QSettings s(dir.filePath("d.ini"), QSettings::IniFormat);
        const QByteArray j = R"({"app":"lumina-player-settings","version":1,"settings":
            {"resume/x":1,"recentUrls":["u"],"interface/language":"bn","a/b":{"nested":1},"../evil":1}})";
        QCOMPARE(importJson(s, j), 1);
        QCOMPARE(s.value("interface/language").toString(), QString("bn"));
    }
};

QTEST_APPLESS_MAIN(TstSettingsIO)
#include "tst_SettingsIO.moc"
