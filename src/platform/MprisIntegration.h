#pragma once

#include "Mpris.h"
#include "PlatformIntegration.h"

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QStringList>

class MprisIntegration;

// Keeps the screen awake while video plays. Tries org.freedesktop.ScreenSaver,
// then the XDG portal, then org.gnome.SessionManager. Async: never blocks the UI thread.
class ScreenInhibitor : public QObject
{
    Q_OBJECT
public:
    explicit ScreenInhibitor(QObject *parent = nullptr) : QObject(parent) {}
    ~ScreenInhibitor() override { setInhibited(false); }
    void setInhibited(bool on);

private:
    void tryBackend(int backend);
    void release();

    bool m_wanted = false;
    bool m_pending = false;
    int m_backend = -1; // backend holding an active inhibit, -1 none
    quint32 m_cookie = 0;
    QString m_handle; // portal request object path
};

// org.mpris.MediaPlayer2 (root)
class MprisRootAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ canQuit)
    Q_PROPERTY(bool CanRaise READ canQuit)
    Q_PROPERTY(bool HasTrackList READ canQuit)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ schemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ mimeTypes)
public:
    explicit MprisRootAdaptor(QObject *parent) : QDBusAbstractAdaptor(parent) {}
    bool canQuit() const { return false; }
    QString identity() const { return QStringLiteral("Lumina Player"); }
    QString desktopEntry() const { return QStringLiteral("io.github.mdmakky.Lumina"); }
    QStringList schemes() const { return {QStringLiteral("file"), QStringLiteral("http"), QStringLiteral("https")}; }
    QStringList mimeTypes() const { return {QStringLiteral("video/mp4"), QStringLiteral("video/x-matroska"), QStringLiteral("audio/mpeg")}; }
public slots:
    void Raise() {}
    void Quit() {}
};

// org.mpris.MediaPlayer2.Player
class MprisPlayerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(double Rate READ rate WRITE setRate)
    Q_PROPERTY(double MinimumRate READ rate)
    Q_PROPERTY(double MaximumRate READ rate)
    Q_PROPERTY(qlonglong Position READ position)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanPlay READ always)
    Q_PROPERTY(bool CanPause READ always)
    Q_PROPERTY(bool CanSeek READ always)
    Q_PROPERTY(bool CanControl READ always)
public:
    explicit MprisPlayerAdaptor(MprisIntegration *owner);
    QString playbackStatus() const;
    QVariantMap metadata() const;
    double volume() const;
    void setVolume(double v);
    double rate() const;
    void setRate(double) {} // ponytail: speed stays under app control; add when a client needs it
    qlonglong position() const;
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool always() const { return true; }

public slots:
    void Play();
    void Pause();
    void PlayPause();
    void Stop();
    void Next();
    void Previous();
    void Seek(qlonglong offsetMicros);
    void SetPosition(const QDBusObjectPath &trackId, qlonglong positionMicros);
    void OpenUri(const QString &uri);

signals:
    void Seeked(qlonglong positionMicros);

private:
    MprisIntegration *m_owner;
};

class MprisIntegration : public PlatformIntegration
{
    Q_OBJECT
public:
    explicit MprisIntegration(QObject *parent = nullptr);
    void update(const MediaState &state) override;
    void seeked(double position) override;
    const MediaState &state() const { return m_state; }

private:
    friend class MprisPlayerAdaptor;
    void emitPropertiesChanged(const QVariantMap &changed);

    MediaState m_state;
    MprisPlayerAdaptor *m_player = nullptr;
    ScreenInhibitor *m_inhibitor = nullptr;
    bool m_connected = false;
};
