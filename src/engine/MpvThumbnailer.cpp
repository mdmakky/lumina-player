#include "MpvThumbnailer.h"

#include <QByteArray>
#include <QElapsedTimer>

#include <mpv/client.h>

#include <clocale>
#include <string>

MpvThumbnailer::~MpvThumbnailer() { close(); }

void MpvThumbnailer::close()
{
    if (m_mpv) {
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
    }
}

bool MpvThumbnailer::waitFor(int eventId, int timeoutMs)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < timeoutMs) {
        const mpv_event *e = mpv_wait_event(m_mpv, 0.05);
        if (e->event_id == eventId)
            return true;
        if (e->event_id == MPV_EVENT_SHUTDOWN
            || (e->event_id == MPV_EVENT_END_FILE
                && static_cast<mpv_event_end_file *>(e->data)->reason != MPV_END_FILE_REASON_STOP))
            return false;
    }
    return false;
}

double MpvThumbnailer::duration() const
{
    double d = 0;
    if (m_mpv && mpv_get_property(m_mpv, "duration", MPV_FORMAT_DOUBLE, &d) >= 0)
        return d;
    return 0;
}

bool MpvThumbnailer::open(const QString &path, bool coverArt, int width)
{
    close();
    std::setlocale(LC_NUMERIC, "C");
    m_mpv = mpv_create();
    if (!m_mpv)
        return false;
    const char *opts[][2] = {
        {"vo", "null"}, {"ao", "null"}, {"hwdec", "no"}, {"aid", "no"}, {"sid", "no"},
        {"hr-seek", "no"}, {"pause", "yes"}, {"load-scripts", "no"}, {"config", "no"},
        {"osc", "no"}, {"input-default-bindings", "no"}, {"audio-display", "no"},
        {"cache", "no"}, {"demuxer-max-bytes", "4MiB"}, {"demuxer-max-back-bytes", "1MiB"},
        {"demuxer-readahead-secs", "0"}, {"vd-lavc-threads", "2"}, {"vf", "scale=240:-2"},
        {"terminal", "no"}, {"msg-level", "all=no"},
    };
    for (const auto &o : opts)
        mpv_set_option_string(m_mpv, o[0], o[1]);
    if (width != 240)
        mpv_set_option_string(m_mpv, "vf", ("scale=" + std::to_string(width) + ":-2").c_str());
    if (coverArt) // music: the "video" is the embedded cover picture
        mpv_set_option_string(m_mpv, "audio-display", "embedded-first");
    if (mpv_initialize(m_mpv) < 0) {
        close();
        return false;
    }
    const QByteArray p = path.toUtf8();
    const char *cmd[] = {"loadfile", p.constData(), nullptr};
    if (mpv_command(m_mpv, cmd) < 0 || !waitFor(MPV_EVENT_FILE_LOADED, 8000)
        || !waitFor(MPV_EVENT_PLAYBACK_RESTART, 8000)) {
        close();
        return false;
    }
    return true;
}

QImage MpvThumbnailer::grab(double seconds)
{
    if (!m_mpv)
        return {};
    const QByteArray t = QByteArray::number(seconds, 'f', 3);
    const char *seek[] = {"seek", t.constData(), "absolute+keyframes", nullptr};
    if (mpv_command(m_mpv, seek) < 0 || !waitFor(MPV_EVENT_PLAYBACK_RESTART, 4000))
        return {};

    const char *shot[] = {"screenshot-raw", "video", nullptr};
    mpv_node res{};
    if (mpv_command_ret(m_mpv, shot, &res) < 0)
        return {};
    QImage img;
    if (res.format == MPV_FORMAT_NODE_MAP) {
        int w = 0, h = 0, stride = 0;
        const mpv_byte_array *data = nullptr;
        QByteArray fmt;
        for (int i = 0; i < res.u.list->num; ++i) {
            const QByteArray k = res.u.list->keys[i];
            const mpv_node &v = res.u.list->values[i];
            if (k == "w") w = int(v.u.int64);
            else if (k == "h") h = int(v.u.int64);
            else if (k == "stride") stride = int(v.u.int64);
            else if (k == "format" && v.format == MPV_FORMAT_STRING) fmt = v.u.string;
            else if (k == "data" && v.format == MPV_FORMAT_BYTE_ARRAY) data = v.u.ba;
        }
        // "bgr0" is little-endian 0xffRRGGBB == QImage::Format_RGB32.
        if (data && fmt == "bgr0" && w > 0 && h > 0 && stride >= w * 4 && data->size >= size_t(stride) * h)
            img = QImage(static_cast<const uchar *>(data->data), w, h, stride, QImage::Format_RGB32).copy();
    }
    mpv_free_node_contents(&res);
    return img;
}
