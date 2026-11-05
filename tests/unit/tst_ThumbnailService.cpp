#include "engine/MpvThumbnailer.h"
#include "media/ThumbnailCache.h"

#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QtTest>

#include <algorithm>

class TstThumbnailService : public QObject
{
    Q_OBJECT
private slots:
    void buckets()
    {
        QCOMPARE(ThumbnailCache::bucketOf(-3), 0);
        QCOMPARE(ThumbnailCache::bucketOf(1.99), 0);
        QCOMPARE(ThumbnailCache::bucketOf(2.0), 1);
        QCOMPARE(ThumbnailCache::bucketOf(61.0), 30);
        QCOMPARE(ThumbnailCache::timeOf(30), 60.0);
    }

    void lruEvictsLeastRecentlyUsed()
    {
        ThumbnailCache c(3);
        QImage img(2, 2, QImage::Format_RGB32);
        c.insert(1, img);
        c.insert(2, img);
        c.insert(3, img);
        QVERIFY(c.has(1)); // touch 1 -> 2 is now the oldest
        c.insert(4, img);
        QCOMPARE(c.size(), 3);
        QVERIFY(!c.has(2));
        QVERIFY(c.has(1) && c.has(3) && c.has(4));
        c.clear();
        QCOMPARE(c.size(), 0);
    }

    // Needs a real clip: LUMINA_TEST_MEDIA=/tmp/long.mp4 (skipped otherwise).
    void grabsFrames()
    {
        const QString media = qEnvironmentVariable("LUMINA_TEST_MEDIA");
        if (media.isEmpty())
            QSKIP("LUMINA_TEST_MEDIA not set");
        MpvThumbnailer g;
        QElapsedTimer t;
        t.start();
        QVERIFY(g.open(media));
        qInfo() << "thumbnailer open ms" << t.elapsed();
        QList<qint64> lat;
        const double dur = qEnvironmentVariable("LUMINA_TEST_DURATION", "50").toDouble();
        for (int i = 0; i < 30; ++i) {
            t.restart();
            const QImage img = g.grab(QRandomGenerator::global()->generateDouble() * dur);
            lat.append(t.elapsed());
            QVERIFY(!img.isNull());
            QVERIFY(img.width() >= 200 && img.width() <= 260);
        }
        std::sort(lat.begin(), lat.end());
        qInfo() << "grab latency ms p50" << lat[lat.size() / 2] << "p90" << lat[lat.size() * 9 / 10] << "max" << lat.last();
    }
};

QTEST_MAIN(TstThumbnailService)
#include "tst_ThumbnailService.moc"
