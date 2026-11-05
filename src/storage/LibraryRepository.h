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
};

// Optional media library table. Safe no-ops when the database is unavailable.
class LibraryRepository
{
public:
    enum Sort { ByTitle = 0, ByDate = 1, ByFolder = 2 };

    explicit LibraryRepository(Database *db) : m_db(db) {}
    bool available() const;

    void upsert(const QList<LibraryItem> &items); // keeps added_at of existing rows
    void remove(const QStringList &paths);
    void removeFolder(const QString &root);      // every row below root
    QHash<QString, QPair<qint64, qint64>> known() const; // path -> (mtime, size)
    // LIKE search on title/folder (empty = all).
    QList<LibraryItem> query(const QString &search, Sort sort, int limit = 5000) const;
    int count() const;

private:
    Database *m_db;
};
