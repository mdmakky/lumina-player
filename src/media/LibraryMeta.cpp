#include "LibraryMeta.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>

#include <algorithm>

namespace LibraryMeta {

int parseTrack(const QString &s)
{
    return qMax(0, s.section(u'/', 0, 0).trimmed().toInt());
}

int parseYear(const QString &s)
{
    const int y = s.trimmed().left(4).toInt();
    return y >= 1000 && y <= 2999 ? y : 0;
}

Tags parseTags(const QVariantMap &metadata)
{
    QVariantMap m;
    for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it)
        m.insert(it.key().toLower(), it.value());
    const auto str = [&](const char *k) { return m.value(QLatin1String(k)).toString().trimmed(); };
    Tags t;
    t.title = str("title");
    t.artist = !str("artist").isEmpty() ? str("artist") : str("album_artist");
    t.album = str("album");
    t.track = parseTrack(str("track"));
    t.year = parseYear(!str("date").isEmpty() ? str("date") : str("year"));
    return t;
}

QString resolutionLabel(int h)
{
    if (h <= 0) return {};
    if (h >= 2000) return QStringLiteral("4K");
    if (h >= 1400) return QStringLiteral("1440p");
    if (h >= 1000) return QStringLiteral("1080p");
    if (h >= 700) return QStringLiteral("720p");
    return QStringLiteral("%1p").arg(h);
}

QString thumbUrl(const QString &path, int kind, qint64 mtime)
{
    return QStringLiteral("image://libthumb/") + QString::fromLatin1(path.toUtf8().toBase64(QByteArray::Base64UrlEncoding))
           + u'/' + QString::number(kind) + u'/' + QString::number(mtime);
}

QString fingerprint(const QString &path)
{
    constexpr qint64 kChunk = 64 * 1024;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    const qint64 size = f.size();
    QCryptographicHash h(QCryptographicHash::Sha1);
    h.addData(QByteArray::number(size));
    h.addData(f.read(kChunk));
    if (size > kChunk) {
        f.seek(qMax<qint64>(kChunk, size - kChunk)); // no overlap with the head chunk
        h.addData(f.read(kChunk));
    }
    return QString::fromLatin1(h.result().toHex());
}

QString toM3u(const QStringList &paths)
{
    return QStringLiteral("#EXTM3U\n") + paths.join(u'\n') + u'\n';
}

QStringList parseM3u(const QString &text, const QDir &baseDir)
{
    QStringList out;
    for (QString line : text.split(u'\n')) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(u'#')) continue;
        if (line.contains(QStringLiteral("://"))) out << line;
        else out << QDir::cleanPath(QFileInfo(line).isAbsolute() ? line : baseDir.absoluteFilePath(line));
    }
    return out;
}

int pruneDir(const QString &dir, int maxFiles)
{
    QFileInfoList files = QDir(dir).entryInfoList(QDir::Files, QDir::NoSort);
    if (files.size() <= maxFiles) return 0;
    std::sort(files.begin(), files.end(), [](const QFileInfo &a, const QFileInfo &b) {
        return a.lastModified() > b.lastModified(); // newest first
    });
    int n = 0;
    for (int i = maxFiles; i < files.size(); ++i)
        n += QFile::remove(files.at(i).absoluteFilePath());
    return n;
}

} // namespace LibraryMeta
