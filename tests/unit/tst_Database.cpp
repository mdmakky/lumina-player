#include "storage/Database.h"

#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

// Upgrade and failure paths of the library database (the first start of a new build runs these on real user data).
class TstDatabase : public QObject
{
    Q_OBJECT
    static void sql(const QString &path, const QStringList &statements)
    {
        const QString conn = QStringLiteral("tst_db_setup");
        {
            QSqlDatabase d = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
            d.setDatabaseName(path);
            QVERIFY(d.open());
            QSqlQuery q(d);
            for (const QString &s : statements)
                QVERIFY2(q.exec(s), qPrintable(s));
        }
        QSqlDatabase::removeDatabase(conn);
    }

private slots:
    void upgradesVersion2KeepingData()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("library.db");
        sql(path, {
            "CREATE TABLE schema_version (version INTEGER NOT NULL)", "INSERT INTO schema_version VALUES (2)",
            "CREATE TABLE history (path TEXT PRIMARY KEY, title TEXT NOT NULL, duration REAL NOT NULL DEFAULT 0,"
            " position REAL NOT NULL DEFAULT 0, last_played INTEGER NOT NULL DEFAULT 0,"
            " play_count INTEGER NOT NULL DEFAULT 0, favorite INTEGER NOT NULL DEFAULT 0)",
            "CREATE TABLE library (path TEXT PRIMARY KEY, title TEXT NOT NULL, folder TEXT NOT NULL,"
            " size INTEGER NOT NULL DEFAULT 0, mtime INTEGER NOT NULL DEFAULT 0, added_at INTEGER NOT NULL DEFAULT 0, duration REAL)",
            "INSERT INTO history(path,title,favorite) VALUES('/m/a.mkv','a',1)",
            "INSERT INTO library(path,title,folder) VALUES('/m/song.mp3','song','/m')",
            "INSERT INTO library(path,title,folder) VALUES('/m/film.mkv','film','/m')"});
        Database db(path);
        QVERIFY(db.valid());
        QCOMPARE(db.schemaVersion(), Database::kSchemaVersion);
        QVERIFY(db.brokenBackup().isEmpty());
        QSqlQuery q(db.db());
        QVERIFY(q.exec("SELECT favorite FROM history WHERE path='/m/a.mkv'") && q.next());
        QCOMPARE(q.value(0).toInt(), 1);
        QVERIFY(q.exec("SELECT kind FROM library WHERE path='/m/song.mp3'") && q.next());
        QCOMPARE(q.value(0).toInt(), 1); // audio detected for rows indexed before the column existed
        QVERIFY(q.exec("SELECT kind FROM library WHERE path='/m/film.mkv'") && q.next());
        QCOMPARE(q.value(0).toInt(), 0);
    }

    void damagedFileIsSetAsideAndRecreated()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("library.db");
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(4096, 'x')); // not a SQLite file
        f.close();
        Database db(path);
        QVERIFY(db.valid());
        QVERIFY(!db.brokenBackup().isEmpty());
        QVERIFY(QFile::exists(db.brokenBackup()));
        QCOMPARE(db.schemaVersion(), Database::kSchemaVersion);
    }

    void newerSchemaIsLeftUntouched()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("library.db");
        sql(path, {"CREATE TABLE schema_version (version INTEGER NOT NULL)", "INSERT INTO schema_version VALUES (99)"});
        {
            Database db(path);
            QVERIFY(!db.valid());
            QVERIFY(db.brokenBackup().isEmpty());
        }
        QVERIFY(QFile::exists(path));
        QCOMPARE(QDir(dir.path()).entryList({"*.broken-*"}).size(), 0);
    }
};

QTEST_GUILESS_MAIN(TstDatabase)
#include "tst_Database.moc"
