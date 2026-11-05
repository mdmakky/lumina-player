#pragma once

#include "media/ThumbnailCache.h"

#include <QObject>
#include <QThread>
#include <QTimer>

#include <atomic>
#include <memory>

class MpvThumbnailer;
class QQuickImageProvider;

// Seek-bar hover preview. QML calls request(t); frames are grabbed on a worker
// thread (latest request wins), cached in an LRU and served via image://thumb/.
// Disabled for audio-only/cover-art files, non-local sources and on any failure
// (the UI then shows the time-only tooltip).
class ThumbnailService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY availableChanged FINAL)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged FINAL)

public:
    explicit ThumbnailService(QObject *parent = nullptr);
    ~ThumbnailService() override;

    bool enabled() const { return m_enabled; }
    void setEnabled(bool on);
    bool available() const;

    // Local file to preview ("" = none: network stream, idle). Drops cache and the mpv handle.
    void setSource(const QString &path);
    void setHasVideo(bool hasVideo);

    // Url of the frame for `seconds` if cached, else "" and the grab is queued (ready() follows).
    Q_INVOKABLE QString request(double seconds);
    Q_INVOKABLE int bucketFor(double seconds) const { return ThumbnailCache::bucketOf(seconds); }
    Q_INVOKABLE QString urlFor(int bucket) const;

    QQuickImageProvider *createProvider() const; // ownership passes to the QML engine

signals:
    void availableChanged();
    void ready(int bucket);

private:
    struct Shared {
        ThumbnailCache cache;
        std::atomic<int> gen{0};
    };
    void run(); // worker thread
    void resetGrabber();

    std::shared_ptr<Shared> m_shared = std::make_shared<Shared>();
    QThread m_thread;
    QObject *m_ctx = nullptr; // lives in m_thread; target of queued work
    std::unique_ptr<MpvThumbnailer> m_grab; // worker thread only
    int m_grabGen = -1;
    QTimer m_idle;

    mutable QMutex m_mx; // guards the fields below
    QString m_path;
    int m_pending = -1;
    bool m_busy = false;
    bool m_failed = false;
    bool m_hasVideo = true;
    bool m_enabled = true;
};
