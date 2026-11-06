#pragma once

#include <QImage>
#include <QString>

struct mpv_handle;

// A second, lightweight libmpv handle (no video/audio output, no audio/subs,
// keyframe-only seeks, tiny cache) that grabs small frames for the seek-bar
// preview. Not thread-safe: use from one worker thread.
class MpvThumbnailer
{
public:
    MpvThumbnailer() = default;
    ~MpvThumbnailer();

    // coverArt: show the embedded picture of audio files; width: scaled frame width (default 240).
    bool open(const QString &path, bool coverArt = false, int width = 240);
    double duration() const; // seconds, 0 if unknown
    // Keyframe at/before `seconds`, scaled to ~240 px wide; null image on failure.
    QImage grab(double seconds);
    void close();

private:
    bool waitFor(int eventId, int timeoutMs);

    mpv_handle *m_mpv = nullptr;
};
