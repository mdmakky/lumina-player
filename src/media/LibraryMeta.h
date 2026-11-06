#pragma once

#include <QDir>
#include <QString>
#include <QStringList>
#include <QVariantMap>

// Pure helpers for library metadata (no mpv / GUI): tag parsing, duplicate
// fingerprints, M3U text and thumbnail-cache pruning. Unit tested.
namespace LibraryMeta {

struct Tags
{
    QString title, artist, album;
    int track = 0, year = 0;
};

// Keys are matched case-insensitively (ID3 "TITLE", Vorbis "title", ...).
Tags parseTags(const QVariantMap &metadata);
int parseTrack(const QString &s);  // "3/12" -> 3, junk -> 0
int parseYear(const QString &s);   // "2019-05-01" / "2019" -> 2019, junk -> 0
QString resolutionLabel(int height); // 2160 -> "4K", 1080 -> "1080p", 0 -> ""

// image://libthumb url for a library row (mtime busts the cache when the file changes); see LibraryThumbs.
QString thumbUrl(const QString &path, int kind, qint64 mtime);

// size + first/last 64 KiB hashed ("" if unreadable). Equal fingerprints + equal duration = duplicate.
QString fingerprint(const QString &path);

QString toM3u(const QStringList &paths);
// Skips comments/blank lines; relative entries resolve against baseDir; URLs are kept.
QStringList parseM3u(const QString &text, const QDir &baseDir);

// Keeps the `maxFiles` most recently used files in dir, deletes the rest (LRU by mtime). Returns deleted count.
int pruneDir(const QString &dir, int maxFiles);

} // namespace LibraryMeta
