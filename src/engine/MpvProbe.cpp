#include "MpvProbe.h"

#include "media/LibraryMeta.h"

#include <QElapsedTimer>
#include <QVariantMap>

#include <mpv/client.h>

#include <clocale>

namespace {
QVariant toVariant(const mpv_node &n)
{
    switch (n.format) {
    case MPV_FORMAT_STRING: return QString::fromUtf8(n.u.string);
    case MPV_FORMAT_FLAG: return n.u.flag != 0;
    case MPV_FORMAT_INT64: return qlonglong(n.u.int64);
    case MPV_FORMAT_DOUBLE: return n.u.double_;
    case MPV_FORMAT_NODE_MAP: {
        QVariantMap m;
        for (int i = 0; i < n.u.list->num; ++i)
            m.insert(QString::fromUtf8(n.u.list->keys[i]), toVariant(n.u.list->values[i]));
        return m;
    }
    case MPV_FORMAT_NODE_ARRAY: {
        QVariantList l;
        for (int i = 0; i < n.u.list->num; ++i) l << toVariant(n.u.list->values[i]);
        return l;
    }
    default: return {};
    }
}

QVariant prop(mpv_handle *h, const char *name)
{
    mpv_node n{};
    if (mpv_get_property(h, name, MPV_FORMAT_NODE, &n) < 0) return {};
    const QVariant v = toVariant(n);
    mpv_free_node_contents(&n);
    return v;
}
} // namespace

bool MpvProbe::probe(LibraryItem &item, int timeoutMs)
{
    std::setlocale(LC_NUMERIC, "C");
    mpv_handle *h = mpv_create();
    if (!h) return false;
    const char *opts[][2] = {
        {"vo", "null"}, {"ao", "null"}, {"hwdec", "no"}, {"aid", "no"}, {"sid", "no"}, {"pause", "yes"},
        {"load-scripts", "no"}, {"config", "no"}, {"osc", "no"}, {"input-default-bindings", "no"},
        {"audio-display", "no"}, {"cache", "no"}, {"demuxer-max-bytes", "2MiB"}, {"demuxer-readahead-secs", "0"},
        {"terminal", "no"}, {"msg-level", "all=no"},
    };
    for (const auto &o : opts) mpv_set_option_string(h, o[0], o[1]);
    if (mpv_initialize(h) < 0) { mpv_terminate_destroy(h); return false; }

    const QByteArray p = item.path.toUtf8();
    const char *cmd[] = {"loadfile", p.constData(), nullptr};
    bool loaded = false;
    if (mpv_command(h, cmd) >= 0) {
        QElapsedTimer t;
        t.start();
        while (t.elapsed() < timeoutMs) {
            const mpv_event *e = mpv_wait_event(h, 0.05);
            if (e->event_id == MPV_EVENT_FILE_LOADED) { loaded = true; break; }
            if (e->event_id == MPV_EVENT_SHUTDOWN || e->event_id == MPV_EVENT_END_FILE) break;
        }
    }
    if (loaded) {
        const QVariant dur = prop(h, "duration");
        item.duration = dur.isValid() && dur.toDouble() > 0 ? dur.toDouble() : -1;
        bool video = false, cover = false;
        for (const QVariant &tv : prop(h, "track-list").toList()) {
            const QVariantMap t = tv.toMap();
            if (t.value(QStringLiteral("type")).toString() != QLatin1String("video")) continue;
            if (t.value(QStringLiteral("albumart")).toBool()) { cover = true; continue; }
            if (t.value(QStringLiteral("image")).toBool() && item.kind == 1) continue;
            if (!video) {
                video = true;
                item.width = t.value(QStringLiteral("demux-w")).toInt();
                item.height = t.value(QStringLiteral("demux-h")).toInt();
                item.vcodec = t.value(QStringLiteral("codec")).toString();
            }
        }
        item.hasCover = cover;
        item.kind = video ? 0 : 1; // no real video stream: it is music
        const LibraryMeta::Tags tags = LibraryMeta::parseTags(prop(h, "metadata").toMap());
        item.tagTitle = tags.title;
        item.artist = tags.artist;
        item.album = tags.album;
        item.track = tags.track;
        item.year = tags.year;
    }
    mpv_terminate_destroy(h);
    return loaded;
}
