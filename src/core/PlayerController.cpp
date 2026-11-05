#include "PlayerController.h"

#include "engine/MpvEngine.h"
#include "utilities/Logger.h"

#include <QUrl>

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
    , m_engine(new MpvEngine(this))
{
    connect(m_engine, &MpvEngine::idleChanged, this, &PlayerController::hasMediaChanged);
}

bool PlayerController::hasMedia() const
{
    return !m_engine->idle();
}

void PlayerController::openFiles(const QVariantList &urls)
{
    if (urls.isEmpty())
        return;

    m_engine->loadFiles(urls);

    for (const QVariant &entry : urls) {
        const QUrl url = entry.toUrl();
        if (url.isLocalFile() || url.scheme().isEmpty())
            rememberFile(url);
    }
}

void PlayerController::clearRecentFiles()
{
    m_recentFiles.clear();
    emit recentFilesChanged();
}

void PlayerController::rememberFile(const QUrl &url)
{
    const QString path = url.toLocalFile().isEmpty() ? url.toString() : url.toLocalFile();
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > 10)
        m_recentFiles.removeLast();
    emit recentFilesChanged();
    qCDebug(lcCore) << "Recent file added:" << path;
}
