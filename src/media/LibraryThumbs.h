#pragma once

#include <QQuickImageProvider>
#include <QString>
#include <QThreadPool>

// image://libthumb/<base64url(path)>/<kind>/<mtime> -> poster frame for the library grid.
// Video: one frame at ~10% of the duration; music: the embedded cover. Rendered by a dedicated
// single-thread pool (mpv per file, so never parallel), cached as JPEG under `cacheDir` and
// pruned LRU-style. Requests for tiles that scrolled away are canceled before mpv is started.
// A failed grab yields a transparent image so the QML placeholder stays visible.
class LibraryThumbs : public QQuickAsyncImageProvider
{
public:
    static constexpr int kMaxFiles = 3000;

    explicit LibraryThumbs(const QString &cacheDir);
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

    static QString cacheFile(const QString &dir, const QString &path, qint64 mtime);

private:
    QString m_dir;
    QThreadPool m_pool;
    int m_writes = 0;
};
