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

    bool open(const QString &path);
    // Keyframe at/before `seconds`, scaled to ~240 px wide; null image on failure.
    QImage grab(double seconds);
    void close();

private:
    bool waitFor(int eventId, int timeoutMs);

    mpv_handle *m_mpv = nullptr;
};
