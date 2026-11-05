#include "storage/Database.h"
#include "storage/HistoryModel.h"
#include "storage/HistoryRepository.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

class TstHistoryRepository : public QObject
{
    Q_OBJECT
private slots:
    void upsertAndPlayCount()
    {
        Database db(":memory:");
        HistoryRepository r(&db);
        QVERIFY(r.available());
        r.recordPlay("/a.mp4", "a", 100);
        r.recordPlay("/a.mp4", "a", 0);
        const auto e = r.entry("/a.mp4");
        QCOMPARE(e.playCount, 2);
        QCOMPARE(e.duration, 100.0); // later zero duration must not erase it
        QCOMPARE(r.list(HistoryRepository::Filter::All).size(), 1);
        QVERIFY(r.entry("/missing").path.isEmpty());
    }

    void orderingAndProgress()
    {
        Database db(":memory:");
        HistoryRepository r(&db);
        r.recordPlay("/a", "a", 100);
        r.recordPlay("/b", "b", 100);
        r.recordPlay("/c", "c", 100); // same-second timestamps: rowid breaks ties
        r.updateProgress("/a", 50, 100);
        r.updateProgress("/b", 98, 100); // within 10 s of end: not resumable
        r.updateProgress("/c", 3, 100);  // too early: not resumable
        const auto all = r.list(HistoryRepository::Filter::All);
        QCOMPARE(all.first().path, QString("/c"));
        QCOMPARE(all.last().path, QString("/a"));
        const auto cont = r.list(HistoryRepository::Filter::Continue);
        QCOMPARE(cont.size(), 1);
        QCOMPARE(cont.first().path, QString("/a"));
        QCOMPARE(r.entry("/a").position, 50.0);
        r.updateProgress("/a", 0, 100); // finished clears resume
        QVERIFY(r.list(HistoryRepository::Filter::Continue).isEmpty());
        QCOMPARE(r.list(HistoryRepository::Filter::All, 2).size(), 2); // limit
    }

    void favorites()
    {
        Database db(":memory:");
        HistoryRepository r(&db);
        r.recordPlay("/a", "a");
        r.recordPlay("/b", "b");
        r.setFavorite("/a", true);
        QCOMPARE(r.list(HistoryRepository::Filter::Favorites).size(), 1);
        r.recordPlay("/a", "a"); // replaying keeps the flag
        QVERIFY(r.entry("/a").favorite);
        r.setFavorite("/a", false);
        QVERIFY(r.list(HistoryRepository::Filter::Favorites).isEmpty());
    }

    void modelRolesAndReload()
    {
        Database db(":memory:");
        HistoryRepository r(&db);
        HistoryModel m(&r, HistoryRepository::Filter::All, 10);
        QCOMPARE(m.rowCount(), 0);
        r.recordPlay("/a", "Title A", 200);
        r.updateProgress("/a", 50, 200);
        m.reload();
        QCOMPARE(m.rowCount(), 1);
        QCOMPARE(m.data(m.index(0), HistoryModel::TitleRole).toString(), QString("Title A"));
        QCOMPARE(m.data(m.index(0), HistoryModel::ProgressRole).toDouble(), 0.25);
    }

    void migrationAndLegacyImport()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("lib.db");
        { Database db(path); QVERIFY(db.valid()); QCOMPARE(db.schemaVersion(), Database::kSchemaVersion); }
        Database db(path); // reopen: no re-migration errors, version stays
        QVERIFY(db.valid());
        QCOMPARE(db.schemaVersion(), Database::kSchemaVersion);
        HistoryRepository r(&db);
        HistoryEntry e;
        e.path = "/old"; e.title = "old"; e.position = 42; e.playCount = 1; e.lastPlayed = 10;
        r.importEntry(e);
        e.position = 1;
        r.importEntry(e); // never overwrites
        QCOMPARE(r.entry("/old").position, 42.0);
    }

    void newerSchemaDisablesLibrary()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("lib.db");
        { Database db(path); QSqlQuery(db.db()).exec("UPDATE schema_version SET version=99"); }
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("newer"));
        Database db(path);
        QVERIFY(!db.valid());
    }

    void unavailableIsNoOp()
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Library disabled"));
        Database db("/nonexistent-dir-xyz/sub/lib.db");
        QVERIFY(!db.valid());
        HistoryRepository r(&db);
        QVERIFY(!r.available());
        r.recordPlay("/a", "a");
        r.updateProgress("/a", 10, 100);
        r.setFavorite("/a", true);
        QVERIFY(r.entry("/a").path.isEmpty());
        QVERIFY(r.list(HistoryRepository::Filter::All).isEmpty());
        HistoryRepository none(nullptr);
        QVERIFY(!none.available());
        none.recordPlay("/a", "a"); // must not crash
    }
};

QTEST_MAIN(TstHistoryRepository)
#include "tst_HistoryRepository.moc"
