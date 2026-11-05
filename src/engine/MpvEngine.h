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
    // A-B loop points in seconds; -1 when unset.
    Q_PROPERTY(double loopA READ loopA NOTIFY loopChanged FINAL)
    Q_PROPERTY(double loopB READ loopB NOTIFY loopChanged FINAL)
    // Video presentation state ("auto" aspect = file's own; rotate in degrees).
    Q_PROPERTY(QString aspect READ aspect NOTIFY videoOptionsChanged FINAL)
    Q_PROPERTY(int rotate READ rotate NOTIFY videoOptionsChanged FINAL)
    Q_PROPERTY(bool deinterlace READ deinterlace NOTIFY videoOptionsChanged FINAL)
    // {brightness, contrast, saturation, gamma, hue} each -100..100.
    Q_PROPERTY(QVariantMap videoAdjust READ videoAdjust NOTIFY videoOptionsChanged FINAL)
    // 10-band graphic equalizer (31 Hz .. 16 kHz), gains in dB (-12..12).
    Q_PROPERTY(bool eqEnabled READ eqEnabled WRITE setEqEnabled NOTIFY eqChanged FINAL)
    Q_PROPERTY(QVariantList eqGains READ eqGains NOTIFY eqChanged FINAL)

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
    double loopA() const { return m_loopA; }
    double loopB() const { return m_loopB; }
    QString aspect() const { return m_aspect; }
    int rotate() const { return m_rotate; }
    bool deinterlace() const { return m_deinterlace; }
    QVariantMap videoAdjust() const { return m_adjust; }

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
    // Seeks are coalesced: while one is in flight only the latest target is kept.
    Q_INVOKABLE void seek(double seconds, bool accurate = true);
    // Keyframe-only seek for timeline scrubbing (fast); finish with seek() on release.
    Q_INVOKABLE void seekPreview(double seconds);
    // Relative seek; negative values seek backwards. Accumulates while held.
    Q_INVOKABLE void seekRelative(double seconds);
    // True while a seek is in flight or queued.
    bool seeking() const { return m_seekInFlight || m_hasPending; }
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
    // A-B loop: A/B take the current position; clearLoop removes both.
    Q_INVOKABLE void setLoopA();
    Q_INVOKABLE void setLoopB();
    Q_INVOKABLE void clearLoop();
    // "auto" (file default) or "W:H".
    Q_INVOKABLE void setAspect(const QString &ratio);
    Q_INVOKABLE void setRotate(int degrees);
    Q_INVOKABLE void setDeinterlace(bool on);
    // name: brightness|contrast|saturation|gamma|hue; value -100..100.
    Q_INVOKABLE void setVideoAdjust(const QString &name, int value);
    Q_INVOKABLE void resetVideoAdjust();
    // Saves the current video frame (no subtitles/OSD) as an image file.
    Q_INVOKABLE bool screenshot(const QString &path);
    Q_INVOKABLE void setHwdec(const QString &mode);
    // Subtitle styling, applied live. color is "#RRGGBB"; size/border in mpv scaled pixels,
    // pos is 0 (top) .. 100 (bottom).
    Q_INVOKABLE void setSubtitleFontSize(int size);
    Q_INVOKABLE void setSubtitleColor(const QString &color);
    Q_INVOKABLE void setSubtitleBorderSize(double size);
    Q_INVOKABLE void setSubtitlePosition(int percent);
    Q_INVOKABLE void setEqualizer(const QVariantList &gains); // 10 values, dB
    // Preferred subtitle languages, e.g. "bn,ben,en,eng" (used when a file is loaded).
    Q_INVOKABLE void setSubtitleLanguages(const QString &langs);
    // Demuxer cache: total size in MiB (half is kept for backward seeks) and read-ahead seconds.
    Q_INVOKABLE void setCacheSize(int mib);
    Q_INVOKABLE void setReadahead(int seconds);
    // Advanced override: sets any mpv property; returns "" on success or mpv's error text.
    Q_INVOKABLE QString setMpvOption(const QString &name, const QString &value);
    Q_INVOKABLE QVariant mpvProperty(const QString &name) const { return getProp(name.toUtf8().constData()); }
    // Human-readable scene-graph renderer (set once the GL context exists).
    QString rendererInfo() const { return m_rendererInfo; }
    void setRendererInfo(const QString &info) { m_rendererInfo = info; }
    void setEqEnabled(bool on);
    bool eqEnabled() const { return m_eqEnabled; }
    QVariantList eqGains() const { return m_eqGains; }

signals:
    void eqChanged();
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
    void loopChanged();
    void videoOptionsChanged();
    // mpv finished a seek (MPV_EVENT_PLAYBACK_RESTART); also fires after file load.
    void seekFinished();

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
    void issueSeek(double seconds, bool accurate);
    void setMediaError(const QString &message);
    void checkEndOfFile();
    void applyEq();
    bool m_eqEnabled = false;
    QVariantList m_eqGains = QVariantList(10, 0.0);
    QString m_rendererInfo;

    static QVariant nodeToVariant(const mpv_node &node);
    void refreshTrackLists();
    static int selectedId(const QVariantList &tracks);
    void refreshStats();
    QVariant getProp(const char *name) const;

    mpv_handle *m_mpv = nullptr;

    // Seek coalescing state.
    bool m_coalesceSeeks = true;
    bool m_seekInFlight = false;
    bool m_hasPending = false;
    double m_pendingTarget = 0.0;
    bool m_pendingExact = true;
    double m_lastTarget = 0.0;
    QTimer m_seekWatchdog; // clears a stuck in-flight flag if no restart arrives

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
    double m_loopA = -1.0;
    double m_loopB = -1.0;
    QString m_aspect = QStringLiteral("auto");
    int m_rotate = 0;
    bool m_deinterlace = false;
    QVariantMap m_adjust{{"brightness", 0}, {"contrast", 0}, {"saturation", 0}, {"gamma", 0}, {"hue", 0}};
};
