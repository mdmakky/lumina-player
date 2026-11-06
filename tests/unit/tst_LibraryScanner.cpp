#include "services/LibraryScanner.h"
#include "storage/Database.h"
#include "storage/LibraryModel.h"
#include "storage/LibraryRepository.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class TstLibraryScanner : public QObject
{
    Q_OBJECT
    static void touch(const QString &path, const QByteArray &data = "x")
    {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(data);
    }
    static void scanAndWait(LibraryScanner &s, const QStringList &roots)
    {
        QSignalSpy done(&s, &LibraryScanner::finished);
        s.scan(roots);
        QVERIFY(done.wait(10000));
    }

private slots:
    void scanIncrementalRemoveSearch()
    {
        QTemporaryDir dir;
        const QString root = dir.path();
        touch(root + "/Alpha Movie.mp4");
        touch(root + "/sub/beta_clip.mkv");
        touch(root + "/sub/notes.txt"); // not media
        Database db(":memory:");
        LibraryRepository repo(&db);
        LibraryScanner scanner(&repo);
        LibraryModel model(&repo);

        scanAndWait(scanner, {root});
        QCOMPARE(repo.count(), 2);
        QCOMPARE(repo.query("beta", LibraryRepository::ByTitle).size(), 1);
        QCOMPARE(repo.query("sub", LibraryRepository::ByTitle).size(), 1); // folder match
        QCOMPARE(repo.query("100%_", LibraryRepository::ByTitle).size(), 0); // wildcards are escaped

        // incremental: unchanged files are skipped, a new one is picked up
        const auto before = repo.known();
        touch(root + "/gamma.avi");
        scanAndWait(scanner, {root});
        QCOMPARE(repo.count(), 3);
        QCOMPARE(repo.known().value(root + "/Alpha Movie.mp4"), before.value(root + "/Alpha Movie.mp4"));

        // changed file (size) is refreshed
        touch(root + "/gamma.avi", "longer content");
        scanAndWait(scanner, {root});
        QCOMPARE(repo.known().value(root + "/gamma.avi").second, qint64(14));

        // vanished file removed
        QVERIFY(QFile::remove(root + "/sub/beta_clip.mkv"));
        scanAndWait(scanner, {root});
        QCOMPARE(repo.count(), 2);

        // model: search + sort + grouping
        model.reload();
        QCOMPARE(model.rowCount(), 2);
        model.setSearch("gam");
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.urls().size(), 1);
        model.setSearch({});
        model.setSortBy(0);
        QCOMPARE(model.data(model.index(0), LibraryModel::TitleRole).toString(), QString("Alpha Movie"));
        model.setGroupByFolder(true);
        QCOMPARE(model.rowCount(), 2);

        repo.removeFolder(root);
        QCOMPARE(repo.count(), 0);
    }

    void missingRootKeepsRows()
    {
        QTemporaryDir dir;
        touch(dir.path() + "/a.mp4");
        Database db(":memory:");
        LibraryRepository repo(&db);
        LibraryScanner scanner(&repo);
        scanAndWait(scanner, {dir.path()});
        QCOMPARE(repo.count(), 1);
        // an unreadable/unmounted root must not wipe its rows
        scanAndWait(scanner, {dir.path() + "_gone", dir.path()});
        QCOMPARE(repo.count(), 1);
        scanAndWait(scanner, {dir.path() + "_gone"});
        QCOMPARE(repo.count(), 1);
    }

    void emptyMountPointKeepsRows()
    {
        // An unmounted drive leaves its mount-point folder behind, empty: that must not read as "everything deleted".
        QTemporaryDir dir;
        touch(dir.path() + "/a.mp4");
        Database db(":memory:");
        LibraryRepository repo(&db);
        LibraryScanner scanner(&repo);
        scanAndWait(scanner, {dir.path()});
        QCOMPARE(repo.count(), 1);
        QVERIFY(QFile::remove(dir.path() + "/a.mp4"));
        scanAndWait(scanner, {dir.path()});
        QCOMPARE(repo.count(), 1); // kept (deleting a whole folder's files needs "remove folder")
    }

    void smallTsFilesAreNotMedia()
    {
        QTemporaryDir dir;
        touch(dir.path() + "/code.ts"); // TypeScript / Qt Linguist source
        touch(dir.path() + "/stream.ts", QByteArray(2 << 20, 'x')); // plausible transport stream
        Database db(":memory:");
        LibraryRepository repo(&db);
        LibraryScanner scanner(&repo);
        scanAndWait(scanner, {dir.path()});
        QCOMPARE(repo.count(), 1);
        QVERIFY(repo.known().contains(dir.path() + "/stream.ts"));
    }

    void cancelStopsCleanly()
    {
        QTemporaryDir dir;
        for (int i = 0; i < 300; ++i) touch(dir.path() + QString("/d%1/f%2.mp4").arg(i % 10).arg(i));
        Database db(":memory:");
        LibraryRepository repo(&db);
        LibraryScanner scanner(&repo);
        QSignalSpy done(&scanner, &LibraryScanner::finished);
        scanner.scan({dir.path()});
        scanner.cancel();
        QVERIFY(done.wait(10000));
        QVERIFY(!scanner.scanning());
    }

    void unavailableIsNoOp()
    {
        LibraryRepository none(nullptr);
        LibraryScanner scanner(&none);
        scanner.scan({"/tmp"}); // must not crash or start a thread
        QVERIFY(!scanner.scanning());
        QVERIFY(none.query({}, LibraryRepository::ByTitle).isEmpty());
    }
};

QTEST_MAIN(TstLibraryScanner)
#include "tst_LibraryScanner.moc"
