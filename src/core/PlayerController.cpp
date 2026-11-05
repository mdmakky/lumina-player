#include "PlayerController.h"

#include "engine/MpvEngine.h"
#include "utilities/Logger.h"
#include "utilities/TimeUtils.h"
#include "utilities/UrlUtils.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJSValue>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>

#ifdef Q_OS_WIN
#include "platform/windows/SleepInhibitor_win.h"
#endif
#include <QTimer>
#include <QUrl>

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
    , m_engine(new MpvEngine(this))
    , m_playlist(new PlaylistModel(this))
{
    connect(m_engine, &MpvEngine::endOfFile, this, [this]() {
        if (m_playlist->currentIndex() >= 0 // -1: current entry was removed; don't restart the queue
            && setting(QStringLiteral("playback/autoNext"), true).toBool())
            advance(true);
    });
    connect(m_engine, &MpvEngine::idleChanged, this, &PlayerController::hasMediaChanged);
#ifdef Q_OS_WIN
    // Keep the display awake while video plays (audio-only may sleep the screen).
    auto updateInhibit = [this]() {
        SleepInhibitor::set(m_engine->playing() && !m_engine->videoTracks().isEmpty());
    };
    connect(m_engine, &MpvEngine::pausedChanged, this, updateInhibit);
    connect(m_engine, &MpvEngine::idleChanged, this, updateInhibit);
    connect(m_engine, &MpvEngine::tracksChanged, this, updateInhibit);
#endif

    m_thumbs = new ThumbnailService(this);
    m_thumbs->setEnabled(setting(QStringLiteral("ui/seekPreview"), true).toBool());
    connect(m_engine, &MpvEngine::idleChanged, this, [this] { if (m_engine->idle()) m_thumbs->setSource({}); });
    connect(m_engine, &MpvEngine::tracksChanged, this, [this] {
        bool video = false; // cover art / still images are not worth a preview
        for (const QVariant &t : m_engine->videoTracks()) {
            const QVariantMap m = t.toMap();
            video = video || (!m.value(QStringLiteral("albumart")).toBool() && !m.value(QStringLiteral("image")).toBool());
        }
        m_thumbs->setHasVideo(video);
    });

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

    // Per-file profiles: restore tracks once the file is open; any later change marks the profile dirty.
    m_armTimer = new QTimer(this);
    m_armTimer->setSingleShot(true);
    m_armTimer->setInterval(1000);
    connect(m_armTimer, &QTimer::timeout, this, [this] { m_profileArmed = true; });
    connect(m_engine, &MpvEngine::fileLoaded, this, [this] {
        applyProfileTracks();
        m_armTimer->start();
    });
    for (auto sig : {&MpvEngine::videoOptionsChanged, &MpvEngine::speedChanged, &MpvEngine::subtitleDelayChanged,
                     &MpvEngine::audioDelayChanged, &MpvEngine::volumeChanged, &MpvEngine::tracksChanged})
        connect(m_engine, sig, this, &PlayerController::markProfileDirty);
    // Remote entries start with a URL-derived title; use mpv's media-title once it is known.
    connect(m_engine, &MpvEngine::mediaTitleChanged, this, [this] {
        const QString t = m_engine->mediaTitle();
        if (m_engine->idle() || t.isEmpty() || !m_currentPath.contains(QStringLiteral("://")))
            return;
        m_playlist->setCurrentTitle(t);
        m_repo.setTitle(m_currentPath, t);
        reloadHistory();
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
    m_recentUrls = settings.value(QStringLiteral("recentUrls")).toStringList();
    m_rememberFile = settings.value(QStringLiteral("playback/rememberFile"), true).toBool();
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
    m_engine->setSubtitleLanguages(settings.value(QStringLiteral("subtitles/languages"), QStringLiteral("bn,ben,en,eng")).toString());
    if (settings.contains(QStringLiteral("playback/defaultSpeed")))
        m_engine->setSpeed(settings.value(QStringLiteral("playback/defaultSpeed")).toDouble());
    if (settings.contains(QStringLiteral("video/aspect")))
        m_engine->setAspect(settings.value(QStringLiteral("video/aspect")).toString());
    if (settings.value(QStringLiteral("video/deinterlace"), false).toBool())
        m_engine->setDeinterlace(true);
    if (settings.contains(QStringLiteral("advanced/cacheMiB")))
        m_engine->setCacheSize(settings.value(QStringLiteral("advanced/cacheMiB")).toInt());
    if (settings.contains(QStringLiteral("advanced/readahead")))
        m_engine->setReadahead(settings.value(QStringLiteral("advanced/readahead")).toInt());
    setVerboseLogging(settings.value(QStringLiteral("advanced/verboseLog"), false).toBool());
    // Power-user overrides, one "name=value" per line; bad lines are logged and skipped.
    for (const QString &line : settings.value(QStringLiteral("advanced/mpvOptions")).toString().split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        const QString err = m_engine->setMpvOption(line.left(eq).trimmed(), line.mid(eq + 1).trimmed());
        if (!err.isEmpty())
            qCWarning(lcCore) << "mpv override" << line << "failed:" << err;
    }
    m_engine->setMuted(settings.value(QStringLiteral("playback/muted"), false).toBool());
    m_engine->setEqualizer(settings.value(QStringLiteral("eq/gains"), QVariantList(10, 0.0)).toList());
    m_engine->setEqEnabled(settings.value(QStringLiteral("eq/enabled"), false).toBool());
    connect(m_engine, &MpvEngine::eqChanged, this, [this] {
        setSetting(QStringLiteral("eq/gains"), m_engine->eqGains());
        setSetting(QStringLiteral("eq/enabled"), m_engine->eqEnabled());
    });
    connect(m_engine, &MpvEngine::volumeChanged, this, [this] {
        if (!m_profileArmed) return; // restoring a file's profile must not change the global default
        setSetting(QStringLiteral("playback/volume"), m_engine->volume());
    });
    connect(m_engine, &MpvEngine::mutedChanged, this, [this] {
        setSetting(QStringLiteral("playback/muted"), m_engine->muted());
    });
}

void PlayerController::reloadBookmarks()
{
    m_bookmarks.clear();
    for (const Bookmark &b : m_repo.bookmarks(m_currentPath))
        m_bookmarks.append(QVariantMap{{"id", b.id}, {"time", b.time}, {"label", b.label}});
    emit bookmarksChanged();
}

void PlayerController::addBookmark(const QString &label)
{
    if (m_currentPath.isEmpty() || m_engine->idle())
        return;
    if (!m_repo.available()) {
        emit toast(tr("Bookmarks unavailable"));
        return;
    }
    const double t = m_engine->position();
    if (!m_repo.addBookmark(m_currentPath, t, label))
        return;
    reloadBookmarks();
    emit toast(tr("Bookmark added at %1").arg(Lumina::TimeUtils::formatDuration(t)));
}

void PlayerController::removeBookmark(qint64 id)
{
    m_repo.removeBookmark(id);
    reloadBookmarks();
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
    // QML passes JS objects as QJSValue; QSettings would store an unreadable binary blob
    // on some platforms (Linux warns on restore). Convert to plain QVariantMap/List first.
    if (value.metaType() == QMetaType::fromType<QJSValue>()) {
        QSettings().setValue(key, value.value<QJSValue>().toVariant());
        return;
    }
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
        else if (Lumina::UrlUtils::isRemote(url))
            rememberUrl(url);
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
    const bool remote = Lumina::UrlUtils::isRemote(url);
    m_currentPath = url.isLocalFile() ? url.toLocalFile() : (remote ? url.toString(QUrl::FullyEncoded) : QString());
    m_resumeTarget = 0.0;
    m_profileArmed = false;
    m_profileDirty = false;
    m_armTimer->stop();
    m_pendingProfile.clear();
    if (!m_currentPath.isEmpty() && !qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST")
        && setting(QStringLiteral("playback/resume"), true).toBool()) {
        m_resumeTarget = m_repo.available() ? m_repo.entry(m_currentPath).position
                                            : setting(resumeKey(m_currentPath), 0.0).toDouble();
        m_repo.recordPlay(m_currentPath, remote ? Lumina::UrlUtils::titleFor(url)
                                                : QFileInfo(m_currentPath).completeBaseName());
        reloadHistory();
    }
    if (!m_currentPath.isEmpty() && m_rememberFile && !qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST")) {
        m_pendingProfile = m_repo.profile(m_currentPath);
        if (m_pendingProfile.isEmpty() && m_hadProfile) {
            resetFileState(); // don't leak the previous file's settings into this one
        } else if (!m_pendingProfile.isEmpty()) {
            const QVariantMap &p = m_pendingProfile;
            for (const char *k : {"brightness", "contrast", "saturation", "gamma", "hue"})
                m_engine->setVideoAdjust(QString::fromLatin1(k), p.value(QString::fromLatin1(k)).toInt());
            m_engine->setAspect(p.value("aspect", "auto").toString());
            m_engine->setRotate(p.value("rotate", 0).toInt());
            m_engine->setSpeed(p.value("speed", 1.0).toDouble());
            m_engine->setVolume(p.value("volume", m_engine->volume()).toInt());
            m_engine->setSubtitleDelay(p.value("subDelay", 0.0).toDouble());
            m_engine->setAudioDelay(p.value("audioDelay", 0.0).toDouble());
        }
        m_hadProfile = !m_pendingProfile.isEmpty();
    }
    m_thumbs->setSource(m_currentPath); // "" for network streams -> time-only tooltip
    reloadBookmarks();
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

QString PlayerController::screenshotDir() const
{
    const QString custom = setting(QStringLiteral("video/screenshotDir")).toString();
    return custom.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QStringLiteral("/Lumina")
                            : custom;
}

void PlayerController::setVerboseLogging(bool on)
{
    Logger::setVerbose(on);
}

QString PlayerController::logsPath() const { return Logger::logsPath(); }

void PlayerController::openLogsFolder() const
{
    QDir().mkpath(Logger::logsPath());
    QDesktopServices::openUrl(QUrl::fromLocalFile(Logger::logsPath()));
}

QVariantMap PlayerController::aboutInfo() const
{
    const QString mpvVer = m_engine->mpvProperty(QStringLiteral("mpv-version")).toString();
    return {
        {QStringLiteral("version"), QCoreApplication::applicationVersion()},
        {QStringLiteral("qt"), QString::fromLatin1(qVersion())},
        {QStringLiteral("mpv"), mpvVer.isEmpty() ? tr("unknown") : mpvVer},
        {QStringLiteral("ffmpeg"), m_engine->mpvProperty(QStringLiteral("ffmpeg-version")).toString()},
        {QStringLiteral("decoder"), m_engine->mpvProperty(QStringLiteral("hwdec-current")).toString()},
        {QStringLiteral("renderer"), m_engine->rendererInfo()},
        {QStringLiteral("os"), QSysInfo::prettyProductName() + QLatin1Char(' ') + QSysInfo::currentCpuArchitecture()},
    };
}

QString PlayerController::takeScreenshot()
{
    const QString dir = screenshotDir();
    const QString ext = setting(QStringLiteral("video/screenshotFormat"), QStringLiteral("png")).toString() == QLatin1String("jpg")
                            ? QStringLiteral(".jpg") : QStringLiteral(".png");
    const QString path = dir + QStringLiteral("/Lumina-")
                         + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) + ext;
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
    saveProfileIfDirty();
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
    openFiles({fileUrl(path)});
}

void PlayerController::setRememberFileSettings(bool on)
{
    if (on == m_rememberFile)
        return;
    m_rememberFile = on;
    setSetting(QStringLiteral("playback/rememberFile"), on);
    emit rememberFileSettingsChanged();
}

void PlayerController::markProfileDirty()
{
    if (m_profileArmed && m_rememberFile && !m_currentPath.isEmpty())
        m_profileDirty = true;
}

void PlayerController::saveProfileIfDirty()
{
    if (!m_profileDirty || !m_rememberFile || m_currentPath.isEmpty() || m_engine->idle())
        return;
    m_profileDirty = false;
    QVariantMap p = m_engine->videoAdjust();
    p.insert("aspect", m_engine->aspect());
    p.insert("rotate", m_engine->rotate());
    p.insert("speed", m_engine->speed());
    p.insert("volume", m_engine->volume());
    p.insert("subDelay", m_engine->subtitleDelay());
    p.insert("audioDelay", m_engine->audioDelay());
    auto describe = [&](const QVariantList &tracks, int id, const char *idKey, const char *langKey) {
        p.insert(idKey, id);
        for (const QVariant &t : tracks)
            if (t.toMap().value("id").toInt() == id)
                p.insert(langKey, t.toMap().value("lang").toString());
    };
    describe(m_engine->audioTracks(), m_engine->audioTrackId(), "aid", "alang");
    describe(m_engine->subtitleTracks(), m_engine->subtitleTrackId(), "sid", "slang");
    m_repo.saveProfile(m_currentPath, p);
    m_hadProfile = true;
}

// Prefers the same language, falls back to the same id; sid 0 means subtitles were off.
void PlayerController::applyProfileTracks()
{
    if (m_pendingProfile.isEmpty())
        return;
    const QVariantMap p = m_pendingProfile;
    m_pendingProfile.clear();
    auto pick = [&](const QVariantList &tracks, int id, const QString &lang) {
        int byId = 0;
        for (const QVariant &t : tracks) {
            const QVariantMap m = t.toMap();
            if (!lang.isEmpty() && m.value("lang").toString() == lang)
                return m.value("id").toInt();
            if (m.value("id").toInt() == id)
                byId = id;
        }
        return byId;
    };
    if (p.value("aid", 0).toInt() > 0) {
        if (const int a = pick(m_engine->audioTracks(), p.value("aid").toInt(), p.value("alang").toString()))
            m_engine->setAudioTrack(a);
    }
    if (p.contains("sid")) {
        const int sid = p.value("sid").toInt();
        if (sid == 0)
            m_engine->setSubtitleTrack(0);
        else if (const int s = pick(m_engine->subtitleTracks(), sid, p.value("slang").toString()))
            m_engine->setSubtitleTrack(s);
    }
}

// Back to the user's defaults (settings), as if the file had never been touched.
void PlayerController::resetFileState()
{
    m_engine->resetVideoAdjust();
    m_engine->setAspect(setting(QStringLiteral("video/aspect"), QStringLiteral("auto")).toString());
    m_engine->setRotate(0);
    m_engine->setSpeed(setting(QStringLiteral("playback/defaultSpeed"), 1.0).toDouble());
    m_engine->setVolume(setting(QStringLiteral("playback/defaultVolume"), 100).toInt());
    m_engine->setSubtitleDelay(0.0);
    m_engine->setAudioDelay(0.0);
    m_engine->setMpvOption(QStringLiteral("aid"), QStringLiteral("auto"));
    m_engine->setMpvOption(QStringLiteral("sid"), QStringLiteral("auto"));
}

void PlayerController::resetFileSettings()
{
    if (m_currentPath.isEmpty() || m_engine->idle())
        return;
    m_repo.clearProfile(m_currentPath);
    m_profileDirty = false;
    m_hadProfile = false;
    resetFileState();
    emit toast(tr("Settings for this file reset"));
}

bool PlayerController::isValidUrl(const QString &text) const
{
    return Lumina::UrlUtils::parseMediaUrl(text).isValid();
}

bool PlayerController::openUrl(const QString &text)
{
    const QUrl url = Lumina::UrlUtils::parseMediaUrl(text);
    if (!url.isValid()) {
        emit toast(tr("That is not a valid media URL"));
        return false;
    }
    openFiles({url});
    return true;
}

void PlayerController::rememberUrl(const QUrl &url)
{
    const QString s = url.toString(QUrl::FullyEncoded);
    m_recentUrls.removeAll(s);
    m_recentUrls.prepend(s);
    while (m_recentUrls.size() > 10)
        m_recentUrls.removeLast();
    setSetting(QStringLiteral("recentUrls"), m_recentUrls);
    emit recentUrlsChanged();
}

void PlayerController::clearRecentUrls()
{
    m_recentUrls.clear();
    setSetting(QStringLiteral("recentUrls"), m_recentUrls);
    emit recentUrlsChanged();
}
