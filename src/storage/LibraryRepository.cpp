#include "LibraryRepository.h"

#include "Database.h"

#include <QDateTime>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QVariant>

bool LibraryRepository::available() const { return m_db && m_db->valid(); }

static QString nn(const QString &s) { return s.isNull() ? QStringLiteral("") : s; } // NOT NULL columns
static QVariant nullable(double v) { return v >= 0 ? QVariant(v) : QVariant(QMetaType(QMetaType::Double)); }

void LibraryRepository::upsert(const QList<LibraryItem> &items)
{
    if (!available() || items.isEmpty()) return;
    QSqlDatabase d = m_db->db();
    d.transaction();
    QSqlQuery q(d);
    q.prepare(QStringLiteral(
        "INSERT INTO library(path,title,folder,size,mtime,added_at,duration,kind) VALUES(?,?,?,?,?,?,?,?) "
        "ON CONFLICT(path) DO UPDATE SET title=excluded.title, folder=excluded.folder, "
        "size=excluded.size, mtime=excluded.mtime, kind=excluded.kind, probed=0"));
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const LibraryItem &i : items) {
        q.addBindValue(i.path);
        q.addBindValue(i.title);
        q.addBindValue(i.folder);
        q.addBindValue(i.size);
        q.addBindValue(i.mtime);
        q.addBindValue(i.addedAt ? i.addedAt : now);
        q.addBindValue(nullable(i.duration));
        q.addBindValue(i.kind);
        q.exec();
    }
    d.commit();
}

void LibraryRepository::updateMeta(const QList<LibraryItem> &items)
{
    if (!available() || items.isEmpty()) return;
    QSqlDatabase d = m_db->db();
    d.transaction();
    QSqlQuery q(d);
    q.prepare(QStringLiteral(
        "UPDATE library SET duration=?, kind=?, width=?, height=?, vcodec=?, artist=?, album=?, tag_title=?,"
        " track=?, year=?, has_cover=?, fhash=?, probed=1 WHERE path=?"));
    for (const LibraryItem &i : items) {
        q.addBindValue(nullable(i.duration));
        q.addBindValue(i.kind);
        q.addBindValue(i.width);
        q.addBindValue(i.height);
        q.addBindValue(nn(i.vcodec));
        q.addBindValue(nn(i.artist));
        q.addBindValue(nn(i.album));
        q.addBindValue(nn(i.tagTitle));
        q.addBindValue(i.track);
        q.addBindValue(i.year);
        q.addBindValue(i.hasCover ? 1 : 0);
        q.addBindValue(nn(i.fhash));
        q.addBindValue(i.path);
        if (!q.exec()) qWarning() << q.lastError().text();
    }
    d.commit();
}

void LibraryRepository::remove(const QStringList &paths)
{
    if (!available() || paths.isEmpty()) return;
    QSqlDatabase d = m_db->db();
    d.transaction();
    QSqlQuery q(d);
    q.prepare(QStringLiteral("DELETE FROM library WHERE path=?"));
    for (const QString &p : paths) { q.addBindValue(p); q.exec(); }
    d.commit();
}

void LibraryRepository::removeFolder(const QString &root)
{
    if (!available()) return;
    QSqlQuery q(m_db->db());
    q.prepare(QStringLiteral("DELETE FROM library WHERE substr(path,1,?)=?"));
    const QString prefix = root.endsWith(u'/') ? root : root + u'/';
    q.addBindValue(int(prefix.size()));
    q.addBindValue(prefix);
    q.exec();
}

QHash<QString, QPair<qint64, qint64>> LibraryRepository::known() const
{
    QHash<QString, QPair<qint64, qint64>> out;
    if (!available()) return out;
    QSqlQuery q(m_db->db());
    if (q.exec(QStringLiteral("SELECT path,mtime,size FROM library")))
        while (q.next())
            out.insert(q.value(0).toString(), {q.value(1).toLongLong(), q.value(2).toLongLong()});
    return out;
}

void LibraryRepository::toggleFavorite(const QString &path, const QString &title)
{
    if (!available()) return;
    QSqlQuery q(m_db->db()); // creates a history row (never played) when the file has none yet
    q.prepare(QStringLiteral("INSERT INTO history(path,title,favorite) VALUES(?,?,1) "
                             "ON CONFLICT(path) DO UPDATE SET favorite=1-favorite"));
    q.addBindValue(path);
    q.addBindValue(title);
    q.exec();
}

QStringList LibraryRepository::unprobed() const
{
    QStringList out;
    if (!available()) return out;
    QSqlQuery q(m_db->db());
    if (q.exec(QStringLiteral("SELECT path FROM library WHERE probed=0 ORDER BY added_at DESC")))
        while (q.next()) out << q.value(0).toString();
    return out;
}

QList<LibraryItem> LibraryRepository::query(const QString &search, Sort sort, int limit) const
{
    LibraryQuery q;
    q.search = search;
    q.sort = sort;
    q.limit = limit;
    return query(q);
}

QList<LibraryItem> LibraryRepository::query(const LibraryQuery &lq) const
{
    QList<LibraryItem> out;
    if (!available()) return out;
    const bool pl = lq.playlistId > 0;
    // Playlist rows come from playlist_items so entries outside the library still play.
    QString sql = QStringLiteral(
        "SELECT %1, l.title, l.folder, l.size, l.mtime, l.added_at, l.duration, l.kind, l.probed, l.width,"
        " l.height, l.vcodec, l.artist, l.album, l.tag_title, l.track, l.year, l.has_cover, l.fhash,"
        " h.favorite, h.last_played, h.play_count FROM ").arg(pl ? QStringLiteral("pi.path") : QStringLiteral("l.path"));
    sql += pl ? QStringLiteral("playlist_items pi LEFT JOIN library l ON l.path=pi.path LEFT JOIN history h ON h.path=pi.path")
              : QStringLiteral("library l LEFT JOIN history h ON h.path=l.path");

    QStringList where;
    QVariantList binds;
    if (pl) { where << QStringLiteral("pi.playlist_id=?"); binds << lq.playlistId; }
    const QString term = lq.search.trimmed();
    if (!term.isEmpty()) {
        where << QStringLiteral("(l.title LIKE ? ESCAPE '\\' OR l.folder LIKE ? ESCAPE '\\' OR l.artist LIKE ? ESCAPE '\\'"
                                " OR l.album LIKE ? ESCAPE '\\' OR l.tag_title LIKE ? ESCAPE '\\')");
        QString e = term;
        e.replace(u'\\', QStringLiteral("\\\\")).replace(u'%', QStringLiteral("\\%")).replace(u'_', QStringLiteral("\\_"));
        for (int k = 0; k < 5; ++k) binds << (u'%' + e + u'%');
    }
    if (lq.kind == LibraryKind::Video) where << QStringLiteral("l.kind=0");
    if (lq.kind == LibraryKind::Audio) where << QStringLiteral("l.kind=1");
    if (lq.favorites) where << QStringLiteral("h.favorite=1");
    if (lq.played) where << QStringLiteral("h.last_played>0");
    if (lq.minHeight > 0) { where << QStringLiteral("l.height>=?"); binds << lq.minHeight; }
    if (lq.minDuration > 0) { where << QStringLiteral("l.duration>=?"); binds << lq.minDuration; }
    if (lq.maxDuration > 0) { where << QStringLiteral("l.duration>0 AND l.duration<?"); binds << lq.maxDuration; }
    if (lq.watched == 1) where << QStringLiteral("h.play_count>0");
    if (lq.watched == 2) where << QStringLiteral("(h.play_count IS NULL OR h.play_count=0)");
    if (lq.duplicates)
        where << QStringLiteral("l.fhash<>'' AND EXISTS(SELECT 1 FROM library d WHERE d.fhash=l.fhash AND d.path<>l.path"
                                " AND d.size=l.size AND CAST(ROUND(IFNULL(d.duration,0)) AS INTEGER)"
                                "=CAST(ROUND(IFNULL(l.duration,0)) AS INTEGER))");
    if (!where.isEmpty()) sql += QStringLiteral(" WHERE ") + where.join(QStringLiteral(" AND "));

    if (pl) sql += QStringLiteral(" ORDER BY pi.pos");
    else if (lq.duplicates) sql += QStringLiteral(" ORDER BY l.size DESC, l.fhash, l.path");
    else switch (lq.sort) {
    case LibraryQuery::ByDate:   sql += QStringLiteral(" ORDER BY l.added_at DESC, l.title COLLATE NOCASE"); break;
    case LibraryQuery::ByFolder: sql += QStringLiteral(" ORDER BY l.folder COLLATE NOCASE, l.title COLLATE NOCASE"); break;
    case LibraryQuery::ByDuration: sql += QStringLiteral(" ORDER BY IFNULL(l.duration,0) DESC, l.title COLLATE NOCASE"); break;
    case LibraryQuery::ByLastPlayed: sql += QStringLiteral(" ORDER BY IFNULL(h.last_played,0) DESC, l.title COLLATE NOCASE"); break;
    default:                     sql += QStringLiteral(" ORDER BY l.title COLLATE NOCASE"); break;
    }
    sql += QStringLiteral(" LIMIT ") + QString::number(qMax(1, lq.limit));

    QSqlQuery q(m_db->db());
    q.prepare(sql);
    for (const QVariant &b : binds) q.addBindValue(b);

    if (q.exec())
        while (q.next()) {
            LibraryItem i;
            int c = 0;
            i.path = q.value(c++).toString();
            i.title = q.value(c++).toString();
            i.folder = q.value(c++).toString();
            i.size = q.value(c++).toLongLong();
            i.mtime = q.value(c++).toLongLong();
            i.addedAt = q.value(c++).toLongLong();
            const QVariant dur = q.value(c++);
            i.duration = dur.isNull() ? -1 : dur.toDouble();
            i.kind = q.value(c++).toInt();
            i.probed = q.value(c++).toInt() != 0;
            i.width = q.value(c++).toInt();
            i.height = q.value(c++).toInt();
            i.vcodec = q.value(c++).toString();
            i.artist = q.value(c++).toString();
            i.album = q.value(c++).toString();
            i.tagTitle = q.value(c++).toString();
            i.track = q.value(c++).toInt();
            i.year = q.value(c++).toInt();
            i.hasCover = q.value(c++).toInt() != 0;
            i.fhash = q.value(c++).toString();
            i.favorite = q.value(c++).toInt() != 0;
            i.lastPlayed = q.value(c++).toLongLong();
            i.playCount = q.value(c++).toInt();
            if (i.title.isEmpty()) { // playlist entry outside the library
                const QFileInfo fi(i.path);
                i.title = fi.completeBaseName();
                i.folder = fi.absolutePath();
            }
            out.append(i);
        }
    return out;
}

int LibraryRepository::count() const
{
    if (!available()) return 0;
    QSqlQuery q(m_db->db());
    return q.exec(QStringLiteral("SELECT COUNT(*) FROM library")) && q.next() ? q.value(0).toInt() : 0;
}
