#include "LibraryProber.h"

#include "engine/MpvProbe.h"
#include "media/LibraryMeta.h"
#include "services/LibraryScanner.h"

#include <QFileInfo>
#include <QThread>

LibraryProber::LibraryProber(LibraryRepository *repo, QObject *parent) : QObject(parent), m_repo(repo) {}

LibraryProber::~LibraryProber()
{
    cancel();
    if (m_thread) m_thread->wait();
}

void LibraryProber::cancel()
{
    m_cancel = true;
    m_again = false;
}

void LibraryProber::start()
{
    if (!m_repo->available()) return;
    if (m_running) { m_again = true; return; }
    const QStringList todo = m_repo->unprobed(); // snapshot; the worker never touches the DB
    if (todo.isEmpty()) return;
    m_running = true;
    m_cancel = false;
    m_thread = QThread::create([this, todo] {
        QList<LibraryItem> batch;
        const auto flush = [&] {
            if (batch.isEmpty()) return;
            QMetaObject::invokeMethod(this, [this, b = std::move(batch)] {
                m_repo->updateMeta(b);
                emit changed();
            }, Qt::QueuedConnection);
            batch.clear();
        };
        for (const QString &path : todo) {
            if (m_cancel) break;
            if (!QFileInfo::exists(path)) continue; // unplugged drive: stays unprobed for next time
            LibraryItem i;
            i.path = path;
            i.kind = LibraryScanner::isAudioFile(path) ? 1 : 0;
            if (!MpvProbe::probe(i)) i.duration = -1; // unreadable: still marked probed so we do not retry forever
            i.fhash = LibraryMeta::fingerprint(path);
            batch.append(i);
            if (batch.size() >= 8) flush();
            QThread::msleep(15); // rate limit: never saturate disk/CPU during playback
        }
        flush();
        QMetaObject::invokeMethod(this, [this] { done(); }, Qt::QueuedConnection);
    });
    QThread *t = m_thread;
    t->setParent(this);
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    connect(t, &QObject::destroyed, this, [this, t] { if (m_thread == t) m_thread = nullptr; });
    t->start(QThread::LowPriority);
}

void LibraryProber::done()
{
    m_running = false;
    emit finished();
    if (m_again) { m_again = false; start(); }
}
