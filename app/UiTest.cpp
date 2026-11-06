#include "UiTest.h"

#include "core/PlayerController.h"
#include "utilities/Logger.h"

#include <QDir>
#include <QElapsedTimer>
#include <algorithm>
#include <QFile>
#include <QFileInfo>
#include "core/PlaylistModel.h"
#include <QGuiApplication>
#include <QImage>
#include <QTcpServer>
#include <QTcpSocket>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSet>
#include <QTest>
#include <QTemporaryDir>
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
    auto wake = [&] { QTest::mouseMove(w, QPoint(w->width() / 2, w->height() / 2)); QTest::qWait(300); }; // 300 ms: let the chrome fade-in finish
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
    // Re-activate first: other apps (or parallel test runs) can steal focus and Shortcuts need an active window.
    auto key = [&](Qt::Key k, Qt::KeyboardModifiers m = {}) { w->requestActivate(); QTest::qWait(30); if (!w->isActive()) qCInfo(lcCore) << "UITEST window not active for key" << k; QTest::keyClick(w, k, m, 40); QTest::qWait(250); };
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

    // LUMINA_THEME=light|oled|dark: run the whole flow (screenshots) in that theme; reset to dark at the end.
    const QString envTheme = qEnvironmentVariable("LUMINA_THEME");
    auto setTheme = [&](const QString &mode) {
        if (auto *sd = find("settingsDialog"))
            QMetaObject::invokeMethod(sd, "setThemeMode", Q_ARG(QVariant, mode));
    };
    if (!envTheme.isEmpty()) setTheme(envTheme);

    // --- home / empty state ---
    shot("home");
    if (doTest) {
        auto *b = find("homeOpenButton");
        result("home open button present", b && b->isVisible() && b->isEnabled());
    }
    // --- media library overlay (isolated DB/settings; folder of copied media) ---
    if (doTest && !files.isEmpty()) {
        QTemporaryDir lib;
        const QString src = QFileInfo::exists("/tmp/short1.mp4") ? QStringLiteral("/tmp/short1.mp4") : files.first();
        bool made = lib.isValid();
        for (const char *n : {"sunrise_alpha.mp4", "Holiday/ocean_beta.mp4", "Holiday/ocean_gamma.mp4"}) {
            QDir().mkpath(QFileInfo(lib.filePath(n)).absolutePath());
            made = made && QFile::copy(src, lib.filePath(n));
        }
        result("library fixture media", made);
        auto *pl0 = p->playlist();
        key(Qt::Key_L);
        result("L opens library", waitFor([&] { return vis("librarySearch"); }));
        p->addLibraryFolder(QUrl::fromLocalFile(lib.path()));
        result("library scan finds files", waitFor([&] { return p->library()->rowCount() == 3 && !p->libraryScanning(); }),
               QString::number(p->library()->rowCount()));
        QTest::qWait(300);
        shot("library");
        for (char c : std::string("ocean")) QTest::keyClick(w, c);
        result("library search filters", waitFor([&] { return p->library()->rowCount() == 2 && vis("libraryRow_1") && !vis("libraryRow_2"); }),
               QString::number(p->library()->rowCount()));
        QTest::qWait(300);
        shot("library-search");
        if (auto *r = find("libraryRow_0")) {
            clickAt(center(r));
            result("library row click plays", waitFor([&] { return !eng->idle() && eng->duration() > 1; }, 8000));
            result("library queues visible results", pl0->rowCount() == 2, QString::number(pl0->rowCount()));
            result("library closes after play", waitFor([&] { return !vis("librarySearch"); }));
        } else result("library row click plays", false, "missing row");
        eng->stop();
        waitFor([&] { return eng->idle(); });
        pl0->clear();
        p->removeLibraryFolder(lib.path());
        result("remove library folder empties library", waitFor([&] { return p->library()->rowCount() == 0; }));
        p->library()->setSearch({});
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
    wake();
    shot("playing-chrome");
    if (!eng->subtitleTracks().isEmpty()) { // subtitle readability / Bengali glyph QA
        for (double t : {3.0, 11.0, 20.0}) {
            eng->seek(t);
            waitFor([&] { return qAbs(eng->position() - t) < 1.5; });
            QTest::qWait(700);
            shot(QStringLiteral("subtitle-%1s").arg(int(t)));
        }
    }
    pause();
    shot("paused");
    eng->play();
    waitFor([&] { return !eng->paused(); });

    if (doTest) {
        const double dur = eng->duration();
        const bool hasChapters = !eng->chapters().isEmpty();
        // Screenshot steps above can leave a popup open; it would swallow the first click.
        key(Qt::Key_Escape);
        key(Qt::Key_Escape);
        // play button
        bool was = eng->paused();
        click("playButton");
        result("play button toggles pause", waitFor([&] { return eng->paused() != was; }));
        click("playButton");
        const bool back = waitFor([&] { return eng->paused() == was; });
        result("play button toggles back", back);
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
        // seek preview: hovering the timeline fetches a thumbnail without disturbing playback
        if (auto *t = find("timeline")) {
            if (eng->paused()) { eng->play(); waitFor([&] { return !eng->paused(); }); }
            wake();
            const QPointF tl = t->mapToScene(QPointF(0, t->height() / 2));
            const auto prop = [&](const char *n) { char *v = mpv_get_property_string(eng->handle(), n); const double d = v ? QString(v).toDouble() : 0; mpv_free(v); return d; };
            QTest::qWait(1500); // let the preceding seek settle before counting drops
            const double drops0 = prop("frame-drop-count") + prop("decoder-frame-drop-count");
            const double pos0 = eng->position();
            QElapsedTimer wall; wall.start();
            QList<qint64> lat;
            bool allReady = true;
            for (int i = 0; i < 8; ++i) {
                const double frac = 0.08 + 0.84 * ((i * 37) % 8) / 8.0 + (i % 3) * 0.013;
                QElapsedTimer l; l.start();
                QTest::mouseMove(w, QPoint(int(tl.x() + t->width() * frac), int(tl.y())));
                QTest::qWait(30);
                const QString want = "/" + QString::number(int(t->property("hoverTime").toDouble() / 2.0));
                auto *img = find("timelineThumb");
                const bool ok = waitFor([&] {
                    img = find("timelineThumb");
                    return img && img->property("status").toInt() == 1 && img->property("source").toUrl().toString().endsWith(want);
                }, 2000);
                lat.append(l.elapsed());
                allReady = allReady && ok;
                QTest::qWait(250);
                if (i == 3) shot("seek-preview");
            }
            std::sort(lat.begin(), lat.end());
            result("timeline hover shows a thumbnail within 2 s", allReady, QString("p50=%1ms max=%2ms").arg(lat[lat.size() / 2]).arg(lat.last()));
            result("hover thumbnail visible", vis("timelineThumb"));
            const double drift = (eng->position() - pos0) - wall.elapsed() / 1000.0 * eng->speed();
            const double drops = prop("frame-drop-count") + prop("decoder-frame-drop-count") - drops0;
            result("hovering does not disturb playback", qAbs(drift) < 1.0 && drops == 0,
                   QString("drift=%1s drops=%2").arg(drift, 0, 'f', 2).arg(drops));
            wake();
        }
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
            {
                // every track row is readable and distinguishable
                QSet<QString> seen;
                bool unique = true, nonEmpty = true;
                for (const auto &list : {eng->audioTracks(), eng->subtitleTracks()}) {
                    seen.clear();
                    for (const QVariant &v : list) {
                        const QVariantMap m = v.toMap();
                        nonEmpty = nonEmpty && !m.value("label").toString().isEmpty();
                        unique = unique && !seen.contains(m.value("label").toString() + "|" + m.value("detail").toString());
                        seen.insert(m.value("label").toString() + "|" + m.value("detail").toString());
                    }
                }
                result("track labels are non-empty and unique", unique && nonEmpty);
                const auto onScreen = [&](const char *n) { auto *it = find(n); return it && it->isVisible() && center(it).y() > 0 && center(it).y() < w->height() - 4; };
                if (onScreen("audioDelayPlus") && onScreen("subDelayPlus")) {
                    const double a0 = eng->audioDelay(), s0 = eng->subtitleDelay();
                    click("audioDelayPlus");
                    result("audio delay + steps by 100 ms", waitFor([&] { return qAbs(eng->audioDelay() - (a0 + 0.1)) < 0.001; }), QString::number(eng->audioDelay()));
                    result("stepping keeps the panel open", vis("audioDelayPlus"));
                    click("audioDelayPlus");
                    click("audioDelayMinus");
                    result("audio delay - steps back", waitFor([&] { return qAbs(eng->audioDelay() - (a0 + 0.1)) < 0.001; }), QString::number(eng->audioDelay()));
                    click("subDelayMinus");
                    result("subtitle delay - steps by 100 ms", waitFor([&] { return qAbs(eng->subtitleDelay() - (s0 - 0.1)) < 0.001; }), QString::number(eng->subtitleDelay()));
                    result("panel still open after both", vis("subDelayMinus"));
                    click("syncReset");
                    result("sync Reset zeroes both delays", waitFor([&] { return eng->audioDelay() == 0 && eng->subtitleDelay() == 0; }));
                } else {
                    qCInfo(lcCore) << "UITEST SKIP delay stepper (scrolled off in this window size)";
                }
            }
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
            // keyboard: Esc closes, Down + Enter picks a row
            if (click("tracksButton")) {
                QTest::qWait(400);
                key(Qt::Key_Escape);
                QTest::qWait(300);
                result("Esc closes the track panel", !vis("subItem_off"));
                if (eng->audioTracks().size() > 1) {
                    click("tracksButton");
                    QTest::qWait(400);
                    const int a1 = eng->audioTrackId();
                    key(Qt::Key_Down);
                    key(Qt::Key_Down);
                    key(Qt::Key_Return);
                    result("Down/Down/Enter selects the third audio row", waitFor([&] { return eng->audioTrackId() != a1 || eng->audioTracks().size() < 3; }, 1500));
                    QTest::qWait(300);
                    result("Enter closes the panel", !vis("subItem_off"));
                }
            }
        }
        // --- A-B loop ---
        eng->play();
        eng->seek(1); waitFor([&] { return qAbs(eng->position() - 1) < 0.5; });
        key(Qt::Key_BracketLeft);
        result("[ sets loop A", waitFor([&] { return eng->loopA() >= 0.5 && eng->loopA() < 2; }), QString::number(eng->loopA()));
        eng->seek(3); waitFor([&] { return qAbs(eng->position() - 3) < 0.5; });
        key(Qt::Key_BracketRight);
        result("] sets loop B", waitFor([&] { return eng->loopB() > eng->loopA(); }), QString::number(eng->loopB()));
        QTest::qWait(300);
        shot("abloop");
        {
            eng->seek(2); waitFor([&] { return qAbs(eng->position() - 2) < 0.5; });
            double prev = eng->position(), maxPos = prev;
            bool wrapped = false;
            for (int i = 0; i < 160 && !wrapped; ++i) { // up to ~4 s
                QTest::qWait(25);
                const double cur = eng->position();
                maxPos = qMax(maxPos, cur);
                wrapped = cur < prev - 0.5;
                prev = cur;
            }
            result("A-B loop wraps position", wrapped && maxPos < eng->loopB() + 0.6,
                   QString("max=%1 B=%2").arg(maxPos).arg(eng->loopB()));
        }
        key(Qt::Key_Backspace);
        result("Backspace clears loop", waitFor([&] { return eng->loopA() < 0 && eng->loopB() < 0; }));

        // --- screenshot + toast ---
        {
            QString toastText;
            auto conn = QObject::connect(p, &PlayerController::toast, [&](const QString &t) { toastText = t; });
            const QString path = p->takeScreenshot();
            result("screenshot file created", !path.isEmpty() && waitFor([&] { return QFileInfo(path).size() > 0; }), path);
            result("screenshot toast emitted", toastText == "Saved screenshot", toastText);
            QTest::qWait(400);
            shot("toast");
            QObject::disconnect(conn);
            if (!path.isEmpty()) QFile::remove(path);
        }

        // --- More menu: aspect / rotate / deinterlace / adjustments ---
        if (click("moreButton")) {
            QTest::qWait(400);
            shot("moremenu");
            if (auto *it = find("aspectItem_16:9")) clickAt(center(it));
            result("More menu sets aspect 16:9", waitFor([&] { return eng->aspect() == "16:9"; }), eng->aspect());
        }
        if (click("moreButton")) {
            QTest::qWait(400);
            if (auto *it = find("rotateItem_90")) clickAt(center(it));
            result("More menu rotates 90", waitFor([&] { return eng->rotate() == 90; }), QString::number(eng->rotate()));
        }
        if (click("moreButton")) {
            QTest::qWait(400);
            if (auto *it = find("deinterlaceItem")) clickAt(center(it));
            result("More menu toggles deinterlace", waitFor([&] { return eng->deinterlace(); }));
        }
        if (click("moreButton")) {
            QTest::qWait(400);
            if (auto *it = find("adjustItem")) clickAt(center(it));
            QTest::qWait(400);
            result("Video adjustments popup opens", vis("adjustPopup") || find("adjust_brightness"));
            eng->setVideoAdjust("brightness", 30);
            QTest::qWait(300);
            shot("adjust");
            result("brightness slider reflects engine", find("adjust_brightness") && qAbs(find("adjust_brightness")->property("value").toDouble() - 30) < 1);
            click("adjustReset");
            result("Reset zeroes adjustments", waitFor([&] { return eng->videoAdjust().value("brightness").toInt() == 0; }));
            key(Qt::Key_Escape);
        }
        eng->setAspect("auto"); eng->setRotate(0); eng->setDeinterlace(false);
        result("aspect/rotate reset", eng->aspect() == "auto" && eng->rotate() == 0);

        // --- equalizer popup ---
        if (click("moreButton")) {
            QTest::qWait(400);
            if (auto *it = find("equalizerItem")) clickAt(center(it));
            QTest::qWait(400);
            result("Equalizer popup opens", find("eqBand_0") != nullptr);
            click("eqPreset_Bass boost");
            result("preset enables EQ and sets gains", waitFor([&] { return eng->property("eqEnabled").toBool() && eng->property("eqGains").toList().value(0).toDouble() == 6; }));
            char *af = mpv_get_property_string(eng->handle(), "af");
            const QString afs = af ? QString::fromUtf8(af) : QString();
            mpv_free(af);
            result("mpv af chain holds equalizer", afs.contains("equalizer"), afs.left(60));
            shot("equalizer");
            click("eqSwitch");
            result("switch disables EQ", waitFor([&] { return !eng->property("eqEnabled").toBool(); }));
            click("eqReset");
            result("Reset zeroes bands", waitFor([&] { return eng->property("eqGains").toList().value(0).toDouble() == 0; }));
            key(Qt::Key_Escape);
        }

        // --- bookmarks ---
        {
            eng->seek(12);
            waitFor([&] { return eng->position() >= 11; }, 4000);
            key(Qt::Key_B);
            result("B adds a bookmark", waitFor([&] { return p->bookmarks().size() == 1; }));
            wake();
            QTest::qWait(300);
            result("bookmark marker on timeline", find("bookmarkMarker") && find("bookmarkMarker")->isVisible());
            eng->seek(2);
            waitFor([&] { return eng->position() < 6; }, 4000);
            if (click("moreButton")) {
                QTest::qWait(400);
                if (auto *it = find("bookmarksItem")) clickAt(center(it));
                QTest::qWait(400);
                shot("bookmarks");
                click("bookmarkRow_0");
                result("bookmark row click seeks", waitFor([&] { return eng->position() >= 11; }, 4000), QString::number(eng->position()));
                click("bookmarkDelete_0");
                result("delete removes bookmark", waitFor([&] { return p->bookmarks().isEmpty(); }));
                key(Qt::Key_Escape);
            }
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

        // --- settings dialog ---
        {
            auto prop = [&](const char *n) {
                char *s = mpv_get_property_string(eng->handle(), n);
                QString r = s ? QString::fromUtf8(s) : QString();
                mpv_free(s);
                return r;
            };
            auto txt = [&](const char *n) { auto *it = find(n); return it ? it->property("text").toString() : QString(); };
            auto setSearch = [&](const QString &t) { if (auto *sf = find("settingsSearch")) sf->setProperty("text", t); QTest::qWait(300); };
            auto page = [&](int i) { return click(QStringLiteral("settingsNav_%1").arg(i).toLatin1().constData()); };
            auto sd = [&] { return find("settingsDialog"); };
            click("settingsButton");
            result("settings button opens dialog", waitFor([&] { return vis("settingsPanel"); }));
            shot("settings-general");

            // sidebar reaches every page (title in the sticky header follows)
            const char *titles[] = {"General", "Playback", "Video", "Audio", "Subtitles", "Appearance", "Library", "Shortcuts", "Advanced", "Mouse", "Rendering", "Network", "Playlist", "About"};
            for (int i = 0; i < 14; ++i) {
                page(i);
                QTest::qWait(200);
                result(QString("sidebar opens page %1").arg(titles[i]),
                       txt("settingsPageTitle") == titles[i] && sd() && sd()->property("currentPage").toInt() == i,
                       txt("settingsPageTitle"));
                shot(QString("settings-page-%1-%2").arg(i).arg(titles[i]).toLower());
            }

            // live language switch: Bengali title + back to English, no restart
            {
                page(0);
                QTest::qWait(200);
                auto pick = [&](int idx) {
                    if (auto *cb = find("languageCombo"))
                        QMetaObject::invokeMethod(cb, "activated", Q_ARG(int, idx));
                    QTest::qWait(400);
                };
                result("language combo present", vis("languageCombo"));
                pick(2); // বাংলা
                result("switch to Bengali relabels the page title", txt("settingsPageTitle") == QStringLiteral("সাধারণ"), txt("settingsPageTitle"));
                shot("settings-general-bn");
                pick(0); // system default (LUMINA_LANG, English in tests)
                result("switch back to default language", txt("settingsPageTitle") == (qEnvironmentVariable("LUMINA_LANG") == "bn" ? QStringLiteral("সাধারণ") : QStringLiteral("General")), txt("settingsPageTitle"));
            }

            // global search: finds a setting on another page, shows it, hides the rest
            page(0);
            setSearch("equalizer");
            result("search shows matching setting from another page", vis("openEqualizer"));
            result("search hides non-matching settings", !vis("resumeSwitch") && !vis("hwdecOption_no"));
            result("search title says results", txt("settingsPageTitle") == "Search results");
            shot("settings-search");
            setSearch("zzzz-nothing");
            result("search with no match shows empty state", vis("settingsNoResults"));
            shot("settings-search-empty");
            setSearch("");
            result("clearing search restores a page", vis("openEqualizer") && !vis("settingsNoResults"));
            page(0);
            result("General page back after search", waitFor([&] { return vis("resumeSwitch"); }));

            page(1);
            QTest::qWait(300);
            click("hwdecOption_no");
            result("hwdec Off applies to engine", waitFor([&] { return prop("hwdec") == "no"; }), prop("hwdec"));
            result("hwdec persisted", p->setting("playback/hwdec").toString() == "no");
            click("hwdecOption_auto-safe");
            result("hwdec restored", waitFor([&] { return prop("hwdec").contains("auto-safe"); }), prop("hwdec"));

            // reset page: change settings, cancel keeps them, confirm restores defaults
            click("seekStepPlus");
            click("seekStepPlus");
            click("autoNextSwitch");
            result("seek step persisted", p->setting("playback/seekStep").toInt() == 7, p->setting("playback/seekStep").toString());
            result("auto-next off persisted", p->setting("playback/autoNext").isValid() && !p->setting("playback/autoNext").toBool());
            click("seekPreviewSwitch");
            result("seek preview off persisted + disables service",
                   p->setting("ui/seekPreview").isValid() && !p->setting("ui/seekPreview").toBool() && !p->thumbs()->available());
            click("settingsReset_1");
            result("reset asks for confirmation", waitFor([&] { return vis("settingsResetConfirm"); }));
            shot("settings-reset-confirm");
            click("settingsResetCancel");
            result("cancel keeps values", !vis("settingsResetConfirm") && p->setting("playback/seekStep").toInt() == 7);
            click("settingsReset_1");
            click("settingsResetConfirm");
            result("reset page restores seek step", waitFor([&] { return p->setting("playback/seekStep").toInt() == 5; }));
            result("reset page restores auto-next", p->setting("playback/autoNext").toBool());
            result("reset page restores seek preview", p->setting("ui/seekPreview").toBool() && p->thumbs()->available());
            result("reset page leaves other pages alone", p->setting("playback/hwdec").toString() == "auto-safe");

            // a new setting changes engine state: seek step drives the arrow keys
            click("seekStepPlus"); // 6 s
            key(Qt::Key_Escape);
            waitFor([&] { return !vis("settingsPanel"); });
            pause();
            eng->seek(10);
            waitFor([&] { return qAbs(eng->position() - 10) < 1.0; });
            double sp0 = eng->position();
            key(Qt::Key_Right);
            result("seek step setting drives Right key", waitFor([&] { return qAbs(eng->position() - (sp0 + 6)) < 1.0; }),
                   QString("%1->%2").arg(sp0).arg(eng->position()));
            key(Qt::Key_Comma, Qt::ControlModifier);
            waitFor([&] { return vis("settingsPanel"); });
            page(1);
            click("seekStepMinus");
            result("seek step minus persisted", p->setting("playback/seekStep").toInt() == 5);
            eng->play();

            // video page: deinterlace switch reaches the engine
            page(2);
            QTest::qWait(300);
            click("deinterlaceSwitch");
            result("deinterlace switch applies to engine", waitFor([&] { return eng->deinterlace(); }));
            result("deinterlace persisted", p->setting("video/deinterlace").toBool());
            click("deinterlaceSwitch");
            result("deinterlace off", waitFor([&] { return !eng->deinterlace(); }));

            page(4);
            QTest::qWait(300);
            click("subSizePlus");
            result("subtitle size applies to engine", waitFor([&] { return qAbs(prop("sub-font-size").toDouble() - 45) < 0.01; }), prop("sub-font-size"));
            result("subtitle size persisted", p->setting("subtitles/fontSize").toInt() == 45);
            click("subColor_FFE45C");
            result("subtitle color applies", waitFor([&] { return prop("sub-color").toUpper().startsWith("#FFFFE45C") || prop("sub-color").toUpper().contains("FFE45C"); }), prop("sub-color"));
            click("subLang_en");
            result("subtitle language preference applies", waitFor([&] { return prop("slang") == "en,eng"; }), prop("slang"));
            click("subSizeMinus");
            click("subColor_FFFFFF");
            click("subLang_bnen");
            shot("settings-subtitles");

            page(5);
            QTest::qWait(300);
            const double hd0 = p->setting("interface/hideDelay", 2.6).toDouble();
            click("hideDelayPlus");
            result("hide delay persisted", qAbs(p->setting("interface/hideDelay").toDouble() - (hd0 + 0.5)) < 0.01,
                   p->setting("interface/hideDelay").toString());
            click("hideDelayMinus");
            shot("settings-appearance");
            // theme + reduced motion
            const QString theme0 = sd() ? sd()->property("activeTheme").toString() : QString();
            click("themeOption_light");
            result("Light theme applies and persists",
                   waitFor([&] { return sd() && sd()->property("activeTheme").toString() == "light"; })
                       && p->setting("interface/theme").toString() == "light");
            shot("settings-appearance-light");
            click("themeOption_oled");
            result("OLED theme applies and persists",
                   waitFor([&] { return sd() && sd()->property("activeTheme").toString() == "oled"; })
                       && p->setting("interface/theme").toString() == "oled");
            shot("settings-appearance-oled");
            click(("themeOption_" + (theme0.isEmpty() ? QStringLiteral("dark") : theme0)).toLatin1().constData());
            result("Theme restored", sd() && waitFor([&] { return sd()->property("activeTheme").toString() == theme0; }));
            result("animations on by default", sd() && sd()->property("animFast").toInt() == 120);
            click("reducedMotionSwitch");
            result("Reduce motion zeroes animation tokens and persists",
                   sd() && waitFor([&] { return sd()->property("animFast").toInt() == 0; }) && sd()->property("animMedium").toInt() == 0
                       && p->setting("interface/reducedMotion").toBool());
            click("reducedMotionSwitch");
            result("Reduce motion off restores tokens",
                   sd() && waitFor([&] { return sd()->property("animFast").toInt() == 120; }) && !p->setting("interface/reducedMotion").toBool());

            // accent colours: presets + custom hex persist; all three modes stay readable (see screenshots)
            if (auto *acc = find("accentSection")) {
                auto inv = [&](const char *m, const QString &arg) {
                    QVariant r;
                    if (arg.isNull()) QMetaObject::invokeMethod(acc, m, Q_RETURN_ARG(QVariant, r));
                    else QMetaObject::invokeMethod(acc, m, Q_RETURN_ARG(QVariant, r), Q_ARG(QVariant, arg));
                    return r;
                };
                const char *modes[] = {"dark", "light", "oled"};
                const char *presets[] = {"violet", "blue", "teal", "green", "orange", "rose", "mono"};
                const char *hexes[] = {"", "#3B82F6", "#14B8A6", "#22C55E", "#F97316", "#F43F5E", "#9AA0AD"};
                for (const char *m : modes) {
                    setTheme(m);
                    for (int i = 0; i < 7; ++i) {
                        click((QByteArray("accentPreset_") + presets[i]).constData());
                        if (i == 0 || i == 3 || i == 6 || QByteArray(m) == "dark")
                            shot(QString("accent-%1-%2").arg(m, presets[i]));
                        if (QByteArray(m) == "dark")
                            result(QString("accent preset %1 persists").arg(presets[i]),
                                   p->setting("interface/accent").toString() == hexes[i]);
                    }
                }
                setTheme("dark");
                inv("setAccent", "#E11D74");
                result("custom accent persists", p->setting("interface/accent").toString() == "#E11D74");
                shot("accent-custom");
                const QString json = inv("themeJson", QString()).toString();
                result("theme JSON has name, mode and accent", json.contains("\"name\"") && json.contains("\"mode\": \"dark\"") && json.contains("#E11D74"), json);
                inv("setAccent", "");
                result("theme JSON import restores accent", inv("applyThemeJson", json).toString().isEmpty()
                                                              && p->setting("interface/accent").toString() == "#E11D74");
                result("bad theme JSON rejected without change", !inv("applyThemeJson", "{\"mode\":\"neon\",\"accent\":\"red\"}").toString().isEmpty()
                                                                  && !inv("applyThemeJson", "garbage").toString().isEmpty()
                                                                  && p->setting("interface/accent").toString() == "#E11D74");
                inv("setAccent", "");
            } else result("accent section present", false);

            // new pages: Mouse, Rendering, Network, Playlist
            page(9); QTest::qWait(300);
            result("mouse page lists click binding", vis("mouse_click") && vis("mouse_edgeV"));
            shot("settings-mouse");
            page(10); QTest::qWait(300);
            result("rendering page shows renderer info and hwdec", vis("rendererInfo") && vis("hwdecOptionRendering_no"));
            shot("settings-rendering");
            page(11); QTest::qWait(300);
            result("network page shows yt-dlp row, quality and timeout", vis("ytdlPath") && vis("ytdlQuality_1080") && vis("networkTimeoutPlus"));
            click("ytdlQuality_720");
            result("ytdl quality applies to the engine and persists", p->setting("network/ytdlQuality").toString() == "720"
                   && prop("ytdl-format").contains("720"), prop("ytdl-format"));
            click("ytdlQuality_1080");
            click("networkBufferingLink");
            result("network buffering link opens Advanced", sd() && waitFor([&] { return sd()->property("currentPage").toInt() == 8; }));
            shot("settings-network");
            page(12); QTest::qWait(300);
            result("playlist page controls", vis("rememberQueueSwitch") && vis("playlistOpenMode_append") && vis("playlistRepeat_1"));
            click("rememberQueueSwitch");
            result("remember queue persists", p->setting("playlist/rememberQueue").toBool());
            click("rememberQueueSwitch");
            result("remember queue off persists", !p->setting("playlist/rememberQueue").toBool());
            shot("settings-playlist");

            // advanced: mpv override box reports errors and applies valid lines
            page(8);
            QTest::qWait(300);
            if (auto *f = find("mpvOptionsField")) f->setProperty("text", "deband=yes\nnot-an-option=1");
            click("mpvOptionsApply");
            result("mpv override applies valid line", waitFor([&] { return prop("deband") == "yes"; }), prop("deband"));
            result("mpv override reports bad line", txt("mpvOptionsStatus").contains("not-an-option"), txt("mpvOptionsStatus"));
            shot("settings-advanced");
            click("settingsReset_8");
            click("settingsResetConfirm");
            result("advanced reset clears override text", waitFor([&] { return p->setting("advanced/mpvOptions").toString().isEmpty(); }));
            eng->setMpvOption("deband", "no");
            page(13);
            QTest::qWait(300);
            shot("settings-about");
            result("about shows mpv version", sd() && sd()->property("about").toMap().value("mpv").toString().contains("mpv"),
                   sd() ? sd()->property("about").toMap().value("mpv").toString() : QString());

            // layout at the minimum window size (sidebar collapses to icons) and on a large display
            const QSize size0 = w->size();
            w->resize(760, 500);
            QTest::qWait(500);
            result("settings fit at 760x500", [&] {
                auto *pn = find("settingsPanel");
                return pn && pn->width() <= 760 && pn->height() <= 500 && pn->x() >= 0 && pn->y() >= 0;
            }());
            for (int i : {1, 7})
                page(i), shot(QString("settings-min-%1").arg(i));
            click("settingsSearch");
            setSearch("subtitle");
            shot("settings-min-search");
            setSearch("");
            w->resize(1920, 1080);
            QTest::qWait(600);
            page(2);
            shot("settings-large-video");
            w->resize(size0);
            QTest::qWait(500);

            key(Qt::Key_Escape);
            result("Esc closes settings", waitFor([&] { return !vis("settingsPanel"); }));
            key(Qt::Key_Comma, Qt::ControlModifier);
            result("Ctrl+, opens settings", waitFor([&] { return vis("settingsPanel"); }));
            // --- rebindable shortcuts: M (mute) -> N, conflict prompt, reset ---
            page(7);
            QTest::qWait(300);
            if (auto *sf = find("shortcutSearch")) sf->setProperty("text", "mute");
            QTest::qWait(300);
            result("shortcuts page lists mute row", vis("shortcutRow_mute"));
            click("shortcutRow_mute");
            QTest::qWait(150);
            shot("settings-shortcuts-capture");
            key(Qt::Key_Space); // already used by play/pause -> conflict prompt, nothing fires or binds
            result("conflicting key shows swap prompt", waitFor([&] { return vis("shortcutSwap"); }));
            shot("settings-shortcuts-conflict");
            click("shortcutConflictCancel");
            result("conflict cancel hides prompt", !vis("shortcutSwap"));
            click("shortcutRow_mute");
            key(Qt::Key_N);
            result("rebinding persisted", p->setting("shortcuts/mute").toString() == "N", p->setting("shortcuts/mute").toString());
            shot("settings-shortcuts-rebound");
            key(Qt::Key_Escape);
            waitFor([&] { return !vis("settingsPanel"); });
            was = eng->muted();
            key(Qt::Key_N);
            result("rebound N toggles mute", eng->muted() != was);
            key(Qt::Key_N);
            was = eng->muted();
            key(Qt::Key_M);
            result("old M no longer toggles mute", eng->muted() == was);
            key(Qt::Key_Comma, Qt::ControlModifier);
            waitFor([&] { return vis("settingsPanel"); });
            page(7);
            QTest::qWait(300);
            if (auto *sf = find("shortcutSearch")) sf->setProperty("text", "mute");
            QTest::qWait(300);
            click("shortcutReset_mute");
            key(Qt::Key_Escape);
            waitFor([&] { return !vis("settingsPanel"); });
            was = eng->muted();
            key(Qt::Key_M);
            result("reset restores M", eng->muted() != was);
            key(Qt::Key_M);
            result("reset removes N binding", !p->setting("shortcuts/mute").isValid());
            key(Qt::Key_Comma, Qt::ControlModifier);
            waitFor([&] { return vis("settingsPanel"); });
            key(Qt::Key_Escape);
            waitFor([&] { return !vis("settingsPanel"); });
        }

        // --- mini player ---
        {
            const QRect geom0 = w->geometry();
            const bool top0 = w->flags() & Qt::WindowStaysOnTopHint;
            key(Qt::Key_M, Qt::ControlModifier);
            result("Ctrl+M enters mini mode", waitFor([&] { return w->width() == 480; }), QString::number(w->width()));
            result("mini mode is always on top", waitFor([&] { return w->flags() & Qt::WindowStaysOnTopHint; }));
            result("mini keeps aspect ratio", w->height() > 150 && w->height() < 400, QString::number(w->height()));
            result("mini hides top bar and controls", !vis("controlsBar"));
            wake();
            QTest::qWait(300);
            shot("mini");
            result("mini play button on hover", vis("miniPlayButton"));
            click("miniExpandButton");
            result("expand restores size", waitFor([&] { return w->width() == geom0.width() && w->height() == geom0.height(); }),
                   QStringLiteral("%1x%2").arg(w->width()).arg(w->height()));
            result("expand restores on-top flag", waitFor([&] { return bool(w->flags() & Qt::WindowStaysOnTopHint) == top0; }));
            key(Qt::Key_M, Qt::ControlModifier);
            waitFor([&] { return w->width() == 480; });
            key(Qt::Key_M, Qt::ControlModifier);
            result("Ctrl+M leaves mini mode", waitFor([&] { return w->width() == geom0.width(); }));
        }

        // --- playlist (queue: argv file + short1 + short2, kept paused) ---
        auto *pl = p->playlist();
        p->setShuffle(false); p->setRepeatMode(0); // persisted settings must not leak into the test
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

        // continue-watching cards: play, seek, persist, return home, click card -> resumes
        p->openFiles({QUrl::fromLocalFile(files.first())});
        if (waitFor([&] { return eng->duration() > 40; }, 6000)) {
            QTest::qWait(1000); // let the automatic resume-seek settle first
            eng->seek(20);
            result("seek before home cards", waitFor([&] { return eng->position() >= 19; }, 6000));
            p->saveResume();
            pl->clear();
            eng->stop();
            waitFor([&] { return eng->idle(); });
            result("continue card present after playing", waitFor([&] { return p->continueWatching()->rowCount() > 0 && vis("continueCard_0"); }));
            QTest::qWait(400);
            shot("home-cards");
            if (auto *c = find("continueCard_0")) {
                clickAt(center(c));
                result("continue card resumes", waitFor([&] { return !eng->idle() && eng->position() >= 15; }, 8000),
                       QString("pos=%1").arg(eng->position()));
            } else result("continue card resumes", false, "missing");
            pause();
            p->saveResume();
            pl->clear();
            eng->stop();
            waitFor([&] { return eng->idle(); });
            QTest::qWait(400);
            if (auto *star = find("cardStar_0")) {
                clickAt(center(star));
                result("star adds favorite", waitFor([&] { return p->favorites()->rowCount() == 1 && vis("favoritesList"); }));
                shot("home-favorites");
            } else result("star adds favorite", false, "missing");
        } else qCInfo(lcCore) << "UITEST SKIP continue card flow (needs media >= 40s, e.g. /tmp/long.mp4)";

        auto typeText = [&](const QString &t) { for (QChar c : t) QTest::keyClick(w, c.toLatin1(), {}, 5); };
        // --- Open URL dialog + network playback (local python http.server) ---
        pl->clear();
        eng->stop();
        waitFor([&] { return eng->idle(); });
        QTest::qWait(400);
        key(Qt::Key_U, Qt::ControlModifier);
        result("Ctrl+U opens URL dialog", waitFor([&] { return vis("urlField"); }));
        QTest::qWait(300);
        typeText("not a url");
        key(Qt::Key_Return);
        result("invalid URL rejected", vis("urlField") && waitFor([&] { return vis("urlError"); }) && eng->idle());
        shot("open-url-invalid");
        key(Qt::Key_A, Qt::ControlModifier);
        // Tiny in-process HTTP server with Range support (python's http.server can't seek, so mp4s with a trailing moov fail).
        QTcpServer httpd;
        const QString mediaPath = files.first();
        QObject::connect(&httpd, &QTcpServer::newConnection, &httpd, [&] {
            while (QTcpSocket *sock = httpd.nextPendingConnection()) {
                QObject::connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
                QObject::connect(sock, &QTcpSocket::readyRead, sock, [sock, mediaPath] {
                    if (!sock->property("done").isNull() || !sock->peek(8192).contains("\r\n\r\n")) return;
                    sock->setProperty("done", true);
                    const QString req = QString::fromLatin1(sock->readAll());
                    QFile f(mediaPath);
                    if (!f.open(QIODevice::ReadOnly))
                        return;
                    qint64 from = 0, to = f.size() - 1;
                    bool ranged = false;
                    const auto m = QRegularExpression("Range: bytes=(\\d+)-(\\d*)", QRegularExpression::CaseInsensitiveOption).match(req);
                    if (m.hasMatch()) {
                        ranged = true;
                        from = m.captured(1).toLongLong();
                        if (!m.captured(2).isEmpty()) to = qMin<qint64>(to, m.captured(2).toLongLong());
                    }
                    QByteArray head = ranged ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n";
                    head += "Accept-Ranges: bytes\r\nContent-Type: video/mp4\r\nConnection: close\r\nContent-Length: "
                            + QByteArray::number(to - from + 1) + "\r\n";
                    if (ranged) head += "Content-Range: bytes " + QByteArray::number(from) + "-" + QByteArray::number(to)
                                        + "/" + QByteArray::number(f.size()) + "\r\n";
                    sock->write(head + "\r\n");
                    if (!req.startsWith("HEAD")) { f.seek(from); sock->write(f.read(to - from + 1)); }
                    sock->disconnectFromHost();
                });
            }
        });
        if (httpd.listen(QHostAddress::LocalHost, 0)) {
            const QString port = QString::number(httpd.serverPort());
            const QString url = QStringLiteral("http://127.0.0.1:%1/%2").arg(port, QFileInfo(files.first()).fileName());
            typeText("  " + url + " ");
            QTest::qWait(300);
            shot("open-url-valid");
            key(Qt::Key_Return);
            result("URL dialog closes on open", waitFor([&] { return !vis("urlField"); }));
            eng->play(); // engine may still be paused from the playlist tests
            result("http URL plays", waitFor([&] { return !eng->idle() && eng->duration() > 1 && eng->position() > 0.5; }, 15000),
                   eng->mediaError());
            result("remote playlist entry titled by file name", pl->rowCount() == 1
                   && pl->data(pl->index(0), PlaylistModel::TitleRole).toString() == QFileInfo(files.first()).fileName());
            result("URL is in recent URLs", p->recentUrls().contains(url));
            result("URL history row, no resume key collision", p->history()->rowCount() > 0);
            QTest::qWait(800);
            shot("network-playing");
            eng->stop();
            waitFor([&] { return eng->idle(); });
        } else qCInfo(lcCore) << "UITEST SKIP http URL playback (cannot listen)";
        httpd.close();

        // --- per-file profile: adjust, reopen with engine reset in between, expect restore ---
        p->setRememberFileSettings(true);
        p->openFiles({QUrl::fromLocalFile(files.first())});
        waitFor([&] { return !eng->idle() && eng->duration() > 1; }, 6000);
        QTest::qWait(1800); // profile tracking arms ~1 s after load
        eng->setVideoAdjust("brightness", 25);
        eng->setSpeed(1.5);
        eng->setAspect("4:3");
        QTest::qWait(300);
        p->saveResume();
        pl->clear();
        eng->stop();
        waitFor([&] { return eng->idle(); });
        eng->resetVideoAdjust();
        eng->setSpeed(1.0);
        eng->setAspect("auto");
        QTest::qWait(300);
        p->openFiles({QUrl::fromLocalFile(files.first())});
        waitFor([&] { return !eng->idle() && eng->duration() > 1; }, 6000);
        QTest::qWait(500);
        result("profile restored after reopen",
               eng->videoAdjust().value("brightness").toInt() == 25 && eng->speed() == 1.5 && eng->aspect() == "4:3",
               QStringLiteral("b=%1 speed=%2 aspect=%3").arg(eng->videoAdjust().value("brightness").toInt()).arg(eng->speed()).arg(eng->aspect()));
        shot("profile-restored");
        p->resetFileSettings();
        QTest::qWait(300);
        result("Reset for this file restores defaults",
               eng->videoAdjust().value("brightness").toInt() == 0 && eng->speed() == 1.0 && eng->aspect() == "auto");
        pl->clear();
        eng->stop();
        waitFor([&] { return eng->idle(); });
        p->openFiles({QUrl::fromLocalFile(files.first())});
        waitFor([&] { return !eng->idle() && eng->duration() > 1; }, 6000);
        QTest::qWait(500);
        result("reset profile stays cleared on reopen", eng->videoAdjust().value("brightness").toInt() == 0 && eng->speed() == 1.0);
        // --- mouse bindings (Settings > Mouse): dispatcher, persistence, modal popups swallow outside clicks ---
        {
            auto *ma = find("mouseActions");
            auto call = [&](const char *m, const QVariant &a, const QVariant &b = {}) {
                QVariant r;
                if (b.isValid()) QMetaObject::invokeMethod(ma, m, Q_RETURN_ARG(QVariant, r), Q_ARG(QVariant, a), Q_ARG(QVariant, b));
                else QMetaObject::invokeMethod(ma, m, Q_RETURN_ARG(QVariant, r), Q_ARG(QVariant, a));
                return r;
            };
            if (!ma) result("mouse dispatcher present", false);
            else {
                p->openFiles({QUrl::fromLocalFile(files.first())});
                waitFor([&] { return !eng->idle() && eng->duration() > 1; }, 6000);
                w->setProperty("playlistOpen", false);
                eng->setMuted(false);
                qCInfo(lcCore) << "UITEST mouse state: settings" << vis("settingsPanel") << "menu" << w->property("menuOpen")
                               << "fs" << hasFs() << "mini" << w->property("miniMode") << "ma" << vis("mouseActions");
                call("run", "mute");
                result("dispatch mute", waitFor([&] { return eng->muted(); }));
                call("run", "mute");
                result("dispatch mute again unmutes", waitFor([&] { return !eng->muted(); }));
                result("invalid binding rejected", !call("bindAction", "click", "explode").toBool());
                result("binding persists", call("bindAction", "click", "mute").toBool() && p->setting("mouse/click").toString() == "mute");
                call("bindAction", "dblclick", "none");
                const QPoint vp(w->width() / 2, w->height() / 3);
                wake();
                clickAt(vp);
                result("single click runs bound action (muted)", waitFor([&] { return eng->muted(); }));
                // modal menu: clicking outside only closes it, never runs the click action
                click("moreButton");
                result("more menu opens", waitFor([&] { return w->property("menuOpen").toBool(); }));
                clickAt(vp);
                result("click outside a popup runs no action", waitFor([&] { return !w->property("menuOpen").toBool(); }) && eng->muted());
                wake();
                clickAt(vp);
                result("click works again after the menu closed", waitFor([&] { return !eng->muted(); }));
                // wheel: bound to seek, one notch = seekStep
                call("bindAction", "wheel", "seek");
                eng->seek(10);
                waitFor([&] { return eng->position() > 9.5; });
                const double before = eng->position();
                call("wheelAction", "seek", 120);
                result("wheel bound to seek moves the playhead", waitFor([&] { return eng->position() > before + 2; }),
                       QString::number(eng->position()));
                call("bindAction", "wheel", "volume");
                call("bindAction", "click", "playpause");
                call("bindAction", "dblclick", "fullscreen");
                result("defaults restored", call("binding", "click").toString() == "playpause" && call("binding", "wheel").toString() == "volume");
                p->setSetting("mouse/click", "playpause");
            }
        }
        pause();
        qCInfo(lcCore) << "UITEST SUMMARY passes=" << passes << "fails=" << fails;
    }
    if (!envTheme.isEmpty()) setTheme("dark");
    QCoreApplication::exit(fails ? 1 : 0);
}
