#include "utilities/UrlUtils.h"

#include <QTest>

using namespace Lumina::UrlUtils;

class TstUrlUtils : public QObject
{
    Q_OBJECT
private slots:
    void accepts_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<bool>("ok");
        QTest::newRow("http") << "http://example.com/a.mp4" << true;
        QTest::newRow("https trimmed") << "  https://example.com/live/index.m3u8\n" << true;
        QTest::newRow("rtmp") << "rtmp://host/app/stream" << true;
        QTest::newRow("rtsp") << "rtsp://10.0.0.2:554/cam" << true;
        QTest::newRow("ftp") << "ftp://host/file.mkv" << true;
        QTest::newRow("smb") << "smb://nas/share/movie.mkv" << true;
        QTest::newRow("nfs") << "nfs://10.0.0.5/export/movie.mkv" << true;
        QTest::newRow("sftp") << "sftp://user@host/movie.mkv" << true;
        QTest::newRow("dash") << "https://host/stream/manifest.mpd" << true;
        QTest::newRow("file") << "file:///tmp/a.mp4" << true;
        QTest::newRow("no scheme") << "example.com/a.mp4" << false;
        QTest::newRow("unsupported scheme") << "javascript:alert(1)" << false;
        QTest::newRow("no host") << "http://" << false;
        QTest::newRow("spaces") << "http://a.com/a b.mp4" << false;
        QTest::newRow("empty") << "   " << false;
    }
    void accepts()
    {
        QFETCH(QString, in);
        QFETCH(bool, ok);
        QCOMPARE(parseMediaUrl(in).isValid(), ok);
    }

    void optionalProtocols()
    {
        QVERIFY(needsOptionalProtocol(QUrl("smb://nas/a")));
        QVERIFY(needsOptionalProtocol(QUrl("SFTP://h/a")));
        QVERIFY(!needsOptionalProtocol(QUrl("https://h/a.mpd")));
        QVERIFY(looksLikeDirectMedia(QUrl("https://h/x/manifest.mpd")));
        QVERIFY(looksLikeDirectMedia(QUrl("rtsp://h/cam")));
    }

    void titles()
    {
        QCOMPARE(titleFor(QUrl("http://h.com/dir/My%20Movie.mp4?x=1")), QString("My Movie.mp4"));
        QCOMPARE(titleFor(QUrl("http://h.com/dir/")), QString("dir"));
        QCOMPARE(titleFor(QUrl("http://h.com")), QString("h.com"));
    }

    void remote()
    {
        QVERIFY(isRemote(QUrl("https://h.com/a")));
        QVERIFY(!isRemote(QUrl::fromLocalFile("/tmp/a")));
        QVERIFY(!isRemote(QUrl()));
    }
};

QTEST_APPLESS_MAIN(TstUrlUtils)
#include "tst_UrlUtils.moc"
