#include "ApplicationContext.h"

#include "core/PlayerController.h"
#include "utilities/Logger.h"
#include "utilities/TimeUtils.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <functional>
#include <QQmlContext>
#include <QQmlEngine>
#include <QRandomGenerator>
#include <QSharedPointer>
#include <QTimer>

namespace {
double percentile(QVector<double> v, double p)
{
    if (v.isEmpty())
        return 0.0;
    std::sort(v.begin(), v.end());
    return v.at(qMax(0, int(std::ceil(p * v.size())) - 1));
}

double droppedNow(MpvEngine *engine)
{
    engine->setStatsEnabled(false);
    engine->setStatsEnabled(true); // enabling refreshes immediately
    const QVariantMap m = engine->stats();
    return m.value(QStringLiteral("droppedFrames")).toDouble()
           + m.value(QStringLiteral("decoderDroppedFrames")).toDouble();
}

// LUMINA_SEEK_BENCH=1: N exact seeks, N keyframe seeks (each awaited), then a
// spam burst. Latency = seek command -> MPV_EVENT_PLAYBACK_RESTART.
void runSeekBench(MpvEngine *engine)
{
    struct State {
        int n = 40, i = 0, mode = 0, gen = 0, restarts = 0;
        QVector<double> lat[2];
        QVector<double> targets[2];
        int timeouts = 0;
        QElapsedTimer clock;
        double drops0 = 0, dropsEnd[2] = {0, 0}, spamDrops = 0;
        qint64 lastRestartMs = 0;
        bool waiting = false;
        std::function<void()> next;
    };
    auto st = QSharedPointer<State>::create();
    const int n = qEnvironmentVariableIntValue("LUMINA_SEEK_BENCH_N");
    if (n > 0)
        st->n = n;

    auto finish = [engine, st]() {
        const QString csvPath = qEnvironmentVariable("LUMINA_SEEK_BENCH_CSV");
        if (!csvPath.isEmpty()) {
            QFile f(csvPath);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&f);
                out << "mode,index,target_s,latency_ms\n";
                static const char *names[] = {"exact", "keyframe"};
                for (int m = 0; m < 2; ++m)
                    for (int k = 0; k < st->lat[m].size(); ++k)
                        out << names[m] << ',' << k << ',' << st->targets[m][k] << ',' << st->lat[m][k] << '\n';
            }
        }
        const auto fmt = [](double v) { return QString::number(v, 'f', 1); };
        const QVariantMap sm = engine->stats();
        QString line = QStringLiteral("SEEKBENCH n=%1 hwdec=%2 codec=%3").arg(st->n)
                           .arg(sm.value(QStringLiteral("hwdec")).toString(), sm.value(QStringLiteral("codec")).toString());
        static const char *names[] = {"exact", "keyframe"};
        for (int m = 0; m < 2; ++m)
            line += QStringLiteral(" %1[p50=%2 p95=%3 max=%4 drops=%5]").arg(QLatin1String(names[m]))
                        .arg(fmt(percentile(st->lat[m], 0.5)), fmt(percentile(st->lat[m], 0.95)),
                             fmt(*std::max_element(st->lat[m].begin(), st->lat[m].end())))
                        .arg(st->dropsEnd[m]);
        line += QStringLiteral(" spam[settle_ms=%1 restarts=%2 drops=%3] timeouts=%4 (latencies in ms)")
                    .arg(st->lastRestartMs).arg(st->restarts).arg(st->spamDrops).arg(st->timeouts);
        qCInfo(lcCore).noquote() << line;
        const bool ok = engine->mediaError().isEmpty() && st->timeouts == 0
                        && percentile(st->lat[0], 0.95) < 3000 && percentile(st->lat[1], 0.95) < 3000;
        qCInfo(lcCore) << "Playback self test" << (ok ? "PASSED" : "FAILED");
        QCoreApplication::exit(ok ? 0 : 1);
    };

    // Phase 3: burst of 3*n rapid exact seeks 10 ms apart; settle = time of the
    // last restart (waits for 1.5 s of quiet).
    auto spam = [engine, st, finish]() {
        const int total = st->n * 3;
        st->restarts = 0;
        st->lastRestartMs = 0;
        st->clock.restart();
        const double d0 = droppedNow(engine);
        for (int k = 0; k < total; ++k)
            QTimer::singleShot(k * 10, engine, [engine]() {
                engine->seek(1.0 + QRandomGenerator::global()->generateDouble() * (engine->duration() - 6.0));
            });
        // Done once restarts have been quiet for 1.5 s (hard cap 100 s).
        auto *quiet = new QTimer(engine);
        quiet->setInterval(100);
        QObject::connect(quiet, &QTimer::timeout, engine, [=]() {
            const qint64 now = st->clock.elapsed();
            if (now < total * 10 + 200 || (now < 100000 && (st->restarts == 0 || now - st->lastRestartMs < 1500)))
                return;
            quiet->stop();
            st->spamDrops = qMax(0.0, droppedNow(engine) - d0);
            finish();
        });
        quiet->start();
    };

    QObject::connect(engine, &MpvEngine::seekFinished, engine, [st]() {
        if (st->mode == 2) {
            ++st->restarts;
            st->lastRestartMs = st->clock.elapsed();
            return;
        }
        if (!st->waiting)
            return;
        st->waiting = false;
        ++st->gen;
        const double ms = st->clock.nsecsElapsed() / 1e6;
        st->lat[st->mode].append(ms);
        QTimer::singleShot(150, st->next);
    });

    st->next = [engine, st, spam]() {
        if (st->i >= st->n) {
            st->dropsEnd[st->mode] = qMax(0.0, droppedNow(engine) - st->drops0);
            st->i = 0;
            st->drops0 = droppedNow(engine);
            if (++st->mode == 2) {
                spam();
                return;
            }
        }
        ++st->i;
        const double target = 1.0 + QRandomGenerator::global()->generateDouble() * (engine->duration() - 6.0);
        st->targets[st->mode].append(target);
        const int gen = ++st->gen;
        st->waiting = true;
        st->clock.restart();
        engine->seek(target, st->mode == 0);
        QTimer::singleShot(5000, engine, [st, gen]() {
            if (st->waiting && st->gen == gen) { // no restart: count and move on
                st->waiting = false;
                ++st->timeouts;
                st->lat[st->mode].append(5000.0);
                st->next();
            }
        });
    };

    auto *poll = new QTimer(engine);
    poll->setInterval(200);
    QObject::connect(poll, &QTimer::timeout, engine, [engine, st, poll]() {
        if (engine->idle() || engine->duration() < 10.0)
            return;
        poll->stop();
        poll->deleteLater();
        QTimer::singleShot(1000, engine, [engine, st]() {
            st->drops0 = droppedNow(engine);
            st->next();
        });
    });
    poll->start();
}
} // namespace

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
    if (qEnvironmentVariableIntValue("LUMINA_SEEK_BENCH") == 1) {
        runSeekBench(engine);
        return;
    }
    if (qEnvironmentVariableIntValue("LUMINA_SEEK_STRESS") == 1) {
        // 50 random absolute seeks at 50 ms intervals, then verdict.
        auto *stress = new QTimer(this);
        const auto count = QSharedPointer<int>::create(0);
        stress->setInterval(50);
        QObject::connect(stress, &QTimer::timeout, this, [engine, count, stress]() {
            if (engine->idle() || engine->duration() <= 0.0)
                return; // wait for load
            if (*count < 50) {
                ++*count;
                // Stay clear of EOF: keep-open pauses there.
                engine->seek(QRandomGenerator::global()->generateDouble() * (engine->duration() - 5.0));
                return;
            }
            stress->stop();
            stress->deleteLater();
            QTimer::singleShot(1000, engine, [engine]() {
                engine->setStatsEnabled(true);
                const QVariantMap st = engine->stats();
                engine->setStatsEnabled(false);
                const bool ok = engine->mediaError().isEmpty() && !engine->idle() && !engine->paused();
                qCInfo(lcCore) << "SEEKSTRESS frame-drop-count=" << st.value(QStringLiteral("droppedFrames"))
                               << "decoder-drops=" << st.value(QStringLiteral("decoderDroppedFrames"))
                               << "hwdec=" << st.value(QStringLiteral("hwdec"));
                qCInfo(lcCore) << "Playback self test" << (ok ? "PASSED" : "FAILED");
                QCoreApplication::exit(ok ? 0 : 1);
            });
        });
        stress->start();
        return;
    }
    if (qEnvironmentVariableIntValue("LUMINA_SPEED_TEST") == 1) {
        // For each speed: 4 s window, then compare media-time progress to the expected rate.
        struct State { int step = -1, tick = 0; double startPos = 0, startDrops = 0; bool ok = true; };
        static const double speeds[] = {1.0, 1.5, 2.0, 0.5, 1.0};
        auto st = QSharedPointer<State>::create();
        engine->setStatsEnabled(true);
        auto *t = new QTimer(this);
        t->setInterval(1000);
        QObject::connect(t, &QTimer::timeout, this, [engine, st, t]() {
            if (engine->idle() || engine->duration() <= 0.0)
                return;
            const auto drops = [engine]() {
                const QVariantMap m = engine->stats();
                return m.value(QStringLiteral("droppedFrames")).toDouble()
                       + m.value(QStringLiteral("decoderDroppedFrames")).toDouble();
            };
            if (st->step >= 0 && ++st->tick == 4) {
                const double rate = (engine->position() - st->startPos) / 4.0;
                const double want = speeds[st->step];
                const bool good = qAbs(rate - want) < qMax(want * 0.15, 0.1);
                st->ok = st->ok && good;
                qCInfo(lcCore) << "SPEEDTEST speed=" << want << "measured=" << rate
                               << "drops=" << (drops() - st->startDrops)
                               << "avsync=" << engine->stats().value(QStringLiteral("avsync"))
                               << (good ? "OK" : "BAD");
            }
            if (st->step < 0 || st->tick >= 4) {
                if (++st->step >= 5) {
                    t->stop();
                    qCInfo(lcCore) << "Playback self test" << (st->ok ? "PASSED" : "FAILED");
                    QCoreApplication::exit(st->ok ? 0 : 1);
                    return;
                }
                engine->setSpeed(speeds[st->step]);
                st->tick = 0;
                st->startPos = engine->position();
                st->startDrops = drops();
            }
        });
        t->start();
        return;
    }
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
