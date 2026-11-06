#pragma once

#include <QList>
#include <QString>
#include <QStringList>

class Database;

struct PlaylistInfo
{
    qint64 id = 0;
    QString name;
    int count = 0;
};

// Saved playlists (tables playlists / playlist_items). Safe no-ops when the DB is unavailable.
class PlaylistRepository
{
public:
    explicit PlaylistRepository(Database *db) : m_db(db) {}
    bool available() const;

    qint64 create(const QString &name); // 0 on failure / empty name
    bool rename(qint64 id, const QString &name);
    void remove(qint64 id);
    QList<PlaylistInfo> list() const;   // by name
    QStringList items(qint64 id) const; // in order
    void append(qint64 id, const QStringList &paths);
    void removeItem(qint64 id, int pos); // pos = 0-based index within the playlist; later items shift up

private:
    Database *m_db;
};
