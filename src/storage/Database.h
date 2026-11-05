#pragma once

#include <QSqlDatabase>
#include <QString>

// Optional SQLite store (AGENTS.md rule 8: playback must work without it).
// The constructor never throws; on failure valid() is false and callers no-op.
class Database
{
public:
    static constexpr int kSchemaVersion = 4;

    // path ":memory:" is allowed for tests. Empty path = default under AppDataLocation.
    explicit Database(const QString &path = {});
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool valid() const { return m_valid; }
    QSqlDatabase db() const { return QSqlDatabase::database(m_conn, false); }
    int schemaVersion() const;

private:
    bool migrate();

    QString m_conn;
    bool m_valid = false;
};
