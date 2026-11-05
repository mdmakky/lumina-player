#pragma once

#include <QCache>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>

// Thread-safe LRU of seek-preview frames keyed by quantized time (2 s buckets).
class ThumbnailCache
{
public:
    static constexpr double kBucketSeconds = 2.0;
    explicit ThumbnailCache(int capacity = 200) : m_cache(capacity) {}

    static int bucketOf(double seconds) { return seconds <= 0 ? 0 : int(seconds / kBucketSeconds); }
    static double timeOf(int bucket) { return bucket * kBucketSeconds; }

    void insert(int bucket, const QImage &img)
    {
        QMutexLocker l(&m_mx);
        m_cache.insert(bucket, new QImage(img)); // QCache owns it; cost 1 each
    }
    // Returns a null image on a miss; a hit becomes most recently used.
    QImage get(int bucket)
    {
        QMutexLocker l(&m_mx);
        const QImage *p = m_cache.object(bucket);
        return p ? *p : QImage();
    }
    bool has(int bucket) { return !get(bucket).isNull(); }
    void clear() { QMutexLocker l(&m_mx); m_cache.clear(); }
    int size() const { QMutexLocker l(&m_mx); return m_cache.size(); }

private:
    mutable QMutex m_mx;
    QCache<int, QImage> m_cache;
};
