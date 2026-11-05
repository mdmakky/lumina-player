#pragma once

#include <QAbstractListModel>
#include <QDir>
#include <QHash>
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
    // Better display title for the current remote entry once the engine knows it.
    void setCurrentTitle(const QString &title);

    Q_INVOKABLE void append(const QVariantList &urls);
    Q_INVOKABLE void remove(int row);
    // Moves the item so it ends up at row `to`.
    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void clear();

    // Pure next/previous selection (-1 = nothing to play). repeat: 0 off, 1 all, 2 one.
    // `autoAdvance` is true at end-of-file (repeat-one replays the entry; manual next skips it).
    // Shuffle never repeats an entry until all were played; history feeds previousIndex().
    int nextIndex(bool shuffle, int repeat, bool autoAdvance);
    int previousIndex(bool shuffle);

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
    QHash<QUrl, QString> m_titles; // remote entries: media-title once known
    QList<QUrl> m_history; // shuffle: entries already played (excludes current)
    int m_round = 0;       // history index where the current shuffle round began
    int m_current = -1;
};
