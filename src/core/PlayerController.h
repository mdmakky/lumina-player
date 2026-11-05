#pragma once

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QVariantList>

#include "core/PlaylistModel.h"
#include "engine/MpvEngine.h"

// Application Core layer: the single object QML talks to. Holds playback
// state that is presentation-relevant and forwards commands to the media
// engine. Must not expose mpv types or contain playback logic itself.
class PlayerController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(MpvEngine *engine READ engine CONSTANT FINAL)
    Q_PROPERTY(PlaylistModel *playlist READ playlist CONSTANT FINAL)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY hasMediaChanged FINAL)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged FINAL)

public:
    explicit PlayerController(QObject *parent = nullptr);

    MpvEngine *engine() const { return m_engine; }
    PlaylistModel *playlist() const { return m_playlist; }
    bool hasMedia() const;
    QStringList recentFiles() const { return m_recentFiles; }

public slots:
    // Accepts a JS array of URLs (from file dialogs or drag & drop).
    void openFiles(const QVariantList &urls);
    // Appends to the queue; starts playing the first one if nothing is current.
    void enqueue(const QVariantList &urls);
    void playIndex(int index);
    // M3U playlists: save the queue, or replace the queue and play from the start.
    bool savePlaylist(const QUrl &file);
    bool loadPlaylist(const QUrl &file);
    void next();
    void previous();
    void clearRecentFiles();
    // Saves a PNG of the current frame to Pictures/Lumina; returns the path ("" on failure).
    QString takeScreenshot();

    // Generic persisted UI settings (QSettings-backed), e.g. window geometry.
    QVariant setting(const QString &key, const QVariant &fallback = {}) const;
    void setSetting(const QString &key, const QVariant &value);

signals:
    // Transient user-facing message; the UI shows it as a toast.
    void toast(const QString &text);
    void hasMediaChanged();
    void recentFilesChanged();

private:
    void rememberFile(const QUrl &url);
    // Resume support: remember where each local file stopped and seek back on reopen.
    static QString resumeKey(const QString &path);
    void saveResume();

    MpvEngine *m_engine = nullptr;
    PlaylistModel *m_playlist = nullptr;
    QString m_currentPath;
    double m_resumeTarget = 0.0;
    QStringList m_recentFiles;
};
