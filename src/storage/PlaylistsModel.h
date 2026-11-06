#pragma once

#include "storage/PlaylistRepository.h"

#include <QAbstractListModel>
#include <QUrl>

// Saved playlists for the library's Playlists tab. Content changes emit contentChanged(id)
// so the library list can reload.
class PlaylistsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
public:
    enum Role { IdRole = Qt::UserRole + 1, NameRole, CountRole };

    explicit PlaylistsModel(PlaylistRepository *repo, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_repo(repo) {}

    int rowCount(const QModelIndex &p = {}) const override { return p.isValid() ? 0 : int(m_rows.size()); }
    QVariant data(const QModelIndex &i, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE qint64 create(const QString &name);
    Q_INVOKABLE bool rename(qint64 id, const QString &name);
    Q_INVOKABLE void remove(qint64 id);
    Q_INVOKABLE void addPaths(qint64 id, const QStringList &paths);
    Q_INVOKABLE void removeItem(qint64 id, int pos);
    Q_INVOKABLE QString nameOf(qint64 id) const
    {
        for (const PlaylistInfo &p : m_rows) if (p.id == id) return p.name;
        return {};
    }
    Q_INVOKABLE QStringList paths(qint64 id) const { return m_repo->items(id); }
    Q_INVOKABLE bool exportM3u(qint64 id, const QUrl &file) const;
    Q_INVOKABLE qint64 importM3u(const QUrl &file); // new playlist named after the file; 0 on failure
    void reload();

signals:
    void countChanged();
    void contentChanged(qint64 id);

private:
    PlaylistRepository *m_repo;
    QList<PlaylistInfo> m_rows;
};
