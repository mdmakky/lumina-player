#include <QtTest>

#include "services/UpdateChecker.h"

class tst_UpdateChecker : public QObject
{
    Q_OBJECT
private slots:
    void versionOrder()
    {
        QVERIFY(UpdateChecker::isNewer("v0.4.0", "0.3.1"));
        QVERIFY(UpdateChecker::isNewer("0.3.10", "0.3.9"));
        QVERIFY(UpdateChecker::isNewer("0.3.1", "0.3.1-preview"));
        QVERIFY(!UpdateChecker::isNewer("0.3.1-preview", "0.3.1"));
        QVERIFY(!UpdateChecker::isNewer("v0.3.1-preview", "0.3.1-preview"));
        QVERIFY(!UpdateChecker::isNewer("0.2.9", "0.3.0"));
    }
    void trustedUrl()
    {
        QVERIFY(UpdateChecker::isTrustedReleaseUrl("https://github.com/mdmakky/lumina-player/releases/tag/v1"));
        QVERIFY(!UpdateChecker::isTrustedReleaseUrl("https://evil.example/mdmakky/lumina-player/"));
        QVERIFY(!UpdateChecker::isTrustedReleaseUrl("http://github.com/mdmakky/lumina-player/"));
    }
    void parseSkipsDraftsAndForeignLinks()
    {
        const QByteArray json =
            "[{\"tag_name\":\"v9.0.0\",\"draft\":true,\"html_url\":\"https://github.com/mdmakky/lumina-player/releases/tag/v9.0.0\"},"
            "{\"tag_name\":\"v8.0.0\",\"draft\":false,\"html_url\":\"https://example.com/x\"},"
            "{\"tag_name\":\"v0.3.1-preview\",\"draft\":false,\"prerelease\":true,\"html_url\":\"https://github.com/mdmakky/lumina-player/releases/tag/v0.3.1-preview\"}]";
        QString tag, url;
        QVERIFY(UpdateChecker::parseLatest(json, &tag, &url));
        QCOMPARE(tag, QString("v0.3.1-preview"));
        QVERIFY(!UpdateChecker::parseLatest("[]", &tag, &url));
    }
};

QTEST_GUILESS_MAIN(tst_UpdateChecker)
#include "tst_UpdateChecker.moc"
