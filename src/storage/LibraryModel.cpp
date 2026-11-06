#include "LibraryModel.h"

#include "media/LibraryMeta.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

QHash<int, QByteArray> LibraryModel::roleNames() const
{
    return {{PathRole, "path"}, {TitleRole, "title"}, {FolderRole, "folder"}, {FolderNameRole, "folderName"},
            {SizeRole, "size"}, {MtimeRole, "mtime"}, {AddedAtRole, "addedAt"}, {DurationRole, "duration"},
            {KindRole, "kind"}, {WidthRole, "videoWidth"}, {HeightRole, "videoHeight"}, {VcodecRole, "vcodec"},
            {ArtistRole, "artist"}, {AlbumRole, "album"}, {TrackRole, "track"}, {YearRole, "year"},
            {HasCoverRole, "hasCover"}, {FavoriteRole, "favorite"}, {LastPlayedRole, "lastPlayed"},
            {ProbedRole, "probed"}, {ThumbRole, "thumb"}};
}

QVariant LibraryModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_rows.size()) return {};
    const LibraryItem &i = m_rows.at(idx.row());
    switch (role) {
    case PathRole: return i.path;
    case TitleRole: return i.tagTitle.isEmpty() ? i.title : i.tagTitle;
    case FolderRole: return i.folder;
    case FolderNameRole: return QFileInfo(i.folder).fileName();
    case SizeRole: return i.size;
    case MtimeRole: return i.mtime;
    case AddedAtRole: return i.addedAt;
    case DurationRole: return i.duration;
    case KindRole: return i.kind;
    case WidthRole: return i.width;
    case HeightRole: return i.height;
    case VcodecRole: return i.vcodec;
    case ArtistRole: return i.artist;
    case AlbumRole: return i.album;
    case TrackRole: return i.track;
    case YearRole: return i.year;
    case HasCoverRole: return i.hasCover;
    case FavoriteRole: return i.favorite;
    case LastPlayedRole: return i.lastPlayed;
    case ProbedRole: return i.probed;
    case ThumbRole: return LibraryMeta::thumbUrl(i.path, i.kind, i.mtime);
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
    s = qBound(0, s, 4);
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
    emit groupedChanged();
    reload();
}

void LibraryModel::setTab(int t)
{
    t = qBound(0, t, int(Duplicates));
    if (t == m_tab) return;
    const bool g = grouped();
    m_tab = t;
    emit tabChanged();
    if (g != grouped()) emit groupedChanged();
    reload();
}

void LibraryModel::setMinHeight(int h)
{
    if (h == m_minHeight) return;
    m_minHeight = h;
    emit filtersChanged();
    reload();
}

void LibraryModel::setDurationBucket(int b)
{
    b = qBound(0, b, 3);
    if (b == m_durBucket) return;
    m_durBucket = b;
    emit filtersChanged();
    reload();
}

void LibraryModel::setWatched(int w)
{
    w = qBound(0, w, 2);
    if (w == m_watched) return;
    m_watched = w;
    emit filtersChanged();
    reload();
}

void LibraryModel::setPlaylistId(qint64 id)
{
    if (id == m_playlistId) return;
    m_playlistId = id;
    emit playlistIdChanged();
    reload();
}

LibraryQuery LibraryModel::currentQuery() const
{
    LibraryQuery q;
    q.search = m_search;
    q.sort = LibraryQuery::Sort(m_sort);
    q.minHeight = m_minHeight;
    q.watched = m_watched;
    if (m_durBucket == 1) q.maxDuration = 600;
    if (m_durBucket == 2) { q.minDuration = 600; q.maxDuration = 3600; }
    if (m_durBucket == 3) q.minDuration = 3600;
    // Grouping needs rows contiguous per folder, so it overrides the sort key.
    if (m_group || m_tab == Folders) q.sort = LibraryQuery::ByFolder;
    switch (m_tab) {
    case Videos: q.kind = LibraryKind::Video; break;
    case Music: q.kind = LibraryKind::Audio; break;
    case Favorites: q.favorites = true; break;
    case RecentlyAdded: q.sort = LibraryQuery::ByDate; q.limit = 100; break;
    case RecentlyPlayed: q.played = true; q.sort = LibraryQuery::ByLastPlayed; q.limit = 100; break;
    case Playlists: q.playlistId = m_playlistId; break;
    case Duplicates: q.duplicates = true; break;
    default: break;
    }
    return q;
}

void LibraryModel::reload()
{
    const int before = int(m_rows.size());
    QList<LibraryItem> rows;
    if (m_tab != Playlists || m_playlistId > 0) // no playlist chosen: nothing to list
        rows = m_repo->query(currentQuery());
    bool samePaths = rows.size() == m_rows.size();
    for (int i = 0; samePaths && i < rows.size(); ++i) samePaths = rows.at(i).path == m_rows.at(i).path;
    if (samePaths) { // e.g. probe results arriving: refresh in place so the view keeps its scroll position
        m_rows = std::move(rows);
        if (!m_rows.isEmpty()) emit dataChanged(index(0), index(int(m_rows.size()) - 1));
        return;
    }
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
    if (before != m_rows.size()) emit countChanged();
}

QVariantList LibraryModel::urls() const
{
    QVariantList out;
    out.reserve(m_rows.size());
    for (const LibraryItem &i : m_rows)
        out << (i.path.contains(QStringLiteral("://")) ? QUrl(i.path) : QUrl::fromLocalFile(i.path));
    return out;
}

QStringList LibraryModel::paths() const
{
    QStringList out;
    for (const LibraryItem &i : m_rows) out << i.path;
    return out;
}

void LibraryModel::removeFromLibrary(const QString &path)
{
    m_repo->remove({path});
    reload();
}

bool LibraryModel::deleteFromDisk(const QString &path)
{
    // Trash, never unlink: the caller shows the confirm dialog first.
    const bool ok = QFile::moveToTrash(path);
    if (ok) removeFromLibrary(path);
    return ok;
}

void LibraryModel::revealInFolder(const QString &path) const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
}

void LibraryModel::toggleFavorite(const QString &path)
{
    QString title = QFileInfo(path).completeBaseName();
    for (const LibraryItem &i : m_rows)
        if (i.path == path) { title = i.title; break; }
    m_repo->toggleFavorite(path, title);
    reload();
}
