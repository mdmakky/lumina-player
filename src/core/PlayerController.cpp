#include "PlayerController.h"

#include "engine/MpvEngine.h"
#include "utilities/Logger.h"

#include <QSettings>
#include <QUrl>

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
    , m_engine(new MpvEngine(this))
    , m_playlist(new PlaylistModel(this))
{
    connect(m_engine, &MpvEngine::endOfFile, this, [this]() {
        if (m_playlist->currentIndex() >= 0) // -1: current entry was removed; don't restart the queue
            next();
    });
    connect(m_engine, &MpvEngine::idleChanged, this, &PlayerController::hasMediaChanged);

    // Restore persisted state; keep volume/mute in sync from here on.
    QSettings settings;
    m_recentFiles = settings.value(QStringLiteral("recentFiles")).toStringList();
    m_engine->setVolume(settings.value(QStringLiteral("playback/volume"), 100).toInt());
    m_engine->setMuted(settings.value(QStringLiteral("playback/muted"), false).toBool());
    connect(m_engine, &MpvEngine::volumeChanged, this, [this] {
        setSetting(QStringLiteral("playback/volume"), m_engine->volume());
    });
    connect(m_engine, &MpvEngine::mutedChanged, this, [this] {
        setSetting(QStringLiteral("playback/muted"), m_engine->muted());
    });
}

QVariant PlayerController::setting(const QString &key, const QVariant &fallback) const
{
    return QSettings().value(key, fallback);
}

void PlayerController::setSetting(const QString &key, const QVariant &value)
{
    QSettings().setValue(key, value);
}

bool PlayerController::hasMedia() const
{
    return !m_engine->idle();
}

void PlayerController::openFiles(const QVariantList &urls)
{
    if (urls.isEmpty())
        return;

    m_playlist->clear();
    m_playlist->append(urls);
    playIndex(0);

    for (const QVariant &entry : urls) {
        const QUrl url = entry.toUrl();
        if (url.isLocalFile() || url.scheme().isEmpty())
            rememberFile(url);
    }
}

void PlayerController::enqueue(const QVariantList &urls)
{
    const int first = m_playlist->rowCount();
    m_playlist->append(urls);
    if (m_playlist->currentIndex() < 0 && m_playlist->rowCount() > first && !hasMedia())
        playIndex(first);
}

void PlayerController::playIndex(int index)
{
    const QUrl url = m_playlist->urlAt(index);
    if (url.isEmpty())
        return;
    m_playlist->setCurrentIndex(index);
    qCInfo(lcCore) << "Playing playlist entry" << index << url.toString();
    m_engine->loadFile(url);
}

void PlayerController::next()
{
    const int i = m_playlist->currentIndex();
    if (i >= 0 && i + 1 < m_playlist->rowCount())
        playIndex(i + 1);
}

void PlayerController::previous()
{
    const int i = m_playlist->currentIndex();
    if (i > 0)
        playIndex(i - 1);
}

void PlayerController::clearRecentFiles()
{
    m_recentFiles.clear();
    setSetting(QStringLiteral("recentFiles"), m_recentFiles);
    emit recentFilesChanged();
}

void PlayerController::rememberFile(const QUrl &url)
{
    const QString path = url.toLocalFile().isEmpty() ? url.toString() : url.toLocalFile();
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > 10)
        m_recentFiles.removeLast();
    setSetting(QStringLiteral("recentFiles"), m_recentFiles);
    emit recentFilesChanged();
    qCDebug(lcCore) << "Recent file added:" << path;
}
