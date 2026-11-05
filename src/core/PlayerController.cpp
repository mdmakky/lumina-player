#include "PlayerController.h"

#include "engine/MpvEngine.h"
#include "utilities/Logger.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QSettings>
#include <QTimer>
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

    // Resume where the file was left (disabled in self-test for determinism).
    connect(m_engine, &MpvEngine::durationChanged, this, [this]() {
        if (m_resumeTarget > 0.0 && m_engine->duration() > m_resumeTarget + 10.0) {
            m_engine->seek(m_resumeTarget);
            qCInfo(lcCore) << "Resuming at" << m_resumeTarget;
        }
        if (m_engine->duration() > 0.0)
            m_resumeTarget = 0.0;
    });
    connect(qApp, &QCoreApplication::aboutToQuit, this, &PlayerController::saveResume);
    auto *resumeTimer = new QTimer(this); // also survives crashes / SIGTERM
    resumeTimer->setInterval(5000);
    connect(resumeTimer, &QTimer::timeout, this, &PlayerController::saveResume);
    resumeTimer->start();

    // Restore persisted state; keep volume/mute in sync from here on.
    QSettings settings;
    m_recentFiles = settings.value(QStringLiteral("recentFiles")).toStringList();
    m_engine->setVolume(settings.value(QStringLiteral("playback/defaultVolume"),
                                       settings.value(QStringLiteral("playback/volume"), 100)).toInt());
    m_engine->setHwdec(settings.value(QStringLiteral("playback/hwdec"), QStringLiteral("auto-safe")).toString());
    if (settings.contains(QStringLiteral("subtitles/fontSize")))
        m_engine->setSubtitleFontSize(settings.value(QStringLiteral("subtitles/fontSize")).toInt());
    if (settings.contains(QStringLiteral("subtitles/color")))
        m_engine->setSubtitleColor(settings.value(QStringLiteral("subtitles/color")).toString());
    if (settings.contains(QStringLiteral("subtitles/borderSize")))
        m_engine->setSubtitleBorderSize(settings.value(QStringLiteral("subtitles/borderSize")).toDouble());
    if (settings.contains(QStringLiteral("subtitles/position")))
        m_engine->setSubtitlePosition(settings.value(QStringLiteral("subtitles/position")).toInt());
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
    saveResume();
    m_currentPath = url.isLocalFile() ? url.toLocalFile() : QString();
    m_resumeTarget = (!m_currentPath.isEmpty() && !qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST")
                      && setting(QStringLiteral("playback/resume"), true).toBool())
        ? setting(resumeKey(m_currentPath), 0.0).toDouble() : 0.0;
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

bool PlayerController::savePlaylist(const QUrl &file)
{
    return m_playlist->saveToFile(file);
}

bool PlayerController::loadPlaylist(const QUrl &file)
{
    const QVariantList urls = PlaylistModel::loadFromFile(file);
    if (urls.isEmpty())
        return false;
    openFiles(urls);
    return true;
}

QString PlayerController::resumeKey(const QString &path)
{
    return QStringLiteral("resume/")
        + QString::fromLatin1(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Md5).toHex());
}

void PlayerController::saveResume()
{
    if (m_currentPath.isEmpty() || m_engine->idle() || qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST")
        || !setting(QStringLiteral("playback/resume"), true).toBool())
        return;
    const double pos = m_engine->position(), dur = m_engine->duration();
    QSettings s;
    if (pos > 5.0 && dur - pos > 10.0)
        s.setValue(resumeKey(m_currentPath), pos);
    else
        s.remove(resumeKey(m_currentPath));
}
