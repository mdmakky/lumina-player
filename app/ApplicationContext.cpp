#include "ApplicationContext.h"

#include "core/PlayerController.h"
#include "utilities/Logger.h"
#include "utilities/TimeUtils.h"

#include <QCoreApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSharedPointer>
#include <QTimer>

ApplicationContext::ApplicationContext(QObject *parent)
    : QObject(parent)
    , m_player(new PlayerController(this))
{
}

void ApplicationContext::registerOnEngine(QQmlEngine *qmlEngine)
{
    qmlEngine->rootContext()->setContextProperty(QStringLiteral("player"), m_player);
    qmlEngine->rootContext()->setContextProperty(QStringLiteral("timefmt"),
                                                 new Lumina::TimeUtils(qmlEngine));
}

void ApplicationContext::openCommandLineFiles(const QStringList &arguments)
{
    if (arguments.isEmpty())
        return;

    QVariantList urls;
    urls.reserve(arguments.size());
    for (const QString &argument : arguments)
        urls.append(QUrl::fromUserInput(argument));

    m_player->openFiles(urls);
    qCInfo(lcCore) << "Opened" << urls.size() << "file(s) from command line";
}

void ApplicationContext::startPlaybackSelfTest()
{
    auto *engine = m_player->engine();
    if (!engine) {
        qCCritical(lcCore) << "Self test requested but no engine exists";
        QCoreApplication::exit(2);
        return;
    }

    qCInfo(lcCore) << "Playback self test started";
    // Shared counter: both lambdas own a reference; freed when the last dies.
    const auto elapsed = QSharedPointer<int>::create(0);
    auto *timer = new QTimer(this);
    timer->setInterval(1000);

    const auto snapshot = [engine, elapsed]() {
        qCInfo(lcCore).nospace() << "t=" << *elapsed << "s"
                                 << " pos=" << QString::number(engine->position(), 'f', 2)
                                 << " dur=" << QString::number(engine->duration(), 'f', 2)
                                 << " paused=" << engine->paused()
                                 << " idle=" << engine->idle()
                                 << " chapters=" << engine->chapters().size()
                                 << " error=" << engine->mediaError();
    };

    QObject::connect(timer, &QTimer::timeout, this, [engine, elapsed, snapshot, timer]() {
        ++*elapsed;
        switch (*elapsed) {
        case 3:
            qCInfo(lcCore) << "SELFTEST seek to 10s";
            engine->seek(10.0);
            break;
        case 4:
            if (engine->audioTracks().size() > 1)
                engine->setAudioTrack(2);
            engine->setSubtitleDelay(0.1);
            break;
        case 5:
            qCInfo(lcCore) << "SELFTEST pause";
            engine->pause();
            break;
        case 7:
            qCInfo(lcCore) << "SELFTEST resume";
            engine->play();
            break;
        case 9: {
            snapshot();
            qCInfo(lcCore) << "SELFTEST tracks audio=" << engine->audioTracks().size()
                           << "sub=" << engine->subtitleTracks().size()
                           << "aid=" << engine->audioTrackId() << "sid=" << engine->subtitleTrackId()
                           << "subDelay=" << engine->subtitleDelay();
            timer->stop();
            timer->deleteLater();

            // Verdict: media must be loaded, non-trivial duration observed,
            // position must have advanced past the seek target, no engine error.
            const bool ok = !engine->idle() && engine->duration() > 5.0
                            && engine->position() >= 10.0
                            && engine->mediaError().isEmpty();
            qCInfo(lcCore) << "Playback self test" << (ok ? "PASSED" : "FAILED");
            QCoreApplication::exit(ok ? 0 : 1);
            return;
        }
        default:
            snapshot();
            break;
        }
    });

    timer->start();
}
