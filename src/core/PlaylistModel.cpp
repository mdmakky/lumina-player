#include "PlaylistModel.h"

#include "utilities/UrlUtils.h"

#include <QRandomGenerator>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

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
    case TitleRole:
        if (url.isLocalFile()) return QFileInfo(path).fileName();
        return m_titles.value(url, Lumina::UrlUtils::isRemote(url) ? Lumina::UrlUtils::titleFor(url) : path);
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

void PlaylistModel::setCurrentTitle(const QString &title)
{
    if (m_current < 0 || m_current >= m_items.size() || title.isEmpty() || m_items.at(m_current).isLocalFile())
        return;
    m_titles.insert(m_items.at(m_current), title);
    emit dataChanged(index(m_current), index(m_current), {TitleRole});
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
    m_history.clear();
    m_titles.clear();
    m_round = 0;
    m_current = -1;
    endResetModel();
    emit countChanged();
    emit currentIndexChanged();
}

int PlaylistModel::nextIndex(bool shuffle, int repeat, bool autoAdvance)
{
    const int n = m_items.size();
    if (n == 0)
        return -1;
    if (repeat == 2 && autoAdvance && m_current >= 0)
        return m_current;
    if (!shuffle)
        return m_current + 1 < n ? m_current + 1 : (repeat == 1 ? 0 : -1);

    if (m_current >= 0)
        m_history.append(m_items.value(m_current));
    auto pick = [&] {
        QList<int> c;
        for (int i = 0; i < n; ++i)
            if (i != m_current && !m_history.mid(m_round).contains(m_items.at(i)))
                c.append(i);
        return c;
    };
    QList<int> c = pick();
    if (c.isEmpty() && repeat == 1) { // everything played: start a new round
        m_round = m_history.size(); // new round; history kept for previous
        for (int i = 0; i < n; ++i)
            if (i != m_current || n == 1)
                c.append(i);
    }
    if (c.isEmpty())
        return -1;
    return c.at(QRandomGenerator::global()->bounded(c.size()));
}

int PlaylistModel::previousIndex(bool shuffle)
{
    if (shuffle) {
        while (!m_history.isEmpty()) {
            const int i = m_items.indexOf(m_history.takeLast());
            m_round = qMin(m_round, int(m_history.size()));
            if (i >= 0)
                return i;
        }
        return -1;
    }
    return m_current > 0 ? m_current - 1 : -1;
}

QString PlaylistModel::toM3u() const
{
    QString out = QStringLiteral("#EXTM3U\n");
    for (const QUrl &u : m_items)
        out += (u.isLocalFile() ? u.toLocalFile() : u.toString()) + QLatin1Char('\n');
    return out;
}

QList<QUrl> PlaylistModel::parseM3u(const QString &text, const QDir &baseDir)
{
    QList<QUrl> urls;
    const auto lines = text.split(QLatin1Char('\n'));
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        if (line.contains(QStringLiteral("://")) && !line.startsWith(QStringLiteral("file://")))
            urls << QUrl(line); // http(s):// etc.
        else if (line.startsWith(QStringLiteral("file://")))
            urls << QUrl(line);
        else
            urls << QUrl::fromLocalFile(QDir::isAbsolutePath(line) ? line : baseDir.absoluteFilePath(line));
    }
    return urls;
}

bool PlaylistModel::saveToFile(const QUrl &file) const
{
    QFile f(file.toLocalFile());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    f.write(toM3u().toUtf8());
    return true;
}

QVariantList PlaylistModel::loadFromFile(const QUrl &file)
{
    QFile f(file.toLocalFile());
    QVariantList result;
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;
    const QDir base = QFileInfo(f).dir();
    for (const QUrl &u : parseM3u(QString::fromUtf8(f.readAll()), base))
        result << u;
    return result;
}
