#pragma once

#include <QObject>
#include <QUrl>
#include <QStringList>
#include <QVariant>
#include <QVariantList>

#include "core/PlaylistModel.h"
#include "engine/MpvEngine.h"
#include "storage/Database.h"
#include "storage/HistoryModel.h"
#include "storage/HistoryRepository.h"

#include <memory>

// Application Core layer: the single object QML talks to. Holds playback
// state that is presentation-relevant and forwards commands to the media
// engine. Must not expose mpv types or contain playback logic itself.
class PlayerController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(MpvEngine *engine READ engine CONSTANT FINAL)
    Q_PROPERTY(PlaylistModel *playlist READ playlist CONSTANT FINAL)
    // Optional library (SQLite). Models are empty and historyAvailable is false if the DB can't open.
    Q_PROPERTY(HistoryModel *history READ history CONSTANT FINAL)
    Q_PROPERTY(HistoryModel *continueWatching READ continueWatching CONSTANT FINAL)
    Q_PROPERTY(HistoryModel *favorites READ favorites CONSTANT FINAL)
    Q_PROPERTY(bool historyAvailable READ historyAvailable CONSTANT FINAL)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY hasMediaChanged FINAL)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged FINAL)
    Q_PROPERTY(int repeatMode READ repeatMode WRITE setRepeatMode NOTIFY repeatModeChanged FINAL)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged FINAL)

public:
    explicit PlayerController(QObject *parent = nullptr);

    MpvEngine *engine() const { return m_engine; }
    PlaylistModel *playlist() const { return m_playlist; }
    bool hasMedia() const;
    HistoryModel *history() const { return m_history; }
    HistoryModel *continueWatching() const { return m_continue; }
    HistoryModel *favorites() const { return m_favorites; }
    bool historyAvailable() const { return m_repo.available(); }
    QStringList recentFiles() const { return m_recentFiles; }
    bool shuffle() const { return m_shuffle; }
    void setShuffle(bool on);
    int repeatMode() const { return m_repeat; }
    void setRepeatMode(int mode); // 0 off, 1 all, 2 one

public slots:
    // Accepts a JS array of URLs (from file dialogs or drag & drop).
    void openFiles(const QVariantList &urls);
    // Local path -> properly escaped file URL (paths with spaces, #, % ...).
    Q_INVOKABLE QUrl fileUrl(const QString &path) const { return QUrl::fromLocalFile(path); }
    // Appends to the queue; starts playing the first one if nothing is current.
    void enqueue(const QVariantList &urls);
    void playIndex(int index);
    // M3U playlists: save the queue, or replace the queue and play from the start.
    bool savePlaylist(const QUrl &file);
    bool loadPlaylist(const QUrl &file);
    void next();
    void previous();
    void clearRecentFiles();
    void toggleFavorite(const QString &path);
    void openHistory(const QString &path); // opens and resumes
    void saveResume();                      // persist position (timer, file switch, quit)

    // Generic persisted UI settings (QSettings-backed), e.g. window geometry.
    QVariant setting(const QString &key, const QVariant &fallback = {}) const;
    void setSetting(const QString &key, const QVariant &value);

signals:
    void hasMediaChanged();
    void recentFilesChanged();
    void shuffleChanged();
    void repeatModeChanged();

private:
    void advance(bool autoAdvance);
    void rememberFile(const QUrl &url);
    // Resume support: remember where each local file stopped and seek back on reopen.
    static QString resumeKey(const QString &path);
    void reloadHistory();
    void migrateLegacy();

    MpvEngine *m_engine = nullptr;
    PlaylistModel *m_playlist = nullptr;
    QString m_currentPath;
    double m_resumeTarget = 0.0;
    QStringList m_recentFiles;
<<<<<<< HEAD
    bool m_shuffle = false;
    int m_repeat = 0;
=======
    std::unique_ptr<Database> m_database;
    HistoryRepository m_repo{nullptr};
    HistoryModel *m_history = nullptr;
    HistoryModel *m_continue = nullptr;
    HistoryModel *m_favorites = nullptr;
>>>>>>> main
};
