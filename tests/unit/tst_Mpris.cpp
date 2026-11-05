#include <QtTest>

#include "platform/Mpris.h"

class TestMpris : public QObject
{
    Q_OBJECT

    static MediaState playing()
    {
        MediaState s;
        s.status = MediaState::Playing;
        s.title = QStringLiteral("Movie");
        s.url = QUrl::fromLocalFile(QStringLiteral("/tmp/a b.mp4"));
        s.duration = 60.5;
        s.hasVideo = true;
        return s;
    }

private slots:
    void statusStrings()
    {
        MediaState s;
        QCOMPARE(Mpris::playbackStatus(s), QStringLiteral("Stopped"));
        s.status = MediaState::Paused;
        QCOMPARE(Mpris::playbackStatus(s), QStringLiteral("Paused"));
        QCOMPARE(Mpris::playbackStatus(playing()), QStringLiteral("Playing"));
    }

    void metadataMapping()
    {
        QVERIFY(Mpris::metadata(MediaState{}).isEmpty());
        const QVariantMap m = Mpris::metadata(playing());
        QCOMPARE(m.value("mpris:trackid").toString(), Mpris::kTrackPath);
        QCOMPARE(m.value("mpris:length").toLongLong(), 60500000LL);
        QCOMPARE(m.value("xesam:title").toString(), QStringLiteral("Movie"));
        QCOMPARE(m.value("xesam:url").toString(), QStringLiteral("file:///tmp/a%20b.mp4"));
    }

    void changedPropertiesIgnoresPosition()
    {
        MediaState a = playing(), b = a;
        b.position = 12;
        QVERIFY(Mpris::changedProperties(a, b).isEmpty());
        b.status = MediaState::Paused;
        b.volume = 0.5;
        b.canNext = true;
        const QVariantMap c = Mpris::changedProperties(a, b);
        QCOMPARE(c.value("PlaybackStatus").toString(), QStringLiteral("Paused"));
        QCOMPARE(c.value("Volume").toDouble(), 0.5);
        QVERIFY(c.value("CanGoNext").toBool());
        QVERIFY(!c.contains("Metadata"));
        b.title = QStringLiteral("Other");
        QVERIFY(Mpris::changedProperties(a, b).contains("Metadata"));
    }

    void inhibitOnlyWhilePlayingVideo()
    {
        MediaState s = playing();
        QVERIFY(Mpris::wantsInhibit(s));
        s.hasVideo = false;
        QVERIFY(!Mpris::wantsInhibit(s));
        s = playing();
        s.status = MediaState::Paused;
        QVERIFY(!Mpris::wantsInhibit(s));
    }

    void seekIsClamped()
    {
        MediaState s = playing();
        s.position = 10;
        QCOMPARE(Mpris::seekTarget(s, 5000000), 15.0);
        QCOMPARE(Mpris::seekTarget(s, -99000000), 0.0);
        QCOMPARE(Mpris::seekTarget(s, 999000000), 60.5);
    }
};

QTEST_APPLESS_MAIN(TestMpris)
#include "tst_Mpris.moc"
