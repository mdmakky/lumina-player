#pragma once

#include "storage/HistoryRepository.h"

#include <QAbstractListModel>

// One filtered view over the repository; call reload() after any write.
class HistoryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
public:
    enum Role { PathRole = Qt::UserRole + 1, TitleRole, DurationRole, PositionRole, ProgressRole,
                LastPlayedRole, PlayCountRole, FavoriteRole };

    HistoryModel(HistoryRepository *repo, HistoryRepository::Filter filter, int limit, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_repo(repo), m_filter(filter), m_limit(limit) {}

    int rowCount(const QModelIndex &p = {}) const override { return p.isValid() ? 0 : int(m_rows.size()); }
    QVariant data(const QModelIndex &i, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void reload();

signals:
    void countChanged();

private:
    HistoryRepository *m_repo;
    HistoryRepository::Filter m_filter;
    int m_limit;
    QList<HistoryEntry> m_rows;
};
