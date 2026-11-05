#include "PlayerController.h"

#include "engine/MpvEngine.h"
#include "utilities/Logger.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
    , m_engine(new MpvEngine(this))
    , m_playlist(new PlaylistModel(this))
{
    connect(m_engine, &MpvEngine::endOfFile, this, [this]() {
        if (m_playlist->currentIndex() >= 0) // -1: current entry was removed; don't restart the queue
            advance(true);
    });
    connect(m_engine, &MpvEngine::idleChanged, this, &PlayerController::hasMediaChanged);

    // Library is optional: LUMINA_PLAYBACK_TEST never touches it; LUMINA_DB_PATH redirects it (tests).
    if (!qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST"))
        m_database = std::make_unique<Database>(qEnvironmentVariable("LUMINA_DB_PATH"));
    m_repo = HistoryRepository(m_database.get());
    m_history = new HistoryModel(&m_repo, HistoryRepository::Filter::All, 50, this);
    m_continue = new HistoryModel(&m_repo, HistoryRepository::Filter::Continue, 12, this);
    m_favorites = new HistoryModel(&m_repo, HistoryRepository::Filter::Favorites, 50, this);
    connect(m_engine, &MpvEngine::idleChanged, this, [this] { if (m_engine->idle()) reloadHistory(); });

    m_libRepo = LibraryRepository(m_database.get());
    m_library = new LibraryModel(&m_libRepo, this);
    m_scanner = new LibraryScanner(&m_libRepo, this);
    connect(m_scanner, &LibraryScanner::changed, m_library, &LibraryModel::reload);
    connect(m_scanner, &LibraryScanner::progress, this, [this](int n) {
        m_libraryProgress = n;
        emit libraryProgressChanged();
    });
    connect(m_scanner, &LibraryScanner::finished, this, [this] {
        m_library->reload();
        emit libraryScanningChanged();
    });

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
    m_shuffle = settings.value(QStringLiteral("playback/shuffle"), false).toBool();
    m_repeat = qBound(0, settings.value(QStringLiteral("playback/repeat"), 0).toInt(), 2);
    m_libraryFolders = settings.value(QStringLiteral("library/folders")).toStringList();
    migrateLegacy();
    reloadHistory();
    m_library->reload();
    if (!m_libraryFolders.isEmpty())
        QTimer::singleShot(0, this, &PlayerController::rescanLibrary);
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

void PlayerController::addLibraryFolder(const QUrl &folder)
{
    const QString dir = QDir::cleanPath(folder.isLocalFile() ? folder.toLocalFile() : folder.toString());
    if (dir.isEmpty() || !QFileInfo(dir).isDir() || m_libraryFolders.contains(dir))
        return;
    m_libraryFolders.append(dir);
    setSetting(QStringLiteral("library/folders"), m_libraryFolders);
    emit libraryFoldersChanged();
    rescanLibrary();
}

void PlayerController::removeLibraryFolder(const QString &folder)
{
    if (!m_libraryFolders.removeOne(folder))
        return;
    setSetting(QStringLiteral("library/folders"), m_libraryFolders);
    m_libRepo.removeFolder(folder);
    m_library->reload();
    emit libraryFoldersChanged();
    rescanLibrary(); // restarts a running scan so it no longer re-adds the removed folder
}

void PlayerController::rescanLibrary()
{
    if (!m_libRepo.available())
        return;
    m_libraryProgress = 0;
    emit libraryProgressChanged();
    m_scanner->scan(m_libraryFolders);
    emit libraryScanningChanged();
}

void PlayerController::playLibrary(int index)
{
    const QVariantList urls = m_library->urls();
    if (index < 0 || index >= urls.size())
        return;
    m_playlist->clear();
    m_playlist->append(urls);
    playIndex(index);
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
    m_resumeTarget = 0.0;
    if (!m_currentPath.isEmpty() && !qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST")
        && setting(QStringLiteral("playback/resume"), true).toBool()) {
        m_resumeTarget = m_repo.available() ? m_repo.entry(m_currentPath).position
                                            : setting(resumeKey(m_currentPath), 0.0).toDouble();
        m_repo.recordPlay(m_currentPath, QFileInfo(m_currentPath).completeBaseName());
        reloadHistory();
    }
    m_playlist->setCurrentIndex(index);
    qCInfo(lcCore) << "Playing playlist entry" << index << url.toString();
    m_engine->loadFile(url);
}

void PlayerController::advance(bool autoAdvance)
{
    const int i = m_playlist->nextIndex(m_shuffle, m_repeat, autoAdvance);
    if (i >= 0)
        playIndex(i);
}

void PlayerController::next()
{
    advance(false);
}

void PlayerController::previous()
{
    const int i = m_playlist->previousIndex(m_shuffle);
    if (i >= 0)
        playIndex(i);
}

void PlayerController::setShuffle(bool on)
{
    if (on == m_shuffle)
        return;
    m_shuffle = on;
    setSetting(QStringLiteral("playback/shuffle"), on);
    emit shuffleChanged();
}

void PlayerController::setRepeatMode(int mode)
{
    mode = qBound(0, mode, 2);
    if (mode == m_repeat)
        return;
    m_repeat = mode;
    setSetting(QStringLiteral("playback/repeat"), mode);
    emit repeatModeChanged();
}

QString PlayerController::takeScreenshot()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                        + QStringLiteral("/Lumina");
    const QString path = dir + QStringLiteral("/Lumina-")
                         + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"))
                         + QStringLiteral(".png");
    if (!QDir().mkpath(dir) || !m_engine->screenshot(path)) {
        emit toast(tr("Screenshot failed"));
        return {};
    }
    emit toast(tr("Saved screenshot"));
    return path;
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
    const bool resumable = pos > 5.0 && dur - pos > 10.0;
    if (m_repo.available()) {
        m_repo.updateProgress(m_currentPath, resumable ? pos : 0.0, dur);
        return;
    }
    QSettings s; // fallback when the library DB is unavailable
    if (resumable)
        s.setValue(resumeKey(m_currentPath), pos);
    else
        s.remove(resumeKey(m_currentPath));
}

void PlayerController::reloadHistory()
{
    m_history->reload();
    m_continue->reload();
    m_favorites->reload();
}

// One-time import of QSettings recents + resume positions (resume keys are path hashes,
// so only files still in the recent list can be recovered).
void PlayerController::migrateLegacy()
{
    QSettings s;
    if (!m_repo.available() || s.value(QStringLiteral("history/migrated")).toBool())
        return;
    qint64 t = QDateTime::currentSecsSinceEpoch();
    for (const QString &path : std::as_const(m_recentFiles)) {
        if (path.contains(QStringLiteral("://")))
            continue;
        HistoryEntry e;
        e.path = path;
        e.title = QFileInfo(path).completeBaseName();
        e.position = s.value(resumeKey(path), 0.0).toDouble();
        e.lastPlayed = t--; // preserves recents order
        e.playCount = 1;
        m_repo.importEntry(e);
    }
    s.setValue(QStringLiteral("history/migrated"), true);
}

void PlayerController::toggleFavorite(const QString &path)
{
    m_repo.setFavorite(path, !m_repo.entry(path).favorite);
    reloadHistory();
}

void PlayerController::openHistory(const QString &path)
{
    openFiles({QUrl::fromLocalFile(path)});
}
