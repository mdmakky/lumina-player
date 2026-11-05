#pragma once

#include <QList>
#include <QString>

class Database;

struct HistoryEntry
{
    QString path, title;
    double duration = 0, position = 0;
    qint64 lastPlayed = 0; // epoch seconds
    int playCount = 0;
    bool favorite = false;
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
    void setFavorite(const QString &path, bool favorite);
    // Legacy QSettings import; never overwrites an existing row.
    void importEntry(const HistoryEntry &e);

    HistoryEntry entry(const QString &path) const; // empty path field if unknown
    // Continue = started (>5 s) and not within 10 s of the end.
    QList<HistoryEntry> list(Filter filter, int limit = 50) const;

private:
    Database *m_db;
};
