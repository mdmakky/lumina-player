#include "HistoryModel.h"

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    return {{PathRole, "path"}, {TitleRole, "title"}, {DurationRole, "duration"}, {PositionRole, "position"},
            {ProgressRole, "progress"}, {LastPlayedRole, "lastPlayed"}, {PlayCountRole, "playCount"},
            {FavoriteRole, "favorite"}};
}

QVariant HistoryModel::data(const QModelIndex &i, int role) const
{
    if (!i.isValid() || i.row() >= m_rows.size()) return {};
    const HistoryEntry &e = m_rows.at(i.row());
    switch (role) {
    case PathRole: return e.path;
    case TitleRole: return e.title;
    case DurationRole: return e.duration;
    case PositionRole: return e.position;
    case ProgressRole: return e.duration > 0 ? qBound(0.0, e.position / e.duration, 1.0) : 0.0;
    case LastPlayedRole: return e.lastPlayed;
    case PlayCountRole: return e.playCount;
    case FavoriteRole: return e.favorite;
    }
    return {};
}

void HistoryModel::reload()
{
    const qsizetype before = m_rows.size();
    beginResetModel();
    m_rows = m_repo->list(m_filter, m_limit);
    endResetModel();
    if (before != m_rows.size()) emit countChanged();
}
