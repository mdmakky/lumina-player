#include "LibraryModel.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

QHash<int, QByteArray> LibraryModel::roleNames() const
{
    return {{PathRole, "path"}, {TitleRole, "title"}, {FolderRole, "folder"}, {FolderNameRole, "folderName"},
            {SizeRole, "size"}, {MtimeRole, "mtime"}, {AddedAtRole, "addedAt"}, {DurationRole, "duration"}};
}

QVariant LibraryModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_rows.size()) return {};
    const LibraryItem &i = m_rows.at(idx.row());
    switch (role) {
    case PathRole: return i.path;
    case TitleRole: return i.title;
    case FolderRole: return i.folder;
    case FolderNameRole: return QFileInfo(i.folder).fileName();
    case SizeRole: return i.size;
    case MtimeRole: return i.mtime;
    case AddedAtRole: return i.addedAt;
    case DurationRole: return i.duration;
    }
    return {};
}

void LibraryModel::setSearch(const QString &s)
{
    if (s == m_search) return;
    m_search = s;
    emit searchChanged();
    reload();
}

void LibraryModel::setSortBy(int s)
{
    s = qBound(0, s, 2);
    if (s == m_sort) return;
    m_sort = s;
    emit sortByChanged();
    reload();
}

void LibraryModel::setGroupByFolder(bool g)
{
    if (g == m_group) return;
    m_group = g;
    emit groupByFolderChanged();
    reload();
}

void LibraryModel::reload()
{
    const int before = int(m_rows.size());
    // Grouping needs rows contiguous per folder, so it overrides the sort key.
    auto rows = m_repo->query(m_search, m_group ? LibraryRepository::ByFolder
                                                : LibraryRepository::Sort(m_sort));
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
    if (before != m_rows.size()) emit countChanged();
}

QVariantList LibraryModel::urls() const
{
    QVariantList out;
    out.reserve(m_rows.size());
    for (const LibraryItem &i : m_rows) out << QUrl::fromLocalFile(i.path);
    return out;
}
