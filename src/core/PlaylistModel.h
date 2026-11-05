#pragma once

#include <QAbstractListModel>
#include <QUrl>
#include <QVariantList>

// Ordered play queue; the single source of truth for what plays next.
class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    enum Roles { TitleRole = Qt::UserRole + 1, PathRole, CurrentRole };

    explicit PlaylistModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int currentIndex() const { return m_current; }
    void setCurrentIndex(int index);
    QUrl urlAt(int row) const;

    Q_INVOKABLE void append(const QVariantList &urls);
    Q_INVOKABLE void remove(int row);
    // Moves the item so it ends up at row `to`.
    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void clear();

signals:
    void currentIndexChanged();
    void countChanged();

private:
    QList<QUrl> m_items;
    int m_current = -1;
};
