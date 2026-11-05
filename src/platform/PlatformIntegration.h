#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

// Snapshot of what the OS should know about playback. Pure data, no OS types.
struct MediaState {
    enum Status { Stopped, Playing, Paused };
    Status status = Stopped;
    QString title;
    QUrl url;
    double duration = 0.0; // seconds
    double position = 0.0; // seconds
    double volume = 1.0;   // 0..1
    double rate = 1.0;
    bool hasVideo = false;
    bool canNext = false;
    bool canPrevious = false;

    bool operator==(const MediaState &) const = default;
};

// OS integration boundary (media keys / now-playing widgets, sleep inhibition).
// One implementation per OS, chosen at compile time in create(); the rest of
// the app only sees this interface and its signals.
class PlatformIntegration : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    // Never null; a no-op implementation is returned where the OS has no backend.
    static PlatformIntegration *create(QObject *parent);

    // Called on every relevant state change (position ticks included).
    virtual void update(const MediaState &state) = 0;
    // Position jumped because of a seek (MPRIS Seeked).
    virtual void seeked(double) {}

signals:
    void playRequested();
    void pauseRequested();
    void playPauseRequested();
    void stopRequested();
    void nextRequested();
    void previousRequested();
    void seekByRequested(double seconds);
    void seekToRequested(double seconds);
    void openUriRequested(const QUrl &url);
    void volumeRequested(double volume); // 0..1
};
