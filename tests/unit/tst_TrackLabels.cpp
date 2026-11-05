#include "utilities/TrackLabels.h"

#include <QSet>
#include <QTest>

using namespace Lumina::TrackLabels;

static QVariantMap track(int id, const QString &lang, const QString &codec, int ch = 0,
                         const QString &title = {}, bool def = false)
{
    return {{"id", id}, {"lang", lang}, {"codec", codec}, {"demux-channel-count", ch},
            {"title", title}, {"default", def}};
}

class TstTrackLabels : public QObject
{
    Q_OBJECT
private slots:
    void languages_data()
    {
        QTest::addColumn<QString>("code");
        QTest::addColumn<QString>("name");
        QTest::newRow("en") << "en" << "English";
        QTest::newRow("eng") << "eng" << "English";
        QTest::newRow("bn") << "bn" << "Bangla";
        QTest::newRow("ben") << "ben" << "Bangla";
        QTest::newRow("jpn") << "JPN" << "Japanese";
        QTest::newRow("ja") << "ja" << "Japanese";
        QTest::newRow("chi") << "chi" << "Chinese";
        QTest::newRow("zho") << "zho" << "Chinese";
        QTest::newRow("region") << "pt-BR" << "Portuguese";
        QTest::newRow("ger") << "ger" << "German";
        QTest::newRow("und") << "und" << "";
        QTest::newRow("empty") << "" << "";
        QTest::newRow("junk") << "xx" << "";
    }
    void languages()
    {
        QFETCH(QString, code);
        QFETCH(QString, name);
        QCOMPARE(languageName(code), name);
    }

    void layouts()
    {
        QCOMPARE(channelLayout(2), QString("Stereo"));
        QCOMPARE(channelLayout(6), QString("5.1"));
        QCOMPARE(channelLayout(3), QString("3 ch"));
        QCOMPARE(channelLayout(0), QString());
    }

    void describesMixedTracks()
    {
        QVariantList in{track(1, "eng", "aac", 2, {}, true), track(2, "eng", "ac3", 6),
                        track(3, "jpn", "aac", 2, "Director Commentary")};
        const QVariantList out = describeTracks(in, true);
        QCOMPARE(out[0].toMap()["label"].toString(), QString("English"));
        QCOMPARE(out[0].toMap()["detail"].toString(), QString("AAC · Stereo · Default"));
        QCOMPARE(out[1].toMap()["detail"].toString(), QString("AC3 · 5.1"));
        QCOMPARE(out[2].toMap()["label"].toString(), QString("Japanese"));
        QVERIFY(out[2].toMap()["detail"].toString().contains("Commentary"));
    }

    void labelsAreNeverIdentical()
    {
        QVariantList in{track(1, "eng", "aac", 2), track(2, "eng", "aac", 2), track(3, "eng", "aac", 2),
                        track(4, "", "aac", 2), track(5, "und", "aac", 2)};
        const QVariantList out = describeTracks(in, true);
        QSet<QString> keys;
        for (const QVariant &v : out)
            keys.insert(v.toMap()["label"].toString() + "|" + v.toMap()["detail"].toString());
        QCOMPARE(keys.size(), in.size());
        QCOMPARE(out[1].toMap()["label"].toString(), QString("English #2"));
        QCOMPARE(out[3].toMap()["label"].toString(), QString("Unknown"));
    }

    void externalSubtitle()
    {
        QVariantMap s = track(3, "", "subrip");
        s["external"] = true;
        s["external-filename"] = "/tmp/movie.en.srt";
        const QVariantMap m = describeTracks({s}, false)[0].toMap();
        QCOMPARE(m["label"].toString(), QString("External"));
        QCOMPARE(m["detail"].toString(), QString("SRT · movie.en.srt"));
    }
};

QTEST_APPLESS_MAIN(TstTrackLabels)
#include "tst_TrackLabels.moc"
