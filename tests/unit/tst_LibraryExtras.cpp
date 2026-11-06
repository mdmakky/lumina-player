// Library extras: metadata helpers, duplicate detection, playlists, filters and the v4 -> v5 migration.
#include "media/LibraryMeta.h"
#include "storage/Database.h"
#include "storage/LibraryModel.h"
#include "storage/LibraryRepository.h"
#include "storage/PlaylistRepository.h"
#include "storage/PlaylistsModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

class TstLibraryExtras : public QObject
{
    Q_OBJECT
    static LibraryItem item(const QString &path, int kind = 0, double dur = 100, int h = 0, const QString &fh = {})
    {
        LibraryItem i;
        i.path = path;
        i.title = QFileInfo(path).completeBaseName();
        i.folder = QFileInfo(path).absolutePath();
        i.size = 1000;
        i.kind = kind;
        i.duration = dur;
        i.height = h;
        i.fhash = fh;
        return i;
    }
    static void put(LibraryRepository &r, const LibraryItem &i) { r.upsert({i}); r.updateMeta({i}); }
    static void write(const QString &path, const QByteArray &d)
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(d);
    }

private slots:
    void tagParsing()
    {
        const auto t = LibraryMeta::parseTags({{"TITLE", " Sunrise "}, {"Artist", "Aurora"}, {"album", "Dawn"},
                                               {"track", "3/12"}, {"date", "2019-05-01"}});
        QCOMPARE(t.title, QString("Sunrise"));
        QCOMPARE(t.artist, QString("Aurora"));
        QCOMPARE(t.album, QString("Dawn"));
        QCOMPARE(t.track, 3);
        QCOMPARE(t.year, 2019);
        QCOMPARE(LibraryMeta::parseTags({{"album_artist", "Band"}, {"YEAR", "1999"}}).artist, QString("Band"));
        QCOMPARE(LibraryMeta::parseYear("abcd"), 0);
        QCOMPARE(LibraryMeta::parseTrack("x"), 0);
        QCOMPARE(LibraryMeta::resolutionLabel(2160), QString("4K"));
        QCOMPARE(LibraryMeta::resolutionLabel(1080), QString("1080p"));
        QCOMPARE(LibraryMeta::resolutionLabel(480), QString("480p"));
        QVERIFY(LibraryMeta::resolutionLabel(0).isEmpty());
    }

    void fingerprintAndPrune()
    {
        QTemporaryDir dir;
        const QString a = dir.filePath("a.bin"), b = dir.filePath("b.bin"), c = dir.filePath("c.bin");
        QByteArray big(300 * 1024, 'x');
        write(a, big);
        write(b, big);
        big[big.size() - 1] = 'y'; // tail differs
        write(c, big);
        QVERIFY(!LibraryMeta::fingerprint(a).isEmpty());
        QCOMPARE(LibraryMeta::fingerprint(a), LibraryMeta::fingerprint(b));
        QVERIFY(LibraryMeta::fingerprint(a) != LibraryMeta::fingerprint(c));
        write(dir.filePath("tiny"), "hi");
        QVERIFY(!LibraryMeta::fingerprint(dir.filePath("tiny")).isEmpty());
        QVERIFY(LibraryMeta::fingerprint(dir.filePath("missing")).isEmpty());

        QCOMPARE(LibraryMeta::pruneDir(dir.path(), 2), 2); // 4 files -> keep 2
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 2);
    }

    void m3uRoundTrip()
    {
        const QStringList p = {"/m/a.mp4", "/m/b c.mkv"};
        QCOMPARE(LibraryMeta::parseM3u(LibraryMeta::toM3u(p), QDir("/x")), p);
        const QStringList r = LibraryMeta::parseM3u("#EXTM3U\n#EXTINF:1,x\nrel/f.mp4\nhttp://h/s.m3u8\n\n", QDir("/base"));
        QCOMPARE(r, (QStringList{"/base/rel/f.mp4", "http://h/s.m3u8"}));
    }

    void duplicateDetection()
    {
        Database db(":memory:");
        LibraryRepository repo(&db);
        put(repo, item("/m/a.mp4", 0, 100, 720, "H1"));
        put(repo, item("/n/a_copy.mp4", 0, 100.2, 720, "H1")); // same hash, same rounded duration
        put(repo, item("/m/other.mp4", 0, 100, 720, "H2"));
        put(repo, item("/m/diffdur.mp4", 0, 500, 720, "H1")); // same hash but different duration: not a duplicate
        put(repo, item("/m/nohash.mp4", 0, 100, 720, ""));
        LibraryQuery q;
        q.duplicates = true;
        const auto d = repo.query(q);
        QCOMPARE(d.size(), 2);
        QCOMPARE(QSet<QString>({d[0].path, d[1].path}), (QSet<QString>{"/m/a.mp4", "/n/a_copy.mp4"}));
    }

    void filtersAndTabs()
    {
        Database db(":memory:");
        LibraryRepository repo(&db);
        LibraryItem hd = item("/m/hd.mp4", 0, 7200, 1080);
        LibraryItem sd = item("/m/sd.mp4", 0, 300, 480);
        LibraryItem song = item("/m/song.mp3", 1, 200);
        song.artist = "Aurora";
        song.tagTitle = "Sunrise";
        put(repo, hd);
        put(repo, sd);
        put(repo, song);
        LibraryModel model(&repo);

        model.reload();
        QCOMPARE(model.rowCount(), 3);
        model.setTab(LibraryModel::Videos);
        QCOMPARE(model.rowCount(), 2);
        model.setTab(LibraryModel::Music);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), LibraryModel::TitleRole).toString(), QString("Sunrise")); // tag title wins
        model.setSearch("aurora"); // artist is searchable
        QCOMPARE(model.rowCount(), 1);
        model.setSearch({});
        model.setTab(LibraryModel::Videos);
        model.setMinHeight(1080);
        QCOMPARE(model.rowCount(), 1);
        model.setMinHeight(0);
        model.setDurationBucket(1); // < 10 min
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), LibraryModel::PathRole).toString(), QString("/m/sd.mp4"));
        model.setDurationBucket(3); // > 1 h
        QCOMPARE(model.data(model.index(0), LibraryModel::PathRole).toString(), QString("/m/hd.mp4"));
        model.setDurationBucket(0);

        // history-based: favorites, watched, recently played
        model.setTab(LibraryModel::All);
        model.toggleFavorite("/m/sd.mp4");
        model.setTab(LibraryModel::Favorites);
        QCOMPARE(model.rowCount(), 1);
        model.toggleFavorite("/m/sd.mp4"); // un-favorite
        QCOMPARE(model.rowCount(), 0);
        QSqlQuery q(db.db());
        QVERIFY(q.exec("INSERT INTO history(path,title,play_count,last_played) VALUES('/m/hd.mp4','hd',2,500)"));
        model.setTab(LibraryModel::RecentlyPlayed);
        QCOMPARE(model.rowCount(), 1);
        model.setTab(LibraryModel::Videos);
        model.setWatched(1);
        QCOMPARE(model.rowCount(), 1);
        model.setWatched(2);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), LibraryModel::PathRole).toString(), QString("/m/sd.mp4"));
        model.setWatched(0);
        model.setSortBy(3); // duration, longest first
        QCOMPARE(model.data(model.index(0), LibraryModel::PathRole).toString(), QString("/m/hd.mp4"));
        QVERIFY(model.grouped() == false);
        model.setTab(LibraryModel::Folders);
        QVERIFY(model.grouped());
    }

    void playlists()
    {
        Database db(":memory:");
        LibraryRepository lib(&db);
        PlaylistRepository repo(&db);
        PlaylistsModel pls(&repo);
        QCOMPARE(pls.create("  "), qint64(0)); // blank name rejected
        const qint64 id = pls.create("Road trip");
        QVERIFY(id > 0);
        QVERIFY(pls.rename(id, "Road trip 2"));
        put(lib, item("/m/a.mp4"));
        pls.addPaths(id, {"/m/b.mp4", "/m/a.mp4", "/elsewhere/c.mp4"});
        QCOMPARE(repo.items(id), (QStringList{"/m/b.mp4", "/m/a.mp4", "/elsewhere/c.mp4"}));
        QCOMPARE(pls.rowCount(), 1);
        QCOMPARE(pls.data(pls.index(0), PlaylistsModel::CountRole).toInt(), 3);

        LibraryModel model(&lib);
        model.setTab(LibraryModel::Playlists);
        QCOMPARE(model.rowCount(), 0); // none selected
        model.setPlaylistId(id);
        QCOMPARE(model.paths(), (QStringList{"/m/b.mp4", "/m/a.mp4", "/elsewhere/c.mp4"})); // order kept, non-library entries too
        QCOMPARE(model.data(model.index(0), LibraryModel::TitleRole).toString(), QString("b"));

        pls.removeItem(id, 0);
        QCOMPARE(repo.items(id), (QStringList{"/m/a.mp4", "/elsewhere/c.mp4"}));

        QTemporaryDir dir;
        const QUrl f = QUrl::fromLocalFile(dir.filePath("out.m3u"));
        QVERIFY(pls.exportM3u(id, f));
        const qint64 id2 = pls.importM3u(f);
        QVERIFY(id2 > 0 && id2 != id);
        QCOMPARE(repo.items(id2), repo.items(id));
        QCOMPARE(pls.rowCount(), 2);

        pls.remove(id);
        QVERIFY(repo.items(id).isEmpty());
        QCOMPARE(pls.rowCount(), 1);
    }

    void migrationFromV4()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("old.db");
        {
            QSqlDatabase d = QSqlDatabase::addDatabase("QSQLITE", "old_v4");
            d.setDatabaseName(path);
            QVERIFY(d.open());
            QSqlQuery q(d);
            for (const char *sql : {
                     "CREATE TABLE schema_version (version INTEGER NOT NULL)", "INSERT INTO schema_version VALUES (4)",
                     "CREATE TABLE history (path TEXT PRIMARY KEY, title TEXT NOT NULL, duration REAL NOT NULL DEFAULT 0,"
                     " position REAL NOT NULL DEFAULT 0, last_played INTEGER NOT NULL DEFAULT 0,"
                     " play_count INTEGER NOT NULL DEFAULT 0, favorite INTEGER NOT NULL DEFAULT 0)",
                     "CREATE TABLE library (path TEXT PRIMARY KEY, title TEXT NOT NULL, folder TEXT NOT NULL,"
                     " size INTEGER NOT NULL DEFAULT 0, mtime INTEGER NOT NULL DEFAULT 0,"
                     " added_at INTEGER NOT NULL DEFAULT 0, duration REAL)",
                     "INSERT INTO library(path,title,folder,size,mtime,added_at) VALUES('/m/a.mp4','a','/m',5,6,7)",
                     "INSERT INTO library(path,title,folder,size,mtime,added_at) VALUES('/m/s.MP3','s','/m',5,6,7)"})
                QVERIFY2(q.exec(sql), sql);
            d.close();
        }
        QSqlDatabase::removeDatabase("old_v4");

        Database db(path);
        QVERIFY(db.valid());
        QCOMPARE(db.schemaVersion(), Database::kSchemaVersion);
        LibraryRepository repo(&db);
        const auto rows = repo.query("", LibraryRepository::ByTitle);
        QCOMPARE(rows.size(), 2); // data preserved
        QCOMPARE(rows[0].size, qint64(5));
        QCOMPARE(rows[0].kind, 0);
        QCOMPARE(rows[1].kind, 1); // audio detected from the extension (case-insensitive)
        QCOMPARE(repo.unprobed().size(), 2);
        PlaylistRepository pl(&db);
        QVERIFY(pl.create("x") > 0);
    }

    void unavailableDatabaseIsHarmless()
    {
        Database db("/nonexistent-dir-lumina/x.db");
        QVERIFY(!db.valid());
        LibraryRepository repo(&db);
        PlaylistRepository pl(&db);
        PlaylistsModel pls(&pl);
        LibraryModel model(&repo);
        model.reload();
        pls.reload();
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(pls.create("x"), qint64(0));
        model.toggleFavorite("/a");
        model.removeFromLibrary("/a");
        QVERIFY(repo.unprobed().isEmpty());
        LibraryQuery q;
        q.duplicates = true;
        QVERIFY(repo.query(q).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstLibraryExtras)
#include "tst_LibraryExtras.moc"
