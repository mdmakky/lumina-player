#pragma once

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QStringList>
#include <QVariant>
#include <QVariantList>

#include "core/PlaylistModel.h"
#include "engine/MpvEngine.h"
#include "storage/Database.h"
#include "storage/HistoryModel.h"
#include "storage/HistoryRepository.h"
#include "storage/LibraryModel.h"
#include "services/LibraryScanner.h"
#include "services/LibraryProber.h"
#include "storage/PlaylistsModel.h"
#include "media/ThumbnailService.h"

#include <memory>

// Application Core layer: the single object QML talks to. Holds playback
// state that is presentation-relevant and forwards commands to the media
// engine. Must not expose mpv types or contain playback logic itself.
class PlayerController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(MpvEngine *engine READ engine CONSTANT FINAL)
    Q_PROPERTY(PlaylistModel *playlist READ playlist CONSTANT FINAL)
    // Seek-bar hover preview frames (disabled automatically when not applicable).
    Q_PROPERTY(ThumbnailService *thumbs READ thumbs CONSTANT FINAL)
    // Optional library (SQLite). Models are empty and historyAvailable is false if the DB can't open.
    Q_PROPERTY(HistoryModel *history READ history CONSTANT FINAL)
    Q_PROPERTY(HistoryModel *continueWatching READ continueWatching CONSTANT FINAL)
    Q_PROPERTY(HistoryModel *favorites READ favorites CONSTANT FINAL)
    Q_PROPERTY(bool historyAvailable READ historyAvailable CONSTANT FINAL)
    Q_PROPERTY(LibraryModel *library READ library CONSTANT FINAL)
    Q_PROPERTY(QStringList libraryFolders READ libraryFolders NOTIFY libraryFoldersChanged FINAL)
    Q_PROPERTY(bool libraryScanning READ libraryScanning NOTIFY libraryScanningChanged FINAL)
    Q_PROPERTY(int libraryProgress READ libraryProgress NOTIFY libraryProgressChanged FINAL)
    Q_PROPERTY(bool libraryAvailable READ libraryAvailable CONSTANT FINAL)
    Q_PROPERTY(PlaylistsModel *libraryPlaylists READ libraryPlaylists CONSTANT FINAL)
    Q_PROPERTY(bool libraryProbing READ libraryProbing NOTIFY libraryProbingChanged FINAL) // reading metadata in the background
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY hasMediaChanged FINAL)
    // OS output volume 0..100, or -1 when the OS backend cannot report it (the UI then drives the engine volume).
    Q_PROPERTY(int systemVolume READ systemVolume NOTIFY systemVolumeChanged FINAL)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged FINAL)
    Q_PROPERTY(int repeatMode READ repeatMode WRITE setRepeatMode NOTIFY repeatModeChanged FINAL)
    // Bookmarks of the current file: [{id, time, label}], ordered by time.
    Q_PROPERTY(QVariantList bookmarks READ bookmarks NOTIFY bookmarksChanged FINAL)
    Q_PROPERTY(QString logsPath READ logsPath CONSTANT FINAL)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged FINAL)
    Q_PROPERTY(QStringList recentUrls READ recentUrls NOTIFY recentUrlsChanged FINAL)
    // Per-file video/audio/subtitle profiles (needs the library DB). Setting "playback/rememberFile".
    Q_PROPERTY(bool rememberFileSettings READ rememberFileSettings WRITE setRememberFileSettings NOTIFY rememberFileSettingsChanged FINAL)

public:
    explicit PlayerController(QObject *parent = nullptr);

    MpvEngine *engine() const { return m_engine; }
    int systemVolume() const { return m_systemVolume; }
    void setSystemVolume(int percent); // from the OS backend
    // The one volume entry point for sliders/keys: OS volume when available, else the engine's.
    Q_INVOKABLE void setVolumePercent(int percent);
    PlaylistModel *playlist() const { return m_playlist; }
    ThumbnailService *thumbs() const { return m_thumbs; }
    bool hasMedia() const;
    HistoryModel *history() const { return m_history; }
    HistoryModel *continueWatching() const { return m_continue; }
    HistoryModel *favorites() const { return m_favorites; }
    bool historyAvailable() const { return m_repo.available(); }
    QVariantList bookmarks() const { return m_bookmarks; }
    QString logsPath() const;
    QUrl currentUrl() const { return m_playlist->urlAt(m_playlist->currentIndex()); }
    bool canNext() const { return m_playlist->rowCount() > 1; }
    bool canPrevious() const { return m_playlist->rowCount() > 1; }
    QStringList recentFiles() const { return m_recentFiles; }
    QStringList recentUrls() const { return m_recentUrls; }
    bool rememberFileSettings() const { return m_rememberFile; }
    void setRememberFileSettings(bool on);
    LibraryModel *library() const { return m_library; }
    PlaylistsModel *libraryPlaylists() const { return m_playlists; }
    bool libraryProbing() const { return m_prober && m_prober->running(); }
    QStringList libraryFolders() const { return m_libraryFolders; }
    bool libraryScanning() const { return m_scanner && m_scanner->scanning(); }
    int libraryProgress() const { return m_libraryProgress; }
    bool libraryAvailable() const { return m_libRepo.available(); }
    bool shuffle() const { return m_shuffle; }
    void setShuffle(bool on);
    int repeatMode() const { return m_repeat; }
    void setRepeatMode(int mode); // 0 off, 1 all, 2 one

public slots:
    // Optional media library (folders persisted; scans run on a worker thread).
    void addLibraryFolder(const QUrl &folder);
    void removeLibraryFolder(const QString &folder);
    void rescanLibrary();
    // Replaces the queue with the library's visible rows and plays row `index`.
    void playLibrary(int index);
    // Accepts a JS array of URLs (from file dialogs or drag & drop).
    void openFiles(const QVariantList &urls);
    // Validates and plays a network URL (http/https/rtmp/rtsp/ftp/file); toasts and returns false if invalid.
    Q_INVOKABLE bool openUrl(const QString &text);
    Q_INVOKABLE bool isValidUrl(const QString &text) const;
    Q_INVOKABLE void clearRecentUrls();
    // Forgets and undoes this file's saved profile (adjustments, tracks, delays, speed, volume).
    Q_INVOKABLE void resetFileSettings();
    // Local path (or URL string) -> properly escaped file URL (paths with spaces, #, % ...).
    Q_INVOKABLE QUrl fileUrl(const QString &path) const
    { return path.contains(QStringLiteral("://")) ? QUrl(path) : QUrl::fromLocalFile(path); }
    // Appends to the queue; starts playing the first one if nothing is current.
    void enqueue(const QVariantList &urls);
    void playIndex(int index);
    // M3U playlists: save the queue, or replace the queue and play from the start.
    bool savePlaylist(const QUrl &file);
    bool loadPlaylist(const QUrl &file);
    void addBookmark(const QString &label = {}); // at the current position
    void removeBookmark(qint64 id);
    void next();
    void previous();
    void clearRecentFiles();
    // Saves a PNG of the current frame to Pictures/Lumina; returns the path ("" on failure).
    QString takeScreenshot();
    // Screenshot target folder (setting "video/screenshotDir", default Pictures/Lumina).
    Q_INVOKABLE QString screenshotDir() const;
    // Version/environment strings for Settings > About.
    Q_INVOKABLE QVariantMap aboutInfo() const;
    Q_INVOKABLE void openLogsFolder() const;
    Q_INVOKABLE void setVerboseLogging(bool on);
    void toggleFavorite(const QString &path);
    void openHistory(const QString &path); // opens and resumes
    void saveResume();                      // persist position (timer, file switch, quit)

    // Generic persisted UI settings (QSettings-backed), e.g. window geometry.
    QVariant setting(const QString &key, const QVariant &fallback = {}) const;
    void setSetting(const QString &key, const QVariant &value);

signals:
    // Transient user-facing message; the UI shows it as a toast.
    void toast(const QString &text);
    void hasMediaChanged();
    void systemVolumeChanged();
    void systemVolumeRequested(int percent);
    void bookmarksChanged();
    void recentFilesChanged();
    void recentUrlsChanged();
    void rememberFileSettingsChanged();
    void shuffleChanged();
    void repeatModeChanged();
    void libraryFoldersChanged();
    void libraryScanningChanged();
    void libraryProgressChanged();
    void libraryProbingChanged();

private:
    void reloadBookmarks();
    void rememberUrl(const QUrl &url);
    void markProfileDirty();
    void saveProfileIfDirty();
    void applyProfileTracks();
    void resetFileState();
    QVariantList m_bookmarks;
    QStringList m_recentUrls;
    bool m_rememberFile = true;
    QVariantMap m_pendingProfile; // tracks are restored on fileLoaded (needs the demuxer)
    bool m_hadProfile = false;
    bool m_profileArmed = false; // false while load-time changes settle; they are not user edits
    bool m_profileDirty = false;
    QTimer *m_armTimer = nullptr;
    void advance(bool autoAdvance);
    void rememberFile(const QUrl &url);
    // Resume support: remember where each local file stopped and seek back on reopen.
    static QString resumeKey(const QString &path);
    void reloadHistory();
    void migrateLegacy();

    MpvEngine *m_engine = nullptr;
    int m_systemVolume = -1;
    PlaylistModel *m_playlist = nullptr;
    ThumbnailService *m_thumbs = nullptr;
    QString m_currentPath;
    double m_resumeTarget = 0.0;
    QStringList m_recentFiles;
    bool m_shuffle = false;
    int m_repeat = 0;
    std::unique_ptr<Database> m_database;
    HistoryRepository m_repo{nullptr};
    HistoryModel *m_history = nullptr;
    HistoryModel *m_continue = nullptr;
    HistoryModel *m_favorites = nullptr;
    LibraryRepository m_libRepo{nullptr};
    LibraryModel *m_library = nullptr;
    LibraryScanner *m_scanner = nullptr;
    LibraryProber *m_prober = nullptr;
    PlaylistRepository m_plRepo{nullptr};
    PlaylistsModel *m_playlists = nullptr;
    QStringList m_libraryFolders;
    int m_libraryProgress = 0;
};
