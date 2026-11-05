#include "Database.h"

#include "utilities/Logger.h"

#include <QDir>
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
    {
        QSqlDatabase d = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_conn);
        d.setDatabaseName(path);
        if (!d.open()) {
            qCWarning(lcCore) << "Library disabled:" << d.lastError().text();
        } else {
            QSqlQuery q(d);
            q.exec(QStringLiteral("PRAGMA journal_mode=WAL")); // stays "memory" for :memory:
            q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
            m_valid = migrate();
        }
    }
    if (!m_valid)
        QSqlDatabase::removeDatabase(m_conn);
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
    return true;
}
