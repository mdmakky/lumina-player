#include "PlaylistsModel.h"

#include "media/LibraryMeta.h"

#include <QFile>
#include <QFileInfo>

QHash<int, QByteArray> PlaylistsModel::roleNames() const
{
    return {{IdRole, "playlistId"}, {NameRole, "name"}, {CountRole, "itemCount"}};
}

QVariant PlaylistsModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_rows.size()) return {};
    const PlaylistInfo &p = m_rows.at(idx.row());
    switch (role) {
    case IdRole: return p.id;
    case NameRole: return p.name;
    case CountRole: return p.count;
    }
    return {};
}

void PlaylistsModel::reload()
{
    const int before = int(m_rows.size());
    beginResetModel();
    m_rows = m_repo->list();
    endResetModel();
    if (before != m_rows.size()) emit countChanged();
}

qint64 PlaylistsModel::create(const QString &name)
{
    const qint64 id = m_repo->create(name);
    reload();
    return id;
}

bool PlaylistsModel::rename(qint64 id, const QString &name)
{
    const bool ok = m_repo->rename(id, name);
    reload();
    return ok;
}

void PlaylistsModel::remove(qint64 id)
{
    m_repo->remove(id);
    reload();
    emit contentChanged(id);
}

void PlaylistsModel::addPaths(qint64 id, const QStringList &paths)
{
    m_repo->append(id, paths);
    reload();
    emit contentChanged(id);
}

void PlaylistsModel::removeItem(qint64 id, int pos)
{
    m_repo->removeItem(id, pos);
    reload();
    emit contentChanged(id);
}

bool PlaylistsModel::exportM3u(qint64 id, const QUrl &file) const
{
    QFile f(file.toLocalFile());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return f.write(LibraryMeta::toM3u(m_repo->items(id)).toUtf8()) > 0;
}

qint64 PlaylistsModel::importM3u(const QUrl &file)
{
    QFile f(file.toLocalFile());
    if (!f.open(QIODevice::ReadOnly)) return 0;
    const QStringList paths = LibraryMeta::parseM3u(QString::fromUtf8(f.readAll()), QFileInfo(f).absoluteDir());
    const qint64 id = m_repo->create(QFileInfo(f).completeBaseName());
    if (id <= 0) return 0;
    m_repo->append(id, paths);
    reload();
    emit contentChanged(id);
    return id;
}
