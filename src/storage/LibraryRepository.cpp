#include "LibraryRepository.h"

#include "Database.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QVariant>

bool LibraryRepository::available() const { return m_db && m_db->valid(); }

void LibraryRepository::upsert(const QList<LibraryItem> &items)
{
    if (!available() || items.isEmpty()) return;
    QSqlDatabase d = m_db->db();
    d.transaction();
    QSqlQuery q(d);
    q.prepare(QStringLiteral(
        "INSERT INTO library(path,title,folder,size,mtime,added_at,duration) VALUES(?,?,?,?,?,?,?) "
        "ON CONFLICT(path) DO UPDATE SET title=excluded.title, folder=excluded.folder, "
        "size=excluded.size, mtime=excluded.mtime"));
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const LibraryItem &i : items) {
        q.addBindValue(i.path);
        q.addBindValue(i.title);
        q.addBindValue(i.folder);
        q.addBindValue(i.size);
        q.addBindValue(i.mtime);
        q.addBindValue(i.addedAt ? i.addedAt : now);
        q.addBindValue(i.duration >= 0 ? QVariant(i.duration) : QVariant(QMetaType(QMetaType::Double)));
        q.exec();
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

QList<LibraryItem> LibraryRepository::query(const QString &search, Sort sort, int limit) const
{
    QList<LibraryItem> out;
    if (!available()) return out;
    QString sql = QStringLiteral("SELECT path,title,folder,size,mtime,added_at,duration FROM library");
    const QString term = search.trimmed();
    if (!term.isEmpty())
        sql += QStringLiteral(" WHERE title LIKE ? ESCAPE '\\' OR folder LIKE ? ESCAPE '\\'");
    switch (sort) {
    case ByDate:   sql += QStringLiteral(" ORDER BY added_at DESC, title COLLATE NOCASE"); break;
    case ByFolder: sql += QStringLiteral(" ORDER BY folder COLLATE NOCASE, title COLLATE NOCASE"); break;
    default:       sql += QStringLiteral(" ORDER BY title COLLATE NOCASE"); break;
    }
    sql += QStringLiteral(" LIMIT ?");
    QSqlQuery q(m_db->db());
    q.prepare(sql);
    if (!term.isEmpty()) {
        QString e = term;
        e.replace(u'\\', QStringLiteral("\\\\")).replace(u'%', QStringLiteral("\\%")).replace(u'_', QStringLiteral("\\_"));
        q.addBindValue(u'%' + e + u'%');
        q.addBindValue(u'%' + e + u'%');
    }
    q.addBindValue(limit);
    if (q.exec())
        while (q.next()) {
            LibraryItem i;
            i.path = q.value(0).toString();
            i.title = q.value(1).toString();
            i.folder = q.value(2).toString();
            i.size = q.value(3).toLongLong();
            i.mtime = q.value(4).toLongLong();
            i.addedAt = q.value(5).toLongLong();
            i.duration = q.value(6).isNull() ? -1 : q.value(6).toDouble();
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
