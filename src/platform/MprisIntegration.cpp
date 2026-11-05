#include "MprisIntegration.h"

#include "utilities/Logger.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingReply>

namespace {

const QString kObjectPath = QStringLiteral("/org/mpris/MediaPlayer2");
const QString kPlayerIface = QStringLiteral("org.mpris.MediaPlayer2.Player");

// The pure layer keeps track ids as strings; D-Bus wants an object path ('o').
QVariantMap dbusMetadata(QVariantMap m)
{
    const auto it = m.find(QStringLiteral("mpris:trackid"));
    if (it != m.end())
        *it = QVariant::fromValue(QDBusObjectPath(it->toString()));
    return m;
}

struct Backend {
    const char *service, *path, *iface, *inhibit, *uninhibit;
};
constexpr Backend kBackends[] = {
    {"org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", "org.freedesktop.ScreenSaver", "Inhibit", "UnInhibit"},
    {"org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Inhibit", "Inhibit", nullptr},
    {"org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager", "Inhibit", "Uninhibit"},
};
constexpr int kBackendCount = int(sizeof(kBackends) / sizeof(kBackends[0]));

} // namespace

// ---- ScreenInhibitor ------------------------------------------------------

void ScreenInhibitor::setInhibited(bool on)
{
    m_wanted = on;
    if (on && m_backend < 0 && !m_pending)
        tryBackend(0);
    else if (!on)
        release();
}

void ScreenInhibitor::tryBackend(int backend)
{
    auto bus = QDBusConnection::sessionBus();
    if (backend >= kBackendCount || !bus.isConnected()) {
        qCInfo(lcCore) << "No screensaver inhibit backend available";
        return;
    }
    const Backend &b = kBackends[backend];
    QDBusMessage msg = QDBusMessage::createMethodCall(QLatin1String(b.service), QLatin1String(b.path),
                                                      QLatin1String(b.iface), QLatin1String(b.inhibit));
    const QString app = QStringLiteral("Lumina Player"), why = QStringLiteral("Playing video");
    if (backend == 0)
        msg << app << why;
    else if (backend == 1)
        msg << QString() << quint32(8) << QVariantMap{{QStringLiteral("reason"), why}}; // 8 = idle
    else
        msg << app << quint32(0) << why << quint32(8);

    m_pending = true;
    auto *w = new QDBusPendingCallWatcher(bus.asyncCall(msg), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this, backend](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        m_pending = false;
        const QDBusMessage reply = watcher->reply();
        if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
            tryBackend(backend + 1);
            return;
        }
        const QVariant v = reply.arguments().first();
        if (backend == 1)
            m_handle = v.value<QDBusObjectPath>().path();
        else
            m_cookie = v.toUInt();
        m_backend = backend;
        qCInfo(lcCore) << "Screensaver inhibited via" << kBackends[backend].service;
        if (!m_wanted) // paused again while the request was in flight
            release();
    });
}

void ScreenInhibitor::release()
{
    if (m_backend < 0)
        return;
    const Backend &b = kBackends[m_backend];
    QDBusMessage msg;
    if (m_backend == 1) {
        msg = QDBusMessage::createMethodCall(QLatin1String(b.service), m_handle,
                                             QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Close"));
    } else {
        msg = QDBusMessage::createMethodCall(QLatin1String(b.service), QLatin1String(b.path),
                                             QLatin1String(b.iface), QLatin1String(b.uninhibit));
        msg << m_cookie;
    }
    QDBusConnection::sessionBus().asyncCall(msg);
    m_backend = -1;
}

// ---- Player adaptor ---------------------------------------------------------

MprisPlayerAdaptor::MprisPlayerAdaptor(MprisIntegration *owner)
    : QDBusAbstractAdaptor(owner), m_owner(owner) {}

QString MprisPlayerAdaptor::playbackStatus() const { return Mpris::playbackStatus(m_owner->state()); }
QVariantMap MprisPlayerAdaptor::metadata() const { return dbusMetadata(Mpris::metadata(m_owner->state())); }
double MprisPlayerAdaptor::volume() const { return m_owner->state().volume; }
void MprisPlayerAdaptor::setVolume(double v) { emit m_owner->volumeRequested(qBound(0.0, v, 1.0)); }
double MprisPlayerAdaptor::rate() const { return m_owner->state().rate; }
qlonglong MprisPlayerAdaptor::position() const { return Mpris::toMicros(m_owner->state().position); }
bool MprisPlayerAdaptor::canGoNext() const { return m_owner->state().canNext; }
bool MprisPlayerAdaptor::canGoPrevious() const { return m_owner->state().canPrevious; }

void MprisPlayerAdaptor::Play() { emit m_owner->playRequested(); }
void MprisPlayerAdaptor::Pause() { emit m_owner->pauseRequested(); }
void MprisPlayerAdaptor::PlayPause() { emit m_owner->playPauseRequested(); }
void MprisPlayerAdaptor::Stop() { emit m_owner->stopRequested(); }
void MprisPlayerAdaptor::Next() { emit m_owner->nextRequested(); }
void MprisPlayerAdaptor::Previous() { emit m_owner->previousRequested(); }
void MprisPlayerAdaptor::Seek(qlonglong offsetMicros) { emit m_owner->seekByRequested(offsetMicros / 1e6); }
void MprisPlayerAdaptor::SetPosition(const QDBusObjectPath &trackId, qlonglong positionMicros)
{
    if (trackId.path() == Mpris::kTrackPath)
        emit m_owner->seekToRequested(positionMicros / 1e6);
}
void MprisPlayerAdaptor::OpenUri(const QString &uri) { emit m_owner->openUriRequested(QUrl::fromUserInput(uri)); }

// ---- Integration ------------------------------------------------------------

MprisIntegration::MprisIntegration(QObject *parent)
    : PlatformIntegration(parent)
{
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCInfo(lcCore) << "No D-Bus session bus; MPRIS and screensaver inhibit disabled";
        return;
    }
    new MprisRootAdaptor(this);
    m_player = new MprisPlayerAdaptor(this);
    m_inhibitor = new ScreenInhibitor(this);
    if (bus.registerObject(kObjectPath, this) && bus.registerService(QStringLiteral("org.mpris.MediaPlayer2.lumina"))) {
        m_connected = true;
        qCInfo(lcCore) << "MPRIS service registered";
    } else {
        qCWarning(lcCore) << "MPRIS registration failed:" << bus.lastError().message();
    }
}

void MprisIntegration::update(const MediaState &state)
{
    const MediaState old = m_state;
    m_state = state;
    if (m_inhibitor)
        m_inhibitor->setInhibited(Mpris::wantsInhibit(state));
    if (!m_connected)
        return;
    const QVariantMap changed = Mpris::changedProperties(old, state);
    if (!changed.isEmpty())
        emitPropertiesChanged(changed);
}

void MprisIntegration::seeked(double position)
{
    m_state.position = position;
    if (m_connected)
        emit m_player->Seeked(Mpris::toMicros(position));
}

void MprisIntegration::emitPropertiesChanged(const QVariantMap &changed)
{
    QVariantMap props = changed;
    if (props.contains(QStringLiteral("Metadata")))
        props[QStringLiteral("Metadata")] = dbusMetadata(props.value(QStringLiteral("Metadata")).toMap());
    QDBusMessage sig = QDBusMessage::createSignal(kObjectPath, QStringLiteral("org.freedesktop.DBus.Properties"),
                                                  QStringLiteral("PropertiesChanged"));
    sig << kPlayerIface << props << QStringList();
    QDBusConnection::sessionBus().send(sig);
}
