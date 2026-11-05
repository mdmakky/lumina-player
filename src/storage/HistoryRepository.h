#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>

class Database;

struct HistoryEntry
{
    QString path, title;
    double duration = 0, position = 0;
    qint64 lastPlayed = 0; // epoch seconds
    int playCount = 0;
    bool favorite = false;
};

struct Bookmark
{
    qint64 id = 0;
    double time = 0;
    QString label;
};

// All methods are safe no-ops (empty/false results) when the database is unavailable.
class HistoryRepository
{
public:
    enum class Filter { All, Continue, Favorites };

    explicit HistoryRepository(Database *db) : m_db(db) {}
    bool available() const;

    // Starting playback: insert or bump play_count/last_played.
    void recordPlay(const QString &path, const QString &title, double duration = 0);
    // position 0 clears resume (finished). Only touches an existing row.
    void updateProgress(const QString &path, double position, double duration);
    void setTitle(const QString &path, const QString &title); // only touches an existing row
    void setFavorite(const QString &path, bool favorite);
    // Legacy QSettings import; never overwrites an existing row.
    void importEntry(const HistoryEntry &e);

    HistoryEntry entry(const QString &path) const; // empty path field if unknown
    // Continue = started (>5 s) and not within 10 s of the end.
    QList<HistoryEntry> list(Filter filter, int limit = 50) const;

    // Per-file bookmarks, ordered by time. addBookmark returns the new id (0 if unavailable).
    qint64 addBookmark(const QString &path, double time, const QString &label = {});
    void removeBookmark(qint64 id);
    QList<Bookmark> bookmarks(const QString &path) const;

    // Per-file playback profile (video adjust, tracks, delays, ...) stored as JSON.
    void saveProfile(const QString &path, const QVariantMap &profile);
    QVariantMap profile(const QString &path) const; // empty if none / unavailable
    void clearProfile(const QString &path);

private:
    Database *m_db;
};
