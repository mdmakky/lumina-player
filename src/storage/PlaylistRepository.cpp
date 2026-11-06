#include "PlaylistRepository.h"

#include "Database.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QVariant>

bool PlaylistRepository::available() const { return m_db && m_db->valid(); }

qint64 PlaylistRepository::create(const QString &name)
{
    const QString n = name.trimmed();
    if (!available() || n.isEmpty()) return 0;
    QSqlQuery q(m_db->db());
    q.prepare(QStringLiteral("INSERT INTO playlists(name, created_at) VALUES(?, ?)"));
    q.addBindValue(n);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    return q.exec() ? q.lastInsertId().toLongLong() : 0;
}

bool PlaylistRepository::rename(qint64 id, const QString &name)
{
    const QString n = name.trimmed();
    if (!available() || n.isEmpty()) return false;
    QSqlQuery q(m_db->db());
    q.prepare(QStringLiteral("UPDATE playlists SET name=? WHERE id=?"));
    q.addBindValue(n);
    q.addBindValue(id);
    return q.exec() && q.numRowsAffected() > 0;
}

void PlaylistRepository::remove(qint64 id)
{
    if (!available()) return;
    QSqlDatabase d = m_db->db();
    d.transaction();
    QSqlQuery q(d);
    for (const char *t : {"DELETE FROM playlist_items WHERE playlist_id=?", "DELETE FROM playlists WHERE id=?"}) {
        q.prepare(QLatin1String(t));
        q.addBindValue(id);
        q.exec();
    }
    d.commit();
}

QList<PlaylistInfo> PlaylistRepository::list() const
{
    QList<PlaylistInfo> out;
    if (!available()) return out;
    QSqlQuery q(m_db->db());
    if (q.exec(QStringLiteral("SELECT p.id, p.name, (SELECT COUNT(*) FROM playlist_items i WHERE i.playlist_id=p.id)"
                              " FROM playlists p ORDER BY p.name COLLATE NOCASE, p.id")))
        while (q.next()) out.append({q.value(0).toLongLong(), q.value(1).toString(), q.value(2).toInt()});
    return out;
}

QStringList PlaylistRepository::items(qint64 id) const
{
    QStringList out;
    if (!available()) return out;
    QSqlQuery q(m_db->db());
    q.prepare(QStringLiteral("SELECT path FROM playlist_items WHERE playlist_id=? ORDER BY pos"));
    q.addBindValue(id);
    if (q.exec())
        while (q.next()) out << q.value(0).toString();
    return out;
}

void PlaylistRepository::append(qint64 id, const QStringList &paths)
{
    if (!available() || paths.isEmpty()) return;
    QSqlDatabase d = m_db->db();
    d.transaction();
    QSqlQuery q(d);
    q.prepare(QStringLiteral("SELECT IFNULL(MAX(pos)+1, 0) FROM playlist_items WHERE playlist_id=?"));
    q.addBindValue(id);
    int pos = q.exec() && q.next() ? q.value(0).toInt() : 0;
    q.prepare(QStringLiteral("INSERT INTO playlist_items(playlist_id, pos, path) VALUES(?,?,?)"));
    for (const QString &p : paths) {
        q.addBindValue(id);
        q.addBindValue(pos++);
        q.addBindValue(p);
        q.exec();
    }
    d.commit();
}

void PlaylistRepository::removeItem(qint64 id, int pos)
{
    if (!available()) return;
    QSqlDatabase d = m_db->db();
    d.transaction();
    QSqlQuery q(d);
    q.prepare(QStringLiteral("DELETE FROM playlist_items WHERE playlist_id=? AND pos=?"));
    q.addBindValue(id);
    q.addBindValue(pos);
    q.exec();
    q.prepare(QStringLiteral("UPDATE playlist_items SET pos=pos-1 WHERE playlist_id=? AND pos>?"));
    q.addBindValue(id);
    q.addBindValue(pos);
    q.exec();
    d.commit();
}
