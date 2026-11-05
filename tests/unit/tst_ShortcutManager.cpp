#include "core/ShortcutManager.h"

#include <QCoreApplication>
#include <QSet>
#include <QSettings>
#include <QSignalSpy>
#include <QtTest>

class TstShortcutManager : public QObject
{
    Q_OBJECT
private slots:
    void init()
    {
        QCoreApplication::setOrganizationName("LuminaTest");
        QCoreApplication::setApplicationName("tst_shortcutmanager");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings().clear();
    }
    void cleanup() { QSettings().clear(); }

    void defaults()
    {
        ShortcutManager m;
        QCOMPARE(m.sequence("playPause"), QString("Space"));
        QCOMPARE(m.sequence("seekForward"), QString("Right"));
        QCOMPARE(m.sequence("settings"), QString("Ctrl+,"));
        QVERIFY(m.actions().size() > 30);
        QSet<QString> seen; // defaults are unique and bound
        for (const auto &v : m.actions()) {
            const QString k = v.toMap()["key"].toString();
            QVERIFY(!k.isEmpty());
            QVERIFY2(!seen.contains(k), qPrintable(k));
            seen.insert(k);
        }
    }
    void rebindAndPersist()
    {
        {
            ShortcutManager m;
            QSignalSpy spy(&m, &ShortcutManager::changed);
            QCOMPARE(m.setSequence("mute", "N"), QString("N"));
            QCOMPARE(spy.count(), 1);
            QCOMPARE(m.sequence("mute"), QString("N"));
        }
        ShortcutManager m2; // new instance reads QSettings
        QCOMPARE(m2.sequence("mute"), QString("N"));
        QCOMPARE(m2.sequence("playPause"), QString("Space"));
    }
    void conflicts()
    {
        ShortcutManager m;
        QCOMPARE(m.conflict("mute", "Space"), QString("playPause"));
        QCOMPARE(m.conflict("mute", "M"), QString()); // own binding is not a conflict
        QCOMPARE(m.conflict("mute", "Q"), QString());
        QCOMPARE(m.conflict("mute", ""), QString());
        m.swap("mute", "Space");
        QCOMPARE(m.sequence("mute"), QString("Space"));
        QCOMPARE(m.sequence("playPause"), QString("M"));
    }
    void clearPersists()
    {
        { ShortcutManager m; m.setSequence("mute", ""); }
        ShortcutManager m2;
        QCOMPARE(m2.sequence("mute"), QString());
    }
    void reset()
    {
        ShortcutManager m;
        m.setSequence("mute", "N");
        m.setSequence("fullscreen", "");
        m.reset("mute");
        QCOMPARE(m.sequence("mute"), QString("M"));
        QCOMPARE(m.sequence("fullscreen"), QString());
        m.resetAll();
        QCOMPARE(m.sequence("fullscreen"), QString("F"));
        ShortcutManager m2;
        QCOMPARE(m2.sequence("mute"), QString("M"));
        QCOMPARE(m2.sequence("fullscreen"), QString("F"));
    }
    void keyEvents()
    {
        ShortcutManager m;
        QCOMPARE(m.fromKeyEvent(Qt::Key_N, 0), QString("N"));
        QCOMPARE(m.fromKeyEvent(Qt::Key_S, Qt::ControlModifier | Qt::ShiftModifier), QString("Ctrl+Shift+S"));
        QCOMPARE(m.fromKeyEvent(Qt::Key_Shift, Qt::ShiftModifier), QString());
    }
};

QTEST_GUILESS_MAIN(TstShortcutManager)
#include "tst_ShortcutManager.moc"
