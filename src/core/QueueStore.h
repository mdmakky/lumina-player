#pragma once

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStringList>

// "Remember queue on exit": the playlist as a small JSON file. Pure helpers, unit-tested.
namespace Lumina::QueueStore {

struct Queue {
    QStringList urls; // URL strings (file:// or network)
    int index = -1;   // last played entry, -1 = none
};

inline QByteArray toJson(const Queue &q)
{
    return QJsonDocument(QJsonObject{{QStringLiteral("index"), q.index},
                                     {QStringLiteral("urls"), QJsonArray::fromStringList(q.urls)}})
        .toJson(QJsonDocument::Compact);
}

// Malformed input yields an empty queue; the index is clamped into range.
inline Queue fromJson(const QByteArray &data)
{
    Queue q;
    const QJsonObject o = QJsonDocument::fromJson(data).object();
    for (const QJsonValue &v : o.value(QStringLiteral("urls")).toArray())
        if (v.isString() && !v.toString().isEmpty())
            q.urls << v.toString();
    const int i = o.value(QStringLiteral("index")).toInt(-1);
    q.index = (i >= 0 && i < q.urls.size()) ? i : -1;
    return q;
}

inline bool save(const QString &path, const Queue &q)
{
    QSaveFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(toJson(q)) >= 0 && f.commit();
}

inline Queue load(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? fromJson(f.readAll()) : Queue{};
}

} // namespace Lumina::QueueStore
