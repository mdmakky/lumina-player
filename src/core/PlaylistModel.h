#pragma once

#include <QAbstractListModel>
#include <QDir>
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

    // M3U/M3U8 persistence. Relative entries resolve against `baseDir`.
    QString toM3u() const;
    static QList<QUrl> parseM3u(const QString &text, const QDir &baseDir);
    Q_INVOKABLE bool saveToFile(const QUrl &file) const;
    // Returns the entries found; the caller decides whether to replace the queue.
    static QVariantList loadFromFile(const QUrl &file);

signals:
    void currentIndexChanged();
    void countChanged();

private:
    QList<QUrl> m_items;
    int m_current = -1;
};
