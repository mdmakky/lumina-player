#include "PlaylistModel.h"

#include <QFileInfo>

PlaylistModel::PlaylistModel(QObject *parent) : QAbstractListModel(parent) {}

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const QUrl &url = m_items.at(index.row());
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    switch (role) {
    case TitleRole: return url.isLocalFile() ? QFileInfo(path).fileName() : path;
    case PathRole: return path;
    case CurrentRole: return index.row() == m_current;
    default: return {};
    }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    return {{TitleRole, "title"}, {PathRole, "path"}, {CurrentRole, "current"}};
}

QUrl PlaylistModel::urlAt(int row) const
{
    return row >= 0 && row < m_items.size() ? m_items.at(row) : QUrl();
}

void PlaylistModel::setCurrentIndex(int index)
{
    if (index < -1 || index >= m_items.size() || index == m_current)
        return;
    const int old = m_current;
    m_current = index;
    for (int r : {old, m_current})
        if (r >= 0)
            emit dataChanged(this->index(r), this->index(r), {CurrentRole});
    emit currentIndexChanged();
}

void PlaylistModel::append(const QVariantList &urls)
{
    QList<QUrl> add;
    for (const QVariant &v : urls) {
        const QUrl u = v.toUrl();
        if (!u.isEmpty())
            add.append(u);
    }
    if (add.isEmpty())
        return;
    beginInsertRows({}, m_items.size(), m_items.size() + add.size() - 1);
    m_items += add;
    endInsertRows();
    emit countChanged();
}

void PlaylistModel::remove(int row)
{
    if (row < 0 || row >= m_items.size())
        return;
    beginRemoveRows({}, row, row);
    m_items.removeAt(row);
    endRemoveRows();
    emit countChanged();
    if (row == m_current) {
        m_current = -1;
        emit currentIndexChanged();
    } else if (row < m_current) {
        --m_current;
        emit currentIndexChanged();
    }
}

void PlaylistModel::move(int from, int to)
{
    const int n = m_items.size();
    if (from == to || from < 0 || to < 0 || from >= n || to >= n)
        return;
    beginMoveRows({}, from, from, {}, to > from ? to + 1 : to);
    m_items.move(from, to);
    endMoveRows();
    int cur = m_current;
    if (cur == from)
        cur = to;
    else if (from < cur && cur <= to)
        --cur;
    else if (to <= cur && cur < from)
        ++cur;
    if (cur != m_current) {
        m_current = cur;
        emit currentIndexChanged();
    }
}

void PlaylistModel::clear()
{
    if (m_items.isEmpty())
        return;
    beginResetModel();
    m_items.clear();
    m_current = -1;
    endResetModel();
    emit countChanged();
    emit currentIndexChanged();
}
