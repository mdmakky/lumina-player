#pragma once

#include "storage/LibraryRepository.h"

#include <QAbstractListModel>
#include <QVariantList>

// Searchable, sortable view of the library table; call reload() after writes.
class LibraryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged FINAL)
    Q_PROPERTY(int sortBy READ sortBy WRITE setSortBy NOTIFY sortByChanged FINAL) // 0 title, 1 date added, 2 folder
    Q_PROPERTY(bool groupByFolder READ groupByFolder WRITE setGroupByFolder NOTIFY groupByFolderChanged FINAL)
public:
    enum Role { PathRole = Qt::UserRole + 1, TitleRole, FolderRole, FolderNameRole, SizeRole, MtimeRole,
                AddedAtRole, DurationRole };

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

    Q_INVOKABLE QVariantList urls() const; // every visible row, in order, as file URLs
    void reload();

signals:
    void countChanged();
    void searchChanged();
    void sortByChanged();
    void groupByFolderChanged();

private:
    LibraryRepository *m_repo;
    QString m_search;
    int m_sort = 0;
    bool m_group = false;
    QList<LibraryItem> m_rows;
};
