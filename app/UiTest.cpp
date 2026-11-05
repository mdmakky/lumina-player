#include "UiTest.h"

#include "core/PlayerController.h"
#include "utilities/Logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "core/PlaylistModel.h"
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

    auto hasFs = [&] { return w->visibility() == QWindow::FullScreen; };
    auto fsCheck = [&](const QString &name, bool wantFs) {
        result(name, waitFor([&] { return hasFs() == wantFs; }, 6000));
        QTest::qWait(1200); // macOS fullscreen animation
    };
    auto vis = [&](const char *n) { auto *it = find(n); return it && it->isVisible(); };
    auto pause = [&] { eng->pause(); waitFor([&] { return eng->paused(); }); };
    auto shownRows = [&] {
        int shown = 0;
        for (int i = 0; i < 3; ++i)
            if (auto *r = find(("playlistRow_" + std::to_string(i)).c_str()); r && r->height() > 0) ++shown;
        return shown;
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
    pause();
    shot("paused");
    eng->play();
    waitFor([&] { return !eng->paused(); });

    if (doTest) {
        const double dur = eng->duration();
        const bool hasChapters = !eng->chapters().isEmpty();
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
            shot("speedmenu");
            if (auto *it = find("speedItem_2")) clickAt(center(it));
            result("speed menu item sets 2x", waitFor([&] { return qAbs(eng->speed() - 2.0) < 0.01; }),
                   QString("speed=%1").arg(eng->speed()));
        }
        key(Qt::Key_Z);
        result("Z resets speed", qAbs(eng->speed() - 1.0) < 0.01);
        // chapters menu (button only exists for files with chapters)
        result("chapters button visibility matches chapter list", vis("chaptersButton") == hasChapters);
        if (hasChapters) {
            eng->seek(0); waitFor([&] { return eng->position() < 1.0; });
            click("chaptersButton"); QTest::qWait(400);
            shot("chaptermenu");
            if (auto *it = find("chapterItem_1")) clickAt(center(it));
            result("chapter menu item seeks", waitFor([&] { return eng->position() > 1.0; }), QString::number(eng->position()));
            eng->seek(0); waitFor([&] { return eng->position() < 1.0; });
            key(Qt::Key_PageDown);
            result("PgDn next chapter", waitFor([&] { return eng->position() > 1.0; }), QString::number(eng->position()));
            const double at = eng->position();
            key(Qt::Key_PageUp);
            result("PgUp previous chapter", waitFor([&] { return eng->position() < at - 0.5; }),
                   QString("%1->%2").arg(at).arg(eng->position()));
        }
        // tracks menu
        if (click("tracksButton")) {
            QTest::qWait(400);
            shot("trackmenu");
            if (eng->audioTracks().size() > 1) {
                const int a0 = eng->audioTrackId();
                auto *it = find(a0 == 2 ? "audioItem_1" : "audioItem_2");
                if (it) clickAt(center(it));
                result("tracks menu switches audio", waitFor([&] { return eng->audioTrackId() != a0; }),
                       QString("%1->%2").arg(a0).arg(eng->audioTrackId()));
            } else {
                result("tracks menu has subtitle Off entry", vis("subItem_off"));
                key(Qt::Key_Escape);
            }
            QTest::qWait(300);
        }
        // fullscreen button + double click + Esc
        click("fullscreenButton");
        fsCheck("fullscreen button enters", true);
        click("fullscreenButton");
        fsCheck("fullscreen button exits", false);
        QTest::mouseMove(w, QPoint(w->width() / 2, w->height() / 3)); QTest::qWait(60);
        QTest::mouseDClick(w, Qt::LeftButton, {}, QPoint(w->width() / 2, w->height() / 3), 60);
        fsCheck("double-click video enters fullscreen", true);
        key(Qt::Key_Escape);
        fsCheck("Esc exits fullscreen", false);
        {
            auto *ob = find("openButton"); // clicking would open a native dialog
            result("open button present", ob && ob->isVisible() && ob->isEnabled());
        }
        // keyboard shortcuts
        was = eng->paused();
        key(Qt::Key_Space);
        result("Space toggles pause", waitFor([&] { return eng->paused() != was; }));
        key(Qt::Key_Space);
        was = eng->muted();
        key(Qt::Key_M);
        result("M toggles mute", eng->muted() != was);
        key(Qt::Key_M);
        key(Qt::Key_F);
        fsCheck("F enters fullscreen", true);
        key(Qt::Key_F);
        fsCheck("F exits fullscreen", false);
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
        const double sd = eng->subtitleDelay();
        key(Qt::Key_H);
        result("H raises subtitle delay", qAbs(eng->subtitleDelay() - (sd + 0.1)) < 0.01, QString::number(eng->subtitleDelay()));
        key(Qt::Key_G);
        result("G lowers subtitle delay", qAbs(eng->subtitleDelay() - sd) < 0.01);
        pause();
        QTest::qWait(300);
        p0 = eng->position();
        key(Qt::Key_Period);
        result("Period steps frame", waitFor([&] { return eng->position() != p0; }), QString("%1->%2").arg(p0).arg(eng->position()));
        p0 = eng->position();
        key(Qt::Key_Comma);
        result("Comma steps back", waitFor([&] { return eng->position() < p0; }), QString("%1->%2").arg(p0).arg(eng->position()));
        eng->play();
        if (eng->audioTracks().size() > 1) {
            const QString a0 = aid();
            key(Qt::Key_A);
            result("A cycles audio track", waitFor([&] { return aid() != a0; }), a0 + "->" + aid());
        }
        // stats overlay
        key(Qt::Key_I);
        QTest::qWait(800);
        result("I shows stats overlay", eng->statsEnabled() && vis("statsOverlay"));
        shot("stats");
        key(Qt::Key_I);
        result("I hides stats overlay", waitFor([&] { return !eng->statsEnabled() && !vis("statsOverlay"); }));

        // --- playlist (queue: argv file + short1 + short2, kept paused) ---
        auto *pl = p->playlist();
        p->openFiles({QUrl::fromLocalFile(files.first()), QUrl::fromLocalFile("/tmp/short1.mp4"),
                      QUrl::fromLocalFile("/tmp/short2.mp4")});
        waitFor([&] { return pl->rowCount() == 3 && !eng->idle(); }, 6000);
        pause();
        result("queue has 3 entries", pl->rowCount() == 3, QString::number(pl->rowCount()));
        click("nextButton");
        result("next button advances", waitFor([&] { return pl->currentIndex() == 1; }), QString::number(pl->currentIndex()));
        click("prevButton");
        result("prev button goes back", waitFor([&] { return pl->currentIndex() == 0; }), QString::number(pl->currentIndex()));
        key(Qt::Key_Right, Qt::ControlModifier);
        result("Ctrl+Right next", waitFor([&] { return pl->currentIndex() == 1; }), QString::number(pl->currentIndex()));
        key(Qt::Key_Left, Qt::ControlModifier);
        result("Ctrl+Left previous", waitFor([&] { return pl->currentIndex() == 0; }), QString::number(pl->currentIndex()));
        pause();
        key(Qt::Key_P);
        result("P opens playlist panel", waitFor([&] { return vis("playlistPanel"); }));
        shot("playlist");
        key(Qt::Key_P);
        result("P closes playlist panel", waitFor([&] { return !vis("playlistPanel"); }));
        click("playlistButton");
        result("playlist button opens panel", waitFor([&] { return vis("playlistPanel"); }));
        QTest::qWait(300);
        if (auto *pn = find("playlistPanel"); pn && find("controlsBar")) {
            const double panelBottom = pn->mapToScene(QPointF(0, pn->height())).y();
            const double barTop = find("controlsBar")->mapToScene(QPointF(0, 0)).y();
            result("playlist panel clear of controls bar", panelBottom <= barTop, QString("%1 <= %2").arg(panelBottom).arg(barTop));
        }
        // Save/Load open native dialogs: check presence, then exercise the controller API they call.
        result("playlist save/load buttons present", vis("playlistSaveButton") && vis("playlistLoadButton"));
        const QUrl m3u = QUrl::fromLocalFile("/tmp/lumina-uitest.m3u8");
        QFile::remove(m3u.toLocalFile());
        result("savePlaylist writes file", p->savePlaylist(m3u) && QFileInfo::exists(m3u.toLocalFile()));
        if (auto *r = find("playlistRow_2")) {
            wake();
            const QPoint pt = center(r);
            QTest::mouseMove(w, pt); QTest::qWait(60);
            QTest::mouseDClick(w, Qt::LeftButton, {}, pt, 60);
            result("double-click row plays entry", waitFor([&] { return pl->currentIndex() == 2; }), QString::number(pl->currentIndex()));
            pause();
        } else result("double-click row plays entry", false, "row missing");
        const QUrl u0 = pl->urlAt(0), u1 = pl->urlAt(1);
        // drag row 0's grip down two rows (rows are 36px) -> row 0 ends at index 2
        if (auto *g = find("rowGrip_0")) {
            wake();
            const QPoint a = center(g);
            QTest::mouseMove(w, a); QTest::qWait(60);
            QTest::mousePress(w, Qt::LeftButton, {}, a); QTest::qWait(60);
            for (int dy = 6; dy <= 74; dy += 8) { QTest::mouseMove(w, a + QPoint(0, dy)); QTest::qWait(25); }
            QTest::mouseRelease(w, Qt::LeftButton, {}, a + QPoint(0, 74)); QTest::qWait(200);
            result("drag reorders row", waitFor([&] { return pl->urlAt(2) == u0 && pl->urlAt(0) == u1; }),
                   pl->urlAt(2).fileName());
        } else result("drag reorders row", false, "grip missing");
        pl->move(2, 0);
        // shuffle / repeat toggles
        p->setShuffle(false); p->setRepeatMode(0);
        click("shuffleButton");
        result("shuffle button toggles on", p->shuffle());
        key(Qt::Key_S, Qt::ShiftModifier);
        result("Shift+S toggles shuffle off", !p->shuffle());
        click("repeatButton");
        result("repeat button -> all", p->repeatMode() == 1);
        key(Qt::Key_R);
        result("R cycles repeat -> one", p->repeatMode() == 2);
        key(Qt::Key_R);
        result("R cycles repeat -> off", p->repeatMode() == 0);
        if (auto *sf = find("playlistSearch")) {
            clickAt(center(sf));
            for (auto k : {Qt::Key_S, Qt::Key_H, Qt::Key_O, Qt::Key_R, Qt::Key_T}) QTest::keyClick(w, k);
            QTest::qWait(300);
            int shown = shownRows();
            result("search filters rows", shown == 2, QString("shown=%1").arg(shown));
            for (int i = 0; i < 5; ++i) QTest::keyClick(w, Qt::Key_Backspace);
            QTest::qWait(300);
            shown = shownRows();
            result("clearing search restores rows", shown == 3, QString("shown=%1").arg(shown));
        } else result("search filters rows", false, "missing");
        click("rowRemove_2");
        result("remove row decreases count", waitFor([&] { return pl->rowCount() == 2; }), QString::number(pl->rowCount()));
        click("playlistClearButton");
        result("clear empties queue", waitFor([&] { return pl->rowCount() == 0; }), QString::number(pl->rowCount()));
        result("loadPlaylist restores queue", p->loadPlaylist(m3u) && waitFor([&] { return pl->rowCount() == 3; }));
        pause();
        // home screen recent rows (stop via engine API; the new UI has no stop button)
        pl->clear();
        eng->stop();
        result("stop returns to home", waitFor([&] { return eng->idle(); }));
        QTest::qWait(500);
        shot("home-recent");
        if (auto *r = find("recentRow_0")) {
            clickAt(center(r));
            result("recent row opens media", waitFor([&] { return !eng->idle() && eng->duration() > 1; }, 6000));
        } else result("recent row opens media", false, "no recent rows");
        qCInfo(lcCore) << "UITEST SUMMARY passes=" << passes << "fails=" << fails;
    }
    QCoreApplication::exit(fails ? 1 : 0);
}
