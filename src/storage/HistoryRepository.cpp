#include "HistoryRepository.h"

#include "Database.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QVariant>

bool HistoryRepository::available() const { return m_db && m_db->valid(); }

void HistoryRepository::recordPlay(const QString &path, const QString &title, double duration)
{
    if (!available()) return;
    QSqlQuery q(m_db->db());
    q.prepare(QStringLiteral(
        "INSERT INTO history(path,title,duration,last_played,play_count) VALUES(?,?,?,?,1) "
        "ON CONFLICT(path) DO UPDATE SET last_played=excluded.last_played, play_count=play_count+1, "
        "title=excluded.title, duration=MAX(duration, excluded.duration)"));
    q.addBindValue(path);
    q.addBindValue(title);
    q.addBindValue(duration);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    q.exec();
}

void HistoryRepository::updateProgress(const QString &path, double position, double duration)
{
    if (!available()) return;
    QSqlQuery q(m_db->db());
    q.prepare(QStringLiteral("UPDATE history SET position=?, duration=? WHERE path=?"));
    q.addBindValue(position);
    q.addBindValue(duration);
    q.addBindValue(path);
    q.exec();
}

void HistoryRepository::setFavorite(const QString &path, bool favorite)
{
    if (!available()) return;
    QSqlQuery q(m_db->db());
    q.prepare(QStringLiteral("UPDATE history SET favorite=? WHERE path=?"));
    q.addBindValue(favorite ? 1 : 0);
    q.addBindValue(path);
    q.exec();
}

void HistoryRepository::importEntry(const HistoryEntry &e)
{
    if (!available()) return;
    QSqlQuery q(m_db->db());
    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO history(path,title,duration,position,last_played,play_count,favorite) "
        "VALUES(?,?,?,?,?,?,?)"));
    q.addBindValue(e.path);
    q.addBindValue(e.title);
    q.addBindValue(e.duration);
    q.addBindValue(e.position);
    q.addBindValue(e.lastPlayed);
    q.addBindValue(e.playCount);
    q.addBindValue(e.favorite ? 1 : 0);
    q.exec();
}

static HistoryEntry fromQuery(const QSqlQuery &q)
{
    HistoryEntry e;
    e.path = q.value(0).toString();
    e.title = q.value(1).toString();
    e.duration = q.value(2).toDouble();
    e.position = q.value(3).toDouble();
    e.lastPlayed = q.value(4).toLongLong();
    e.playCount = q.value(5).toInt();
    e.favorite = q.value(6).toBool();
    return e;
}

static const char *kCols = "SELECT path,title,duration,position,last_played,play_count,favorite FROM history ";

HistoryEntry HistoryRepository::entry(const QString &path) const
{
    if (!available()) return {};
    QSqlQuery q(m_db->db());
    q.prepare(QLatin1String(kCols) + QStringLiteral("WHERE path=?"));
    q.addBindValue(path);
    return q.exec() && q.next() ? fromQuery(q) : HistoryEntry{};
}

QList<HistoryEntry> HistoryRepository::list(Filter filter, int limit) const
{
    QList<HistoryEntry> out;
    if (!available()) return out;
    const char *where = filter == Filter::Favorites ? "WHERE favorite=1 "
        : filter == Filter::Continue ? "WHERE position>5 AND (duration<=0 OR duration-position>10) " : "";
    QSqlQuery q(m_db->db());
    q.prepare(QLatin1String(kCols) + QLatin1String(where)
              + QStringLiteral("ORDER BY last_played DESC, rowid DESC LIMIT ?"));
    q.addBindValue(limit);
    if (q.exec())
        while (q.next())
            out << fromQuery(q);
    return out;
}

qint64 HistoryRepository::addBookmark(const QString &path, double time, const QString &label)
{
    if (!available()) return 0;
    QSqlQuery q(m_db->db());
    q.prepare(QStringLiteral("INSERT INTO bookmarks(path,time,label) VALUES(?,?,?)"));
    q.addBindValue(path);
    q.addBindValue(time);
    q.addBindValue(label.isNull() ? QStringLiteral("") : label); // NOT NULL column
    return q.exec() ? q.lastInsertId().toLongLong() : 0;
}

void HistoryRepository::removeBookmark(qint64 id)
{
    if (!available()) return;
    QSqlQuery q(m_db->db());
    q.prepare(QStringLiteral("DELETE FROM bookmarks WHERE id=?"));
    q.addBindValue(id);
    q.exec();
}

QList<Bookmark> HistoryRepository::bookmarks(const QString &path) const
{
    QList<Bookmark> out;
    if (!available()) return out;
    QSqlQuery q(m_db->db());
    q.prepare(QStringLiteral("SELECT id,time,label FROM bookmarks WHERE path=? ORDER BY time, id"));
    q.addBindValue(path);
    if (q.exec())
        while (q.next())
            out.append({q.value(0).toLongLong(), q.value(1).toDouble(), q.value(2).toString()});
    return out;
}
