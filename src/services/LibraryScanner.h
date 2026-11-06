#pragma once

#include "storage/LibraryRepository.h"

#include <QObject>
#include <QStringList>

#include <atomic>

class QThread;

// Walks user-chosen folders on a worker thread and syncs the library table:
// new/changed files (by mtime+size) are upserted in batches on the GUI thread,
// vanished files are removed. Cancelable; never blocks the GUI thread on file I/O.
class LibraryScanner : public QObject
{
    Q_OBJECT
public:
    explicit LibraryScanner(LibraryRepository *repo, QObject *parent = nullptr);
    ~LibraryScanner() override;

    bool scanning() const { return m_scanning; }
    static bool isMediaFile(const QString &name);
    static bool isAudioFile(const QString &name);

public slots:
    // Starts (or, if one is running, cancels it and restarts with these roots).
    void scan(const QStringList &roots);
    void cancel();

signals:
    void progress(int filesSeen);
    void changed();             // rows were added/updated/removed
    void finished(bool canceled);

private:
    void start(const QStringList &roots);
    void done(const QStringList &vanished, bool canceled);

    LibraryRepository *m_repo;
    QThread *m_thread = nullptr;
    std::atomic<bool> m_cancel{false};
    bool m_scanning = false;
    bool m_restart = false;
    QStringList m_pending;
};
