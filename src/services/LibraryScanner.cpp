#include "LibraryScanner.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <QThread>

bool LibraryScanner::isMediaFile(const QString &name)
{
    static const QSet<QString> exts = {
        "mp4", "mkv", "avi", "mov", "webm", "m4v", "wmv", "flv", "ts", "m2ts", "mpg", "mpeg", "ogv", "3gp",
        "mp3", "flac", "wav", "ogg", "oga", "opus", "m4a", "aac"};
    return exts.contains(QFileInfo(name).suffix().toLower());
}

bool LibraryScanner::isAudioFile(const QString &name)
{
    static const QSet<QString> exts = {"mp3", "flac", "wav", "ogg", "oga", "opus", "m4a", "aac"};
    return exts.contains(QFileInfo(name).suffix().toLower());
}

LibraryScanner::LibraryScanner(LibraryRepository *repo, QObject *parent)
    : QObject(parent), m_repo(repo) {}

LibraryScanner::~LibraryScanner()
{
    cancel();
    if (m_thread) m_thread->wait();
}

void LibraryScanner::cancel() { m_cancel = true; }

void LibraryScanner::scan(const QStringList &roots)
{
    if (!m_repo->available()) return;
    if (m_scanning) { // restart once the running worker has wound down
        m_cancel = true;
        m_restart = true;
        m_pending = roots;
        return;
    }
    start(roots);
}

void LibraryScanner::start(const QStringList &roots)
{
    m_scanning = true;
    m_cancel = false;
    const auto known = m_repo->known(); // snapshot; the worker never touches the DB
    m_thread = QThread::create([this, roots, known] {
        QSet<QString> seen;
        QList<LibraryItem> batch;
        int count = 0;
        QStringList skippedRoots; // roots that are unreadable right now (drive unplugged): keep their rows
        const auto flush = [&] {
            if (batch.isEmpty()) return;
            QMetaObject::invokeMethod(this, [this, b = std::move(batch)] {
                m_repo->upsert(b);
                emit changed();
            }, Qt::QueuedConnection);
            batch.clear();
        };
        for (const QString &rootIn : roots) {
            const QString root = QDir::cleanPath(rootIn);
            if (!QFileInfo(root).isDir()) { skippedRoots << root; continue; }
            QDirIterator it(root, QDir::Files | QDir::Readable, QDirIterator::Subdirectories);
            while (it.hasNext() && !m_cancel) {
                const QString path = it.next();
                if (!isMediaFile(path)) continue;
                const QFileInfo fi = it.fileInfo();
                seen.insert(path);
                if ((++count % 50) == 0)
                    QMetaObject::invokeMethod(this, [this, count] { emit progress(count); }, Qt::QueuedConnection);
                const qint64 mtime = fi.lastModified().toSecsSinceEpoch();
                const auto k = known.constFind(path);
                if (k != known.constEnd() && k->first == mtime && k->second == fi.size())
                    continue; // unchanged: incremental
                LibraryItem i;
                i.path = path;
                i.title = fi.completeBaseName();
                i.folder = fi.absolutePath();
                i.kind = isAudioFile(path) ? 1 : 0;
                i.size = fi.size();
                i.mtime = mtime;
                batch.append(i);
                if (batch.size() >= 200) flush();
            }
        }
        flush();
        const bool canceled = m_cancel;
        QStringList vanished;
        if (!canceled) {
            for (auto k = known.constBegin(); k != known.constEnd(); ++k) {
                if (seen.contains(k.key())) continue;
                for (const QString &r : roots) {
                    const QString root = QDir::cleanPath(r);
                    if (!skippedRoots.contains(root) && k.key().startsWith(root + u'/')) {
                        vanished << k.key();
                        break;
                    }
                }
            }
        }
        const int total = count;
        QMetaObject::invokeMethod(this, [this, vanished, canceled, total] {
            emit progress(total);
            done(vanished, canceled);
        }, Qt::QueuedConnection);
    });
    QThread *t = m_thread;
    t->setParent(this);
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    connect(t, &QObject::destroyed, this, [this, t] { if (m_thread == t) m_thread = nullptr; });
    t->start();
}

void LibraryScanner::done(const QStringList &vanished, bool canceled)
{
    if (!vanished.isEmpty()) {
        m_repo->remove(vanished);
        emit changed();
    }
    m_scanning = false;
    emit finished(canceled);
    if (m_restart) {
        m_restart = false;
        start(m_pending);
    }
}
