#include "UiTest.h"

#include "core/PlayerController.h"
#include "utilities/Logger.h"

#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QTest>
#include <QTimer>
#include <QUrl>

#include <functional>

static bool env(const char *n) { return qEnvironmentVariableIsSet(n); }

bool UiTest::requested() { return env("LUMINA_UI_TEST") || env("LUMINA_SCREENSHOT"); }

UiTest::UiTest(QQuickWindow *window, PlayerController *player, QStringList f, QObject *parent)
    : QObject(parent), w(window), p(player), files(std::move(f)) {}

void UiTest::start()
{
    QTimer::singleShot(500, this, [this] { run(); });
}

namespace {
// QML-created objects are QObject-parented to their component, not their visual
// parent, so findChild() misses them; walk the visual tree instead.
QQuickItem *findItem(QQuickItem *root, const QString &name)
{
    if (root->objectName() == name) return root;
    for (QQuickItem *c : root->childItems())
        if (auto *r = findItem(c, name)) return r;
    return nullptr;
}
bool waitFor(const std::function<bool()> &cond, int ms = 4000)
{
    for (int t = 0; t < ms; t += 25) {
        if (cond()) return true;
        QTest::qWait(25);
    }
    return cond();
}
}

void UiTest::run()
{
    auto *eng = p->engine();
    const bool doTest = env("LUMINA_UI_TEST");
    const QString shotDir = qEnvironmentVariable("LUMINA_SCREENSHOT");
    w->requestActivate();
    QTest::qWait(500);

    auto result = [&](const QString &name, bool ok, const QString &detail = {}) {
        ++(ok ? passes : fails);
        qCInfo(lcCore).noquote() << "UITEST" << (ok ? "PASS" : "FAIL") << name << detail;
    };
    auto shot = [&](const QString &name) {
        if (shotDir.isEmpty()) return;
        QDir().mkpath(shotDir);
        QTest::qWait(300); // let the scene graph present a fresh frame
        const QImage img = w->grabWindow();
        QSet<QRgb> colors;
        for (int y = 0; y < img.height(); y += 7)
            for (int x = 0; x < img.width(); x += 7)
                colors.insert(img.pixel(x, y));
        const QString path = QDir(shotDir).filePath(name + ".png");
        const bool ok = !img.isNull() && img.save(path) && colors.size() > 1;
        result("screenshot " + name, ok, QStringLiteral("%1 (%2x%3, %4 colors)")
               .arg(path).arg(img.width()).arg(img.height()).arg(colors.size()));
    };
    auto find = [&](const char *n) { return findItem(w->contentItem(), QString::fromLatin1(n)); };
    auto center = [&](QQuickItem *it) { return it->mapToScene(QPointF(it->width() / 2, it->height() / 2)).toPoint(); };
    auto wake = [&] { QTest::mouseMove(w, QPoint(w->width() / 2, w->height() / 2)); QTest::qWait(60); };
    auto clickAt = [&](QPoint pt) {
        QTest::mouseMove(w, pt); QTest::qWait(40);
        QTest::mouseClick(w, Qt::LeftButton, {}, pt, 60);
        QTest::qWait(150);
    };
    auto click = [&](const char *n) {
        wake();
        auto *it = find(n);
        if (!it || !it->isVisible()) { result(QString("find ") + n, false, "missing/invisible"); return false; }
        clickAt(center(it));
        return true;
    };
    auto key = [&](Qt::Key k, Qt::KeyboardModifiers m = {}) { QTest::keyClick(w, k, m, 40); QTest::qWait(250); };
    auto aid = [&] {
        char *s = mpv_get_property_string(eng->handle(), "aid");
        QString r = s ? QString::fromUtf8(s) : QString();
        mpv_free(s);
        return r;
    };

    // --- home / empty state ---
    shot("home");
    if (doTest) {
        auto *b = find("homeOpenButton");
        result("home open button present", b && b->isVisible() && b->isEnabled());
    }
    if (files.isEmpty()) { qCInfo(lcCore) << "no media arg; stopping after home"; QCoreApplication::exit(fails ? 1 : 0); return; }

    // --- load media ---
    QVariantList urls;
    for (const auto &f : files) urls << QUrl::fromUserInput(f);
    p->openFiles(urls);
    if (!waitFor([&] { return !eng->idle() && eng->duration() > 1 && !eng->paused(); }, 8000)) {
        result("load media", false, eng->mediaError()); QCoreApplication::exit(1); return;
    }
    QTest::qWait(2000);
    shot("playing");
    eng->pause(); QTest::qWait(300);
    shot("paused");
    eng->play();
    waitFor([&] { return !eng->paused(); });

    if (doTest) {
        const double dur = eng->duration();
        // play button
        bool was = eng->paused();
        click("playButton");
        result("play button toggles pause", waitFor([&] { return eng->paused() != was; }));
        click("playButton");
        result("play button toggles back", waitFor([&] { return eng->paused() == was; }));
        // mute
        was = eng->muted();
        click("muteButton");
        result("mute button toggles", waitFor([&] { return eng->muted() != was; }));
        click("muteButton");
        result("mute button toggles back", waitFor([&] { return eng->muted() == was; }));
        // volume slider: click at 25% of the track
        if (auto *v = find("volumeSlider")) {
            wake();
            const QPointF tl = v->mapToScene(QPointF(0, v->height() / 2));
            clickAt(QPoint(int(tl.x() + v->width() * 0.25), int(tl.y())));
            result("volume slider click", waitFor([&] { return eng->volume() >= 20 && eng->volume() <= 55; }),
                   QString("volume=%1").arg(eng->volume()));
        } else result("volume slider click", false, "missing");
        // timeline: click at 60%
        if (auto *t = find("timeline")) {
            wake();
            const QPointF tl = t->mapToScene(QPointF(0, t->height() / 2));
            clickAt(QPoint(int(tl.x() + t->width() * 0.6), int(tl.y())));
            const double target = dur * 0.6;
            result("timeline click seeks", waitFor([&] { return qAbs(eng->position() - target) < dur * 0.1; }),
                   QString("pos=%1 target=%2").arg(eng->position()).arg(target));
        } else result("timeline click seeks", false, "missing");
        // speed menu
        if (click("speedButton")) {
            QTest::qWait(400);
            if (auto *it = find("speedItem_2")) clickAt(center(it));
            result("speed menu item sets 2x", waitFor([&] { return qAbs(eng->speed() - 2.0) < 0.01; }),
                   QString("speed=%1").arg(eng->speed()));
        }
        key(Qt::Key_Z);
        result("Z resets speed", qAbs(eng->speed() - 1.0) < 0.01);
        // fullscreen button
        click("fullscreenButton");
        result("fullscreen button enters", waitFor([&] { return w->visibility() == QWindow::FullScreen; }, 6000));
        QTest::qWait(1200);
        click("fullscreenButton");
        result("fullscreen button exits", waitFor([&] { return w->visibility() != QWindow::FullScreen; }, 6000));
        QTest::qWait(1200);
        // double-click on video toggles fullscreen
        QTest::mouseMove(w, QPoint(w->width() / 2, w->height() / 3)); QTest::qWait(60);
        QTest::mouseDClick(w, Qt::LeftButton, {}, QPoint(w->width() / 2, w->height() / 3), 60);
        result("double-click video enters fullscreen", waitFor([&] { return w->visibility() == QWindow::FullScreen; }, 6000));
        QTest::qWait(1200);
        key(Qt::Key_Escape);
        result("Esc exits fullscreen", waitFor([&] { return w->visibility() != QWindow::FullScreen; }, 6000));
        QTest::qWait(1200);
        // open button exists (clicking would block on a native dialog)
        auto *ob = find("openButton");
        result("open button present", ob && ob->isVisible() && ob->isEnabled());
        // keyboard shortcuts
        was = eng->paused();
        key(Qt::Key_Space);
        result("Space toggles pause", eng->paused() != was);
        key(Qt::Key_Space);
        was = eng->muted();
        key(Qt::Key_M);
        result("M toggles mute", eng->muted() != was);
        key(Qt::Key_M);
        key(Qt::Key_F);
        result("F enters fullscreen", waitFor([&] { return w->visibility() == QWindow::FullScreen; }, 6000));
        QTest::qWait(1200);
        key(Qt::Key_F);
        result("F exits fullscreen", waitFor([&] { return w->visibility() != QWindow::FullScreen; }, 6000));
        QTest::qWait(1200);
        eng->seek(10); waitFor([&] { return qAbs(eng->position() - 10) < 1; });
        double p0 = eng->position();
        key(Qt::Key_Right);
        result("Right seeks +5", waitFor([&] { return eng->position() > p0 + 3; }), QString("%1->%2").arg(p0).arg(eng->position()));
        p0 = eng->position();
        key(Qt::Key_Left);
        result("Left seeks -5", waitFor([&] { return eng->position() < p0 - 3; }), QString("%1->%2").arg(p0).arg(eng->position()));
        int v0 = eng->volume();
        key(Qt::Key_Up);
        result("Up raises volume", eng->volume() == v0 + 5, QString::number(eng->volume()));
        key(Qt::Key_Down);
        result("Down lowers volume", eng->volume() == v0);
        key(Qt::Key_C);
        result("C speeds up", qAbs(eng->speed() - 1.25) < 0.01);
        key(Qt::Key_X);
        result("X slows down", qAbs(eng->speed() - 1.0) < 0.01);
        eng->pause(); waitFor([&] { return eng->paused(); }); QTest::qWait(300);
        p0 = eng->position();
        key(Qt::Key_Period);
        result("Period steps frame", waitFor([&] { return eng->position() != p0; }), QString("%1->%2").arg(p0).arg(eng->position()));
        eng->play();
        if (eng->audioTracks().size() > 1) {
            const QString a0 = aid();
            key(Qt::Key_A);
            result("A cycles audio track", waitFor([&] { return aid() != a0; }), a0 + "->" + aid());
        }
        // engine-level chapter seek (no chapters UI in this build)
        if (files.first().contains("chapters")) {
            eng->seek(0); waitFor([&] { return eng->position() < 1.0; });
            eng->seekChapter(1);
            result("engine seekChapter(1)", waitFor([&] { return eng->position() > 1.0; }), QString::number(eng->position()));
        }
        // stop button
        click("stopButton");
        result("stop button unloads media", waitFor([&] { return eng->idle(); }));
        shot("stopped");
        qCInfo(lcCore) << "UITEST SUMMARY passes=" << passes << "fails=" << fails;
    }
    QCoreApplication::exit(fails ? 1 : 0);
}
