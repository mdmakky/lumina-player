#include "SingleInstance.h"

#include <QDir>
#include <QSignalSpy>
#include <QtTest>

class tst_SingleInstance : public QObject
{
    Q_OBJECT
private slots:
    void noServerMeansNoForward()
    {
        SingleInstance s(QStringLiteral("lumina-test-none"));
        QVERIFY(!s.forward({QStringLiteral("a.mp4")}));
    }

    void forwardsArgumentsAndAbsolutizes()
    {
        const QString name = QStringLiteral("lumina-test-fwd");
        SingleInstance primary(name);
        QVERIFY(primary.listen());
        QSignalSpy spy(&primary, &SingleInstance::argumentsReceived);

        SingleInstance second(name);
        QVERIFY(second.forward({QStringLiteral("rel.mp4"), QStringLiteral("http://x/y.mp4")}));
        QVERIFY(spy.wait(2000));
        const QStringList got = spy.first().first().toStringList();
        QCOMPARE(got.size(), 2);
        QVERIFY(QDir::isAbsolutePath(got.at(0)));
        QVERIFY(got.at(0).endsWith(QStringLiteral("/rel.mp4")));
        QCOMPARE(got.at(1), QStringLiteral("http://x/y.mp4"));
    }
};

QTEST_MAIN(tst_SingleInstance)
#include "tst_SingleInstance.moc"
