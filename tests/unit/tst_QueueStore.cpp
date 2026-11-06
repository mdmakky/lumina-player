#include "core/QueueStore.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace Lumina::QueueStore;

class TstQueueStore : public QObject {
    Q_OBJECT
private slots:
    void roundTrip()
    {
        QTemporaryDir d;
        const QString p = d.filePath("q.json");
        QVERIFY(save(p, {{"file:///a.mp4", "http://x/y.mkv"}, 1}));
        const Queue q = load(p);
        QCOMPARE(q.urls, (QStringList{"file:///a.mp4", "http://x/y.mkv"}));
        QCOMPARE(q.index, 1);
    }
    void badInput()
    {
        QVERIFY(fromJson("garbage").urls.isEmpty());
        QCOMPARE(fromJson(R"({"urls":["a",3,""],"index":9})").index, -1);
        QCOMPARE(fromJson(R"({"urls":["a",3,""],"index":0})").urls, QStringList{"a"});
        QVERIFY(load("/nonexistent/q.json").urls.isEmpty());
    }
};
QTEST_APPLESS_MAIN(TstQueueStore)
#include "tst_QueueStore.moc"
