#pragma once

#include "storage/LibraryRepository.h"

#include <QAbstractListModel>
#include <QVariantList>

// Searchable, sortable, filterable view of the library table; call reload() after writes.
// `tab` selects the section (All, Videos, Music, Favorites, Recently added, Recently played,
// Folders, Playlists, Duplicates); the filters and sort apply on top of it.
class LibraryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged FINAL)
    // 0 title, 1 date added, 2 folder, 3 duration, 4 last played
    Q_PROPERTY(int sortBy READ sortBy WRITE setSortBy NOTIFY sortByChanged FINAL)
    Q_PROPERTY(bool groupByFolder READ groupByFolder WRITE setGroupByFolder NOTIFY groupByFolderChanged FINAL)
    Q_PROPERTY(int tab READ tab WRITE setTab NOTIFY tabChanged FINAL)
    Q_PROPERTY(bool grouped READ grouped NOTIFY groupedChanged FINAL) // rows come grouped by folder
    Q_PROPERTY(int minHeight READ minHeight WRITE setMinHeight NOTIFY filtersChanged FINAL)           // 0 any, 720, 1080, 2160
    Q_PROPERTY(int durationBucket READ durationBucket WRITE setDurationBucket NOTIFY filtersChanged FINAL) // 0 any, 1 <10 min, 2 10-60, 3 >60
    Q_PROPERTY(int watched READ watched WRITE setWatched NOTIFY filtersChanged FINAL)                 // 0 any, 1 watched, 2 unwatched
    Q_PROPERTY(qint64 playlistId READ playlistId WRITE setPlaylistId NOTIFY playlistIdChanged FINAL)
public:
    enum Tab { All, Videos, Music, Favorites, RecentlyAdded, RecentlyPlayed, Folders, Playlists, Duplicates };
    Q_ENUM(Tab)
    enum Role { PathRole = Qt::UserRole + 1, TitleRole, FolderRole, FolderNameRole, SizeRole, MtimeRole,
                AddedAtRole, DurationRole, KindRole, WidthRole, HeightRole, VcodecRole, ArtistRole, AlbumRole,
                TrackRole, YearRole, HasCoverRole, FavoriteRole, LastPlayedRole, ProbedRole, ThumbRole };

    explicit LibraryModel(LibraryRepository *repo, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_repo(repo) {}

    int rowCount(const QModelIndex &p = {}) const override { return p.isValid() ? 0 : int(m_rows.size()); }
    QVariant data(const QModelIndex &i, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString search() const { return m_search; }
    void setSearch(const QString &s);
    int sortBy() const { return m_sort; }
    void setSortBy(int s);
    bool groupByFolder() const { return m_group; }
    void setGroupByFolder(bool g);
    int tab() const { return m_tab; }
    void setTab(int t);
    bool grouped() const { return m_group || m_tab == Folders || m_tab == Duplicates; }
    int minHeight() const { return m_minHeight; }
    void setMinHeight(int h);
    int durationBucket() const { return m_durBucket; }
    void setDurationBucket(int b);
    int watched() const { return m_watched; }
    void setWatched(int w);
    qint64 playlistId() const { return m_playlistId; }
    void setPlaylistId(qint64 id);

    Q_INVOKABLE QVariantList urls() const; // every visible row, in order, as file URLs
    Q_INVOKABLE QStringList paths() const;
    // Row actions. Removing only drops the library entry; deleting moves the file to the trash.
    Q_INVOKABLE void removeFromLibrary(const QString &path);
    Q_INVOKABLE bool deleteFromDisk(const QString &path);
    Q_INVOKABLE void revealInFolder(const QString &path) const;
    Q_INVOKABLE void toggleFavorite(const QString &path);
    Q_INVOKABLE void reload();

    LibraryQuery currentQuery() const;

signals:
    void countChanged();
    void searchChanged();
    void sortByChanged();
    void groupByFolderChanged();
    void groupedChanged();
    void tabChanged();
    void filtersChanged();
    void playlistIdChanged();

private:
    LibraryRepository *m_repo;
    QString m_search;
    int m_sort = 0;
    bool m_group = false;
    int m_tab = All;
    int m_minHeight = 0, m_durBucket = 0, m_watched = 0;
    qint64 m_playlistId = 0;
    QList<LibraryItem> m_rows;
};
