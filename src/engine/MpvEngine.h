#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantMap>
#include <QVariantList>

#include <mpv/client.h>

// Thin wrapper around the libmpv client API.
//
// Owns the mpv_handle, converts mpv events into Qt signals, and exposes an
// mpv-free API surface to the layers above. All commands/state changes happen
// on the GUI thread; mpv internally decodes on its own threads.
//
// NOTE: This class must not leak mpv types through its public API — callers
// above Media Engine must remain mpv-agnostic (architecture rule).
class MpvEngine : public QObject
{
    Q_OBJECT

    // True while media is loaded and not paused.
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged FINAL)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged FINAL)
    // True when mpv has no file loaded ("idle").
    Q_PROPERTY(bool idle READ idle NOTIFY idleChanged FINAL)
    Q_PROPERTY(double position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged FINAL)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged FINAL)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged FINAL)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged FINAL)
    Q_PROPERTY(QString mediaTitle READ mediaTitle NOTIFY mediaTitleChanged FINAL)
    Q_PROPERTY(QString mediaError READ mediaError NOTIFY mediaErrorChanged FINAL)
    Q_PROPERTY(QVariantList audioTracks READ audioTracks NOTIFY tracksChanged FINAL)
    Q_PROPERTY(QVariantList videoTracks READ videoTracks NOTIFY tracksChanged FINAL)
    Q_PROPERTY(QVariantList subtitleTracks READ subtitleTracks NOTIFY tracksChanged FINAL)
    // [{title, time}] from the container; empty when the file has no chapters.
    Q_PROPERTY(QVariantList chapters READ chapters NOTIFY chaptersChanged FINAL)
    // Currently selected track ids (0 = none/off), derived from track-list.
    Q_PROPERTY(int audioTrackId READ audioTrackId NOTIFY tracksChanged FINAL)
    Q_PROPERTY(int subtitleTrackId READ subtitleTrackId NOTIFY tracksChanged FINAL)
    // Seconds; positive delays the subtitles/audio.
    Q_PROPERTY(double subtitleDelay READ subtitleDelay WRITE setSubtitleDelay NOTIFY subtitleDelayChanged FINAL)
    Q_PROPERTY(double audioDelay READ audioDelay WRITE setAudioDelay NOTIFY audioDelayChanged FINAL)
    // Read-only decoder/render statistics; refreshed ~2x/s only while statsEnabled.
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY statsChanged FINAL)
    Q_PROPERTY(bool statsEnabled READ statsEnabled WRITE setStatsEnabled NOTIFY statsEnabledChanged FINAL)

public:
    explicit MpvEngine(QObject *parent = nullptr);
    ~MpvEngine() override;

    mpv_handle *handle() const { return m_mpv; }

    bool playing() const;
    bool paused() const { return m_paused; }
    bool idle() const { return m_idle; }
    double position() const { return m_position; }
    double duration() const { return m_duration; }
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    double speed() const { return m_speed; }
    QString mediaTitle() const { return m_mediaTitle; }
    QString mediaError() const { return m_mediaError; }
    QVariantList audioTracks() const { return m_audioTracks; }
    QVariantList videoTracks() const { return m_videoTracks; }
    QVariantList subtitleTracks() const { return m_subtitleTracks; }
    QVariantList chapters() const { return m_chapters; }
    int audioTrackId() const { return selectedId(m_audioTracks); }
    int subtitleTrackId() const { return selectedId(m_subtitleTracks); }
    double subtitleDelay() const { return m_subDelay; }
    double audioDelay() const { return m_audioDelay; }
    QVariantMap stats() const { return m_stats; }
    bool statsEnabled() const { return m_statsTimer.isActive(); }
    void setStatsEnabled(bool enabled);

public slots:
    void setPaused(bool paused);
    void setVolume(int volume);
    void setMuted(bool muted);
    void setSpeed(double speed);
    void setSubtitleDelay(double seconds);
    void setAudioDelay(double seconds);

public:
    // QML-facing playback control.
    Q_INVOKABLE void loadFile(const QUrl &url);
    Q_INVOKABLE void loadFiles(const QVariantList &urls);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void stop();
    // Absolute seek in seconds (accurate, decodes to the exact target).
    Q_INVOKABLE void seek(double seconds, bool accurate = true);
    // Relative seek; negative values seek backwards.
    Q_INVOKABLE void seekRelative(double seconds);
    // Step one frame; dir < 0 steps backwards (where mpv supports it).
    Q_INVOKABLE void stepFrame(int direction);
    Q_INVOKABLE void cycleAudioTrack();
    Q_INVOKABLE void cycleSubtitleTrack();
    Q_INVOKABLE void setSubtitlesVisible(bool visible);
    Q_INVOKABLE void seekChapter(int index);
    Q_INVOKABLE void setAudioTrack(int id);
    // id 0 turns subtitles off.
    Q_INVOKABLE void setSubtitleTrack(int id);
    Q_INVOKABLE void loadExternalSubtitle(const QUrl &url);
    // mode: "auto-safe" | "no" | "auto". Applies to the current and next files.
    Q_INVOKABLE void setHwdec(const QString &mode);
    // Subtitle styling, applied live. color is "#RRGGBB"; size/border in mpv scaled pixels,
    // pos is 0 (top) .. 100 (bottom).
    Q_INVOKABLE void setSubtitleFontSize(int size);
    Q_INVOKABLE void setSubtitleColor(const QString &color);
    Q_INVOKABLE void setSubtitleBorderSize(double size);
    Q_INVOKABLE void setSubtitlePosition(int percent);

signals:
    void playingChanged();
    void pausedChanged();
    void idleChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void mutedChanged();
    void speedChanged();
    void mediaTitleChanged();
    void mediaErrorChanged();
    void tracksChanged();
    void chaptersChanged();
    // Emitted when a file played to its natural end (not stopped/replaced/errored).
    void endOfFile();
    void subtitleDelayChanged();
    void audioDelayChanged();
    void statsChanged();
    void statsEnabledChanged();

private:
    // Drains the mpv event queue; runs on the GUI thread via the wakeup callback.
    void processEvents();
    // mpv calls this from an internal thread; re-dispatches to the GUI thread.
    static void onWakeup(void *ctx)
    {
        auto *self = static_cast<MpvEngine *>(ctx);
        QMetaObject::invokeMethod(self, &MpvEngine::processEvents, Qt::QueuedConnection);
    }

    // Sends a command array; reports failures through mediaError.
    bool command(const QStringList &args);
    void setMediaError(const QString &message);
    void checkEndOfFile();

    static QVariant nodeToVariant(const mpv_node &node);
    void refreshTrackLists();
    static int selectedId(const QVariantList &tracks);
    void refreshStats();
    QVariant getProp(const char *name) const;

    mpv_handle *m_mpv = nullptr;

    // Cached state mirrored from observed mpv properties.
    bool m_paused = true;
    bool m_idle = true;
    bool m_eof = false;
    double m_position = 0.0;
    double m_duration = 0.0;
    int m_volume = 100;
    bool m_muted = false;
    double m_speed = 1.0;
    QString m_mediaTitle;
    QString m_mediaError;
    QVariantList m_audioTracks;
    QVariantList m_videoTracks;
    QVariantList m_subtitleTracks;
    QVariantList m_chapters;
    double m_subDelay = 0.0;
    double m_audioDelay = 0.0;
    QVariantMap m_stats;
    QTimer m_statsTimer;
};
