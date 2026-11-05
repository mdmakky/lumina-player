#include "ThumbnailService.h"

#include "engine/MpvThumbnailer.h"

#include <QPainter>
#include <QPainterPath>
#include <QQuickImageProvider>

namespace {
// Serves cached frames to QML; holds the shared state so it can outlive the service.
class Provider : public QQuickImageProvider
{
public:
    Provider(std::shared_ptr<ThumbnailCache> cache, std::shared_ptr<std::atomic<int>> gen)
        : QQuickImageProvider(QQuickImageProvider::Image), m_cache(std::move(cache)), m_gen(std::move(gen)) {}

    // id = "<generation>/<bucket>"; stale generations (previous file) miss.
    QImage requestImage(const QString &id, QSize *size, const QSize &) override
    {
        const QStringList p = id.split(QLatin1Char('/'));
        QImage img;
        if (p.size() == 2 && p[0].toInt() == m_gen->load())
            img = m_cache->get(p[1].toInt());
        if (size)
            *size = img.size();
        return img;
    }

private:
    std::shared_ptr<ThumbnailCache> m_cache;
    std::shared_ptr<std::atomic<int>> m_gen;
};
} // namespace

ThumbnailService::ThumbnailService(QObject *parent)
    : QObject(parent)
    , m_ctx(new QObject)
{
    m_ctx->moveToThread(&m_thread);
    m_thread.start(QThread::LowPriority);
    m_idle.setSingleShot(true);
    m_idle.setInterval(20000);
    connect(&m_idle, &QTimer::timeout, this, [this] { resetGrabber(); });
}

ThumbnailService::~ThumbnailService()
{
    {
        QMutexLocker l(&m_mx);
        m_pending = -1;
        m_failed = true;
    }
    m_thread.quit();
    m_thread.wait();
    m_grab.reset();
    delete m_ctx;
}

bool ThumbnailService::available() const
{
    QMutexLocker l(&m_mx);
    return m_enabled && m_hasVideo && !m_failed && !m_path.isEmpty();
}

void ThumbnailService::setEnabled(bool on)
{
    {
        QMutexLocker l(&m_mx);
        if (m_enabled == on)
            return;
        m_enabled = on;
        m_pending = -1;
    }
    if (!on)
        resetGrabber();
    emit availableChanged();
}

void ThumbnailService::setHasVideo(bool v)
{
    {
        QMutexLocker l(&m_mx);
        if (m_hasVideo == v)
            return;
        m_hasVideo = v;
    }
    emit availableChanged();
}

void ThumbnailService::setSource(const QString &path)
{
    {
        QMutexLocker l(&m_mx);
        m_path = path;
        m_pending = -1;
        m_failed = false;
        m_hasVideo = true; // re-evaluated once the new file reports its tracks
        ++m_shared->gen;
    }
    m_shared->cache.clear();
    resetGrabber();
    emit availableChanged();
}

// Closes the second mpv handle (unless a grab is running; run() closes it on exit then).
void ThumbnailService::resetGrabber()
{
    m_idle.stop();
    QMetaObject::invokeMethod(m_ctx, [this] {
        QMutexLocker l(&m_mx);
        if (!m_busy)
            m_grab.reset();
    });
}

QString ThumbnailService::urlFor(int bucket) const
{
    return QStringLiteral("image://thumb/%1/%2").arg(m_shared->gen.load()).arg(bucket);
}

QString ThumbnailService::request(double seconds)
{
    if (!available())
        return {};
    const int b = ThumbnailCache::bucketOf(seconds);
    if (m_shared->cache.has(b))
        return urlFor(b);
    bool kick = false;
    {
        QMutexLocker l(&m_mx);
        m_pending = b; // latest wins; older queued requests are dropped
        if (!m_busy)
            kick = m_busy = true;
    }
    if (kick)
        QMetaObject::invokeMethod(m_ctx, [this] { run(); });
    m_idle.start();
    return {};
}

void ThumbnailService::run()
{
    for (;;) {
        int b, gen;
        QString path;
        {
            QMutexLocker l(&m_mx);
            if (m_pending < 0 || m_failed || !m_enabled) {
                m_pending = -1;
                m_busy = false;
                if (m_grab && (m_grabGen != m_shared->gen || m_path.isEmpty() || !m_enabled))
                    m_grab.reset();
                return;
            }
            b = m_pending;
            m_pending = -1;
            path = m_path;
            gen = m_shared->gen;
        }
        if (!m_grab || m_grabGen != gen) {
            m_grab = std::make_unique<MpvThumbnailer>();
            m_grabGen = gen;
            if (!m_grab->open(path)) {
                m_grab.reset();
                {
                    QMutexLocker l(&m_mx);
                    if (gen == m_shared->gen)
                        m_failed = true; // give up on this file: UI falls back to time-only
                }
                QMetaObject::invokeMethod(this, &ThumbnailService::availableChanged, Qt::QueuedConnection);
                continue;
            }
        }
        const QImage img = m_grab->grab(ThumbnailCache::timeOf(b));
        if (img.isNull() || gen != m_shared->gen)
            continue;
        // Pre-round the corners (Qt Quick has no cheap clip-to-radius).
        QImage rounded(img.size(), QImage::Format_ARGB32_Premultiplied);
        rounded.fill(Qt::transparent);
        QPainter pa(&rounded);
        pa.setRenderHint(QPainter::Antialiasing);
        QPainterPath clip;
        clip.addRoundedRect(QRectF(rounded.rect()), 6, 6);
        pa.setClipPath(clip);
        pa.drawImage(0, 0, img);
        pa.end();
        m_shared->cache.insert(b, rounded);
        QMetaObject::invokeMethod(this, [this, b, gen] {
            if (gen == m_shared->gen)
                emit ready(b);
        }, Qt::QueuedConnection);
    }
}

QQuickImageProvider *ThumbnailService::createProvider() const
{
    // Aliasing shared_ptrs keep the whole Shared block alive while the provider exists.
    return new Provider(std::shared_ptr<ThumbnailCache>(m_shared, &m_shared->cache),
                        std::shared_ptr<std::atomic<int>>(m_shared, &m_shared->gen));
}
