#include <QtTest>

#include "platform/CodecFixer.h"

class tst_CodecFixer : public QObject
{
    Q_OBJECT
private slots:
    void scriptForValidRelease()
    {
        const QString s = CodecFixer::buildScript("45");
        QVERIFY(s.contains("rpmfusion-free-release-45.noarch.rpm"));
        QVERIFY(s.contains("dnf install -y libavcodec-freeworld"));
    }
    void rejectsAnythingButDigits()
    {
        QVERIFY(CodecFixer::buildScript("").isEmpty());
        QVERIFY(CodecFixer::buildScript("4").isEmpty());
        QVERIFY(CodecFixer::buildScript("1234").isEmpty());
        QVERIFY(CodecFixer::buildScript("45; rm -rf /").isEmpty());
        QVERIFY(CodecFixer::buildScript("$(id)").isEmpty());
        QVERIFY(CodecFixer::buildScript("45\n").isEmpty());
    }
    void unavailableOffFedora()
    {
#ifndef Q_OS_LINUX
        CodecFixer f;
        QVERIFY(!f.available());
#endif
    }
};

QTEST_GUILESS_MAIN(tst_CodecFixer)
#include "tst_CodecFixer.moc"
