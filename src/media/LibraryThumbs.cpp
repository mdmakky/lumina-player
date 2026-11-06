#include "LibraryThumbs.h"

#include "engine/MpvThumbnailer.h"
#include "media/LibraryMeta.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QQuickImageResponse>
#include <QRunnable>
#include <QSet>

#include <atomic>
#include <memory>

namespace {
constexpr int kWidth = 320;

class Job : public QObject, public QRunnable
{
    Q_OBJECT
public:
    Job(QString path, int kind, QString file, std::shared_ptr<std::atomic<bool>> cancel)
        : m_path(std::move(path)), m_kind(kind), m_file(std::move(file)), m_cancel(std::move(cancel)) {}

    void run() override
    {
        QImage img;
        if (!m_cancel->load()) {
            if (QFile::exists(m_file)) {
                img.load(m_file);
                QFile(m_file).setFileTime(QDateTime::currentDateTime(), QFileDevice::FileModificationTime); // LRU touch
            }
            if (img.isNull() && !m_cancel->load()) {
                MpvThumbnailer g;
                if (g.open(m_path, m_kind == 1, kWidth)) {
                    const double d = g.duration();
                    img = g.grab(m_kind == 1 || d <= 0 ? 0.0 : d * 0.10);
                }
                if (!img.isNull()) {
                    QDir().mkpath(QFileInfo(m_file).absolutePath());
                    img.save(m_file, "JPG", 82);
                }
            }
        }
        emit done(img);
    }
signals:
    void done(const QImage &img);

private:
    QString m_path;
    int m_kind;
    QString m_file;
    std::shared_ptr<std::atomic<bool>> m_cancel;
};

class Response : public QQuickImageResponse
{
public:
    Response(QThreadPool *pool, QString path, int kind, QString file)
        : m_cancel(std::make_shared<std::atomic<bool>>(false))
    {
        auto *job = new Job(std::move(path), kind, std::move(file), m_cancel);
        connect(job, &Job::done, this, [this](const QImage &img) {
            m_img = img.isNull() ? transparent() : img;
            emit finished();
        });
        pool->start(job);
    }
    QQuickTextureFactory *textureFactory() const override { return QQuickTextureFactory::textureFactoryForImage(m_img); }
    void cancel() override { m_cancel->store(true); }

private:
    static QImage transparent()
    {
        QImage t(16, 9, QImage::Format_ARGB32_Premultiplied);
        t.fill(Qt::transparent);
        return t;
    }
    std::shared_ptr<std::atomic<bool>> m_cancel;
    QImage m_img;
};
} // namespace

LibraryThumbs::LibraryThumbs(const QString &cacheDir) : m_dir(cacheDir)
{
    m_pool.setMaxThreadCount(1);
    QDir().mkpath(m_dir);
    LibraryMeta::pruneDir(m_dir, kMaxFiles);
}

QString LibraryThumbs::cacheFile(const QString &dir, const QString &path, qint64 mtime)
{
    const QByteArray h = QCryptographicHash::hash((path + u'|' + QString::number(mtime)).toUtf8(), QCryptographicHash::Sha1);
    return dir + u'/' + QString::fromLatin1(h.toHex()) + QStringLiteral(".jpg");
}

QQuickImageResponse *LibraryThumbs::requestImageResponse(const QString &id, const QSize &)
{
    const QStringList p = id.split(u'/');
    const QString path = p.size() == 3
        ? QString::fromUtf8(QByteArray::fromBase64(p[0].toLatin1(), QByteArray::Base64UrlEncoding)) : QString();
    if (++m_writes % 100 == 0) LibraryMeta::pruneDir(m_dir, kMaxFiles); // keep the cache bounded while running
    return new Response(&m_pool, path, p.size() == 3 ? p[1].toInt() : 0, cacheFile(m_dir, path, p.size() == 3 ? p[2].toLongLong() : 0));
}

#include "LibraryThumbs.moc"
