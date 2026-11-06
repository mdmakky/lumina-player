#pragma once

#include "storage/LibraryRepository.h"

#include <QObject>

#include <atomic>

class QThread;

// Fills in metadata (duration, resolution, tags, cover art, duplicate fingerprint) for library rows that
// were not probed yet. One worker thread, one short-lived mpv handle per file, small pause between files
// (rate limit); results are written on the GUI thread in small batches. Cancelable and incremental:
// whatever is still unprobed is simply picked up by the next start().
class LibraryProber : public QObject
{
    Q_OBJECT
public:
    explicit LibraryProber(LibraryRepository *repo, QObject *parent = nullptr);
    ~LibraryProber() override;

    bool running() const { return m_running; }

public slots:
    void start(); // no-op if a run is active (it is restarted when it ends so new rows are included)
    void cancel();

signals:
    void changed();   // a batch was stored
    void finished();

private:
    void done();

    LibraryRepository *m_repo;
    QThread *m_thread = nullptr;
    std::atomic<bool> m_cancel{false};
    bool m_running = false;
    bool m_again = false;
};
