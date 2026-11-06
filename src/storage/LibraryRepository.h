#pragma once

#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

class Database;

struct LibraryItem
{
    QString path, title, folder;
    qint64 size = 0, mtime = 0, addedAt = 0; // mtime/addedAt: epoch seconds
    double duration = -1;                    // -1 = unknown (optional, not probed yet)
    // Probed metadata (filled by LibraryProber; defaults until then).
    int kind = 0;                            // 0 video, 1 audio (extension first, refined by the probe)
    bool probed = false;
    int width = 0, height = 0;
    QString vcodec, artist, album, tagTitle, fhash;
    int track = 0, year = 0;
    bool hasCover = false;
    // Joined from history (read only).
    bool favorite = false;
    qint64 lastPlayed = 0;
    int playCount = 0;
};

enum class LibraryKind { Any, Video, Audio };

struct LibraryQuery
{
    enum Sort { ByTitle = 0, ByDate = 1, ByFolder = 2, ByDuration = 3, ByLastPlayed = 4 };
    QString search;
    Sort sort = ByTitle;
    LibraryKind kind = LibraryKind::Any;
    bool favorites = false;
    bool played = false;       // only files with history (last_played > 0)
    bool duplicates = false;   // only files that have a duplicate (fingerprint + size + duration)
    qint64 playlistId = 0;     // > 0: rows of that playlist, in playlist order (sort ignored)
    int minHeight = 0;         // 0 = any
    double minDuration = 0;    // seconds, 0 = any
    double maxDuration = 0;    // seconds, 0 = any
    int watched = 0;           // 0 any, 1 watched (played before), 2 unwatched
    int limit = 5000;
};

// Optional media library table. Safe no-ops when the database is unavailable.
class LibraryRepository
{
public:
    using Sort = LibraryQuery::Sort;
    static constexpr Sort ByTitle = LibraryQuery::ByTitle;
    static constexpr Sort ByDate = LibraryQuery::ByDate;
    static constexpr Sort ByFolder = LibraryQuery::ByFolder;

    explicit LibraryRepository(Database *db) : m_db(db) {}
    bool available() const;

    void upsert(const QList<LibraryItem> &items); // keeps added_at of existing rows; resets probed
    void remove(const QStringList &paths);
    void removeFolder(const QString &root);      // every row below root
    QHash<QString, QPair<qint64, qint64>> known() const; // path -> (mtime, size)
    void toggleFavorite(const QString &path, const QString &title);
    QStringList unprobed() const;               // paths still waiting for the metadata probe
    void updateMeta(const QList<LibraryItem> &items); // probe results (duration, tags, fingerprint...), marks probed
    // LIKE search on title/folder/tags (empty = all).
    QList<LibraryItem> query(const QString &search, Sort sort, int limit = 5000) const;
    QList<LibraryItem> query(const LibraryQuery &q) const;
    int count() const;

private:
    Database *m_db;
};
