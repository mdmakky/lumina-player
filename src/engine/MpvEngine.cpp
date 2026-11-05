#include "MpvEngine.h"

#include "utilities/Logger.h"

#include <QByteArray>
#include <QMetaObject>
#include <QUrl>
#include <QVector>

#include <clocale>
#include <vector>

namespace {
} // namespace

MpvEngine::MpvEngine(QObject *parent)
    : QObject(parent)
{
    // mpv refuses to start under a non-C numeric locale, and QGuiApplication
    // has already switched to the environment locale by now (mpv docs
    // explicitly require this call from embedded clients).
    std::setlocale(LC_NUMERIC, "C");

    m_mpv = mpv_create();
    if (!m_mpv) {
        setMediaError(QStringLiteral("Failed to create mpv handle"));
        return;
    }

    // Presentation: render API, no built-in OSC/OSD — the UI draws everything.
    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "osc", "no");
    mpv_set_option_string(m_mpv, "osd-level", "0");
    mpv_set_option_string(m_mpv, "input-default-bindings", "no");
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
    mpv_set_option_string(m_mpv, "input-cursor", "no");

    // Playback behaviour.
    mpv_set_option_string(m_mpv, "idle", "yes");
    mpv_set_option_string(m_mpv, "keep-open", "always");
    mpv_set_option_string(m_mpv, "hwdec", "auto-safe");
    mpv_set_option_string(m_mpv, "volume-max", "150");
    mpv_set_option_string(m_mpv, "audio-pitch-correction", "yes");

    const int rc = mpv_initialize(m_mpv);
    if (rc < 0) {
        setMediaError(QStringLiteral("mpv initialization failed: %1").arg(mpv_error_string(rc)));
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
        return;
    }

    mpv_set_wakeup_callback(m_mpv, &MpvEngine::onWakeup, this);

    // Observe the properties the UI needs.
    const char *properties[] = {
        "time-pos", "duration", "pause", "idle-active", "media-title",
        "volume", "mute", "speed", "track-list",
    };
    for (const char *name : properties) {
        mpv_format format = MPV_FORMAT_DOUBLE;
        if (qstrcmp(name, "pause") == 0 || qstrcmp(name, "idle-active") == 0
            || qstrcmp(name, "mute") == 0) {
            format = MPV_FORMAT_FLAG;
        } else if (qstrcmp(name, "media-title") == 0) {
            format = MPV_FORMAT_STRING;
        } else if (qstrcmp(name, "track-list") == 0) {
            format = MPV_FORMAT_NODE;
        }
        mpv_observe_property(m_mpv, 0, name, format);
    }

    qCInfo(lcEngine) << "mpv initialized, version" << mpv_get_property_string(m_mpv, "mpv-version");
}

MpvEngine::~MpvEngine()
{
    if (m_mpv) {
        mpv_set_wakeup_callback(m_mpv, nullptr, nullptr);
        mpv_terminate_destroy(m_mpv);
    }
}

bool MpvEngine::playing() const
{
    return !m_paused && !m_idle;
}

void MpvEngine::processEvents()
{
    if (!m_mpv)
        return;

    for (;;) {
        mpv_event *event = mpv_wait_event(m_mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;

        switch (event->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE: {
            auto *prop = static_cast<mpv_event_property *>(event->data);
            const QString name = QString::fromUtf8(prop->name);

            if (name == QLatin1String("time-pos") && prop->format == MPV_FORMAT_DOUBLE) {
                m_position = *static_cast<double *>(prop->data);
                emit positionChanged();
            } else if (name == QLatin1String("duration") && prop->format == MPV_FORMAT_DOUBLE) {
                m_duration = *static_cast<double *>(prop->data);
                emit durationChanged();
            } else if (name == QLatin1String("pause") && prop->format == MPV_FORMAT_FLAG) {
                m_paused = *static_cast<int *>(prop->data) != 0;
                emit pausedChanged();
                emit playingChanged();
            } else if (name == QLatin1String("idle-active") && prop->format == MPV_FORMAT_FLAG) {
                m_idle = *static_cast<int *>(prop->data) != 0;
                emit idleChanged();
                emit playingChanged();
            } else if (name == QLatin1String("media-title") && prop->format == MPV_FORMAT_STRING) {
                m_mediaTitle = QString::fromUtf8(*static_cast<const char **>(prop->data));
                emit mediaTitleChanged();
            } else if (name == QLatin1String("volume") && prop->format == MPV_FORMAT_DOUBLE) {
                m_volume = qBound(0, qRound(*static_cast<double *>(prop->data)), 150);
                emit volumeChanged();
            } else if (name == QLatin1String("mute") && prop->format == MPV_FORMAT_FLAG) {
                m_muted = *static_cast<int *>(prop->data) != 0;
                emit mutedChanged();
            } else if (name == QLatin1String("speed") && prop->format == MPV_FORMAT_DOUBLE) {
                m_speed = *static_cast<double *>(prop->data);
                emit speedChanged();
            } else if (name == QLatin1String("track-list") && prop->format == MPV_FORMAT_NODE) {
                refreshTrackLists();
            }
            break;
        }
        case MPV_EVENT_END_FILE: {
            auto *endFile = static_cast<mpv_event_end_file *>(event->data);
            if (endFile->reason == MPV_END_FILE_REASON_EOF)
                emit endOfFile();
            if (endFile->reason == MPV_END_FILE_REASON_ERROR && endFile->error < 0) {
                setMediaError(QStringLiteral("Playback failed: %1").arg(mpv_error_string(endFile->error)));
            }
            break;
        }
        case MPV_EVENT_START_FILE:
            // New file started — clear any stale error state.
            setMediaError(QString());
            break;
        case MPV_EVENT_SHUTDOWN:
            qCWarning(lcEngine) << "mpv core shutdown";
            break;
        default:
            break;
        }
    }
}

bool MpvEngine::command(const QStringList &args)
{
    if (!m_mpv || args.isEmpty())
        return false;

    QVector<QByteArray> bytes;
    bytes.reserve(args.size());
    for (const QString &arg : args)
        bytes.append(arg.toUtf8());

    std::vector<const char *> pointers;
    pointers.reserve(bytes.size() + 1);
    for (const QByteArray &byte : bytes)
        pointers.push_back(byte.constData());
    pointers.push_back(nullptr);

    const int rc = mpv_command(m_mpv, pointers.data());
    if (rc < 0) {
        qCWarning(lcEngine) << "mpv command failed:" << args.join(u' ')
                            << "-" << mpv_error_string(rc);
        setMediaError(QStringLiteral("Command failed: %1").arg(mpv_error_string(rc)));
        return false;
    }
    return true;
}

void MpvEngine::setMediaError(const QString &message)
{
    if (m_mediaError == message)
        return;
    m_mediaError = message;
    emit mediaErrorChanged();
}

void MpvEngine::refreshTrackLists()
{
    m_audioTracks.clear();
    m_videoTracks.clear();
    m_subtitleTracks.clear();

    mpv_node node{};
    if (mpv_get_property(m_mpv, "track-list", MPV_FORMAT_NODE, &node) < 0)
        return;

    const QVariantList tracks = nodeToVariant(node).toList();
    mpv_free_node_contents(&node);

    for (const QVariant &track : tracks) {
        const QVariantMap map = track.toMap();
        const QString type = map.value(QStringLiteral("type")).toString();
        if (type == QLatin1String("audio"))
            m_audioTracks.append(track);
        else if (type == QLatin1String("video"))
            m_videoTracks.append(track);
        else if (type == QLatin1String("sub"))
            m_subtitleTracks.append(track);
    }

    emit tracksChanged();
}

QVariant MpvEngine::nodeToVariant(const mpv_node &node)
{
    switch (node.format) {
    case MPV_FORMAT_STRING:
        return QString::fromUtf8(node.u.string);
    case MPV_FORMAT_FLAG:
        return node.u.flag != 0;
    case MPV_FORMAT_INT64:
        return QVariant::fromValue<qlonglong>(node.u.int64);
    case MPV_FORMAT_DOUBLE:
        return node.u.double_;
    case MPV_FORMAT_NODE_MAP: {
        QVariantMap map;
        for (int i = 0; i < node.u.list->num; ++i)
            map.insert(QString::fromUtf8(node.u.list->keys[i]), nodeToVariant(node.u.list->values[i]));
        return map;
    }
    case MPV_FORMAT_NODE_ARRAY: {
        QVariantList list;
        list.reserve(node.u.list->num);
        for (int i = 0; i < node.u.list->num; ++i)
            list.append(nodeToVariant(node.u.list->values[i]));
        return list;
    }
    default:
        return {};
    }
}

void MpvEngine::loadFile(const QUrl &url)
{
    const QString path = url.scheme().isEmpty() ? url.toString() : url.toLocalFile();
    if (path.isEmpty()) {
        setMediaError(QStringLiteral("Cannot open an empty path"));
        return;
    }
    command({QStringLiteral("loadfile"), path, QStringLiteral("replace")});
}

void MpvEngine::loadFiles(const QVariantList &urls)
{
    if (urls.isEmpty())
        return;

    for (int i = 0; i < urls.size(); ++i) {
        const QUrl url = urls.at(i).toUrl();
        const QString path = url.scheme().isEmpty() ? url.toString() : url.toLocalFile();
        if (path.isEmpty())
            continue;
        command({QStringLiteral("loadfile"), path,
                 i == 0 ? QStringLiteral("replace") : QStringLiteral("append-play")});
    }
}

void MpvEngine::play()
{
    setPaused(false);
}

void MpvEngine::pause()
{
    setPaused(true);
}

void MpvEngine::togglePlayPause()
{
    setPaused(!m_paused);
}

void MpvEngine::stop()
{
    command({QStringLiteral("stop")});
    setMediaError(QString());
}

void MpvEngine::setPaused(bool paused)
{
    if (m_idle || !m_mpv)
        return;
    command({QStringLiteral("set"), QStringLiteral("pause"), paused ? QStringLiteral("yes")
                                                                    : QStringLiteral("no")});
}

void MpvEngine::setVolume(int volume)
{
    if (!m_mpv)
        return;
    command({QStringLiteral("set"), QStringLiteral("volume"), QString::number(qBound(0, volume, 150))});
}

void MpvEngine::setMuted(bool muted)
{
    if (!m_mpv)
        return;
    command({QStringLiteral("set"), QStringLiteral("mute"), muted ? QStringLiteral("yes")
                                                                  : QStringLiteral("no")});
}

void MpvEngine::setSpeed(double speed)
{
    if (!m_mpv)
        return;
    command({QStringLiteral("set"), QStringLiteral("speed"), QString::number(speed, 'f', 2)});
}

void MpvEngine::seek(double seconds, bool accurate)
{
    if (m_idle || !m_mpv)
        return;
    command({QStringLiteral("seek"), QString::number(seconds, 'f', 3),
             accurate ? QStringLiteral("absolute+exact") : QStringLiteral("absolute")});
}

void MpvEngine::seekRelative(double seconds)
{
    if (m_idle || !m_mpv)
        return;
    command({QStringLiteral("seek"), QString::number(seconds, 'f', 3), QStringLiteral("relative")});
}

void MpvEngine::stepFrame(int direction)
{
    if (m_idle || !m_mpv)
        return;
    command({QStringLiteral("frame-step")});
    Q_UNUSED(direction)
}

void MpvEngine::cycleAudioTrack()
{
    if (m_idle || !m_mpv)
        return;
    command({QStringLiteral("cycle"), QStringLiteral("audio")});
}

void MpvEngine::cycleSubtitleTrack()
{
    if (m_idle || !m_mpv)
        return;
    command({QStringLiteral("cycle"), QStringLiteral("sub")});
}

void MpvEngine::setSubtitlesVisible(bool visible)
{
    if (!m_mpv)
        return;
    command({QStringLiteral("set"), QStringLiteral("sub-visibility"),
             visible ? QStringLiteral("yes") : QStringLiteral("no")});
}

void MpvEngine::seekChapter(int index)
{
    if (m_idle || !m_mpv || index < 0)
        return;
    command({QStringLiteral("seek"), QString::number(index), QStringLiteral("absolute"),
             QStringLiteral("chapters")});
}
