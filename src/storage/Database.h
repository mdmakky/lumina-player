#pragma once

#include <QSqlDatabase>
#include <QString>

// Optional SQLite store (AGENTS.md rule 8: playback must work without it).
// The constructor never throws; on failure valid() is false and callers no-op.
class Database
{
public:
    static constexpr int kSchemaVersion = 5;

    // path ":memory:" is allowed for tests. Empty path = default under AppDataLocation.
    explicit Database(const QString &path = {});
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool valid() const { return m_valid; }
    QSqlDatabase db() const { return QSqlDatabase::database(m_conn, false); }
    int schemaVersion() const;
    // Set when an unusable file was moved aside and a fresh database created ("" otherwise).
    QString brokenBackup() const { return m_brokenBackup; }

private:
    bool migrate();
    bool open(const QString &path);

    QString m_conn;
    QString m_brokenBackup;
    bool m_valid = false;
    bool m_tooNew = false; // file written by a newer build: never touched
};
