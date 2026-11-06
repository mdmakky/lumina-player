#include "Database.h"

#include "utilities/Logger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

Database::Database(const QString &pathIn)
    : m_conn(QUuid::createUuid().toString())
{
    QString path = pathIn;
    if (path.isEmpty()) {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (dir.isEmpty() || !QDir().mkpath(dir)) {
            qCWarning(lcCore) << "Library disabled: no writable data directory";
            return;
        }
        path = dir + QStringLiteral("/library.db");
    }
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        qCWarning(lcCore) << "Library disabled: QSQLITE driver missing";
        return;
    }
    m_valid = open(path);
    if (!m_valid && !m_tooNew && path != QLatin1String(":memory:") && QFileInfo::exists(path)) {
        // Damaged file: keep it for inspection and start over, so history/favorites work again.
        const QString backup = path + QStringLiteral(".broken-") + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
        if (QFile::rename(path, backup)) {
            for (const char *ext : {"-wal", "-shm"})
                QFile::remove(path + QLatin1String(ext));
            m_valid = open(path);
            if (m_valid) {
                m_brokenBackup = backup;
                qCWarning(lcCore) << "Library database was unusable; saved as" << backup << "and recreated";
            }
        }
    }
}

bool Database::open(const QString &path)
{
    bool ok = false;
    {
        QSqlDatabase d = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_conn);
        d.setDatabaseName(path);
        if (!d.open()) {
            qCWarning(lcCore) << "Library disabled:" << d.lastError().text();
        } else {
            QSqlQuery q(d);
            q.exec(QStringLiteral("PRAGMA journal_mode=WAL")); // stays "memory" for :memory:
            q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
            ok = migrate();
        }
    }
    if (!ok)
        QSqlDatabase::removeDatabase(m_conn);
    return ok;
}

Database::~Database()
{
    if (m_valid) {
        { QSqlDatabase d = db(); d.close(); }
        QSqlDatabase::removeDatabase(m_conn);
    }
}

int Database::schemaVersion() const
{
    QSqlQuery q(db());
    return q.exec(QStringLiteral("SELECT version FROM schema_version")) && q.next() ? q.value(0).toInt() : 0;
}

bool Database::migrate()
{
    QSqlDatabase d = db();
    QSqlQuery q(d);
    if (!q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS schema_version (version INTEGER NOT NULL)")))
        return false;
    const int v = schemaVersion();
    if (v > kSchemaVersion) { // written by a newer build; don't touch
        qCWarning(lcCore) << "Library disabled: schema" << v << "is newer than" << kSchemaVersion;
        m_tooNew = true;
        return false;
    }
    if (v < 1) {
        d.transaction();
        const bool ok = q.exec(QStringLiteral(
            "CREATE TABLE history ("
            " path TEXT PRIMARY KEY, title TEXT NOT NULL, duration REAL NOT NULL DEFAULT 0,"
            " position REAL NOT NULL DEFAULT 0, last_played INTEGER NOT NULL DEFAULT 0,"
            " play_count INTEGER NOT NULL DEFAULT 0, favorite INTEGER NOT NULL DEFAULT 0)"))
            && q.exec(QStringLiteral("CREATE INDEX history_last_played ON history(last_played DESC)"))
            && q.exec(QStringLiteral("DELETE FROM schema_version"))
            && q.exec(QStringLiteral("INSERT INTO schema_version VALUES (1)"));
        if (!ok) {
            qCWarning(lcCore) << "Library migration failed:" << q.lastError().text();
            d.rollback();
            return false;
        }
        d.commit();
    }
    if (v < 2) {
        d.transaction();
        const bool ok = q.exec(QStringLiteral(
            "CREATE TABLE library ("
            " path TEXT PRIMARY KEY, title TEXT NOT NULL, folder TEXT NOT NULL,"
            " size INTEGER NOT NULL DEFAULT 0, mtime INTEGER NOT NULL DEFAULT 0,"
            " added_at INTEGER NOT NULL DEFAULT 0, duration REAL)"))
            && q.exec(QStringLiteral("CREATE INDEX library_folder ON library(folder)"))
            && q.exec(QStringLiteral("DELETE FROM schema_version"))
            && q.exec(QStringLiteral("INSERT INTO schema_version VALUES (2)"));
        if (!ok) {
            qCWarning(lcCore) << "Library migration failed:" << q.lastError().text();
            d.rollback();
            return false;
        }
        d.commit();
    }
    if (v < 3) {
        d.transaction();
        const bool ok = q.exec(QStringLiteral(
            "CREATE TABLE bookmarks (id INTEGER PRIMARY KEY, path TEXT NOT NULL,"
            " time REAL NOT NULL, label TEXT NOT NULL DEFAULT '')"))
            && q.exec(QStringLiteral("CREATE INDEX bookmarks_path ON bookmarks(path, time)"))
            && q.exec(QStringLiteral("DELETE FROM schema_version"))
            && q.exec(QStringLiteral("INSERT INTO schema_version VALUES (3)"));
        if (!ok) {
            qCWarning(lcCore) << "Library migration failed:" << q.lastError().text();
            d.rollback();
            return false;
        }
        d.commit();
    }
    if (v < 4) {
        d.transaction();
        const bool ok = q.exec(QStringLiteral(
            "CREATE TABLE file_profile (path TEXT PRIMARY KEY, data TEXT NOT NULL)"))
            && q.exec(QStringLiteral("DELETE FROM schema_version"))
            && q.exec(QStringLiteral("INSERT INTO schema_version VALUES (4)"));
        if (!ok) {
            qCWarning(lcCore) << "Library migration failed:" << q.lastError().text();
            d.rollback();
            return false;
        }
        d.commit();
    }
    if (v < 5) { // library metadata, duplicate fingerprints, saved playlists (all additive)
        d.transaction();
        bool ok = true;
        for (const char *col : {"kind INTEGER NOT NULL DEFAULT 0", "probed INTEGER NOT NULL DEFAULT 0",
                                "width INTEGER NOT NULL DEFAULT 0", "height INTEGER NOT NULL DEFAULT 0",
                                "vcodec TEXT NOT NULL DEFAULT ''", "artist TEXT NOT NULL DEFAULT ''",
                                "album TEXT NOT NULL DEFAULT ''", "tag_title TEXT NOT NULL DEFAULT ''",
                                "track INTEGER NOT NULL DEFAULT 0", "year INTEGER NOT NULL DEFAULT 0",
                                "has_cover INTEGER NOT NULL DEFAULT 0", "fhash TEXT NOT NULL DEFAULT ''"})
            ok = ok && q.exec(QStringLiteral("ALTER TABLE library ADD COLUMN ") + QLatin1String(col));
        ok = ok
            && q.exec(QStringLiteral("UPDATE library SET kind=1 WHERE " // rows indexed before audio detection existed
                "path LIKE '%.mp3' OR path LIKE '%.flac' OR path LIKE '%.wav' OR path LIKE '%.ogg' OR path LIKE '%.oga'"
                " OR path LIKE '%.opus' OR path LIKE '%.m4a' OR path LIKE '%.aac'"))
            && q.exec(QStringLiteral("CREATE INDEX library_fhash ON library(fhash)"))
            && q.exec(QStringLiteral("CREATE TABLE playlists (id INTEGER PRIMARY KEY, name TEXT NOT NULL,"
                " created_at INTEGER NOT NULL DEFAULT 0)"))
            && q.exec(QStringLiteral("CREATE TABLE playlist_items (playlist_id INTEGER NOT NULL, pos INTEGER NOT NULL,"
                " path TEXT NOT NULL, PRIMARY KEY(playlist_id, pos))"))
            && q.exec(QStringLiteral("DELETE FROM schema_version"))
            && q.exec(QStringLiteral("INSERT INTO schema_version VALUES (5)"));
        if (!ok) {
            qCWarning(lcCore) << "Library migration failed:" << q.lastError().text();
            d.rollback();
            return false;
        }
        d.commit();
    }
    return true;
}
