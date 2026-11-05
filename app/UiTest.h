#pragma once

#include <QObject>
#include <QStringList>

class QQuickWindow;
class PlayerController;

// Optional in-app UI driver (compiled when LUMINA_ENABLE_UITEST).
//  LUMINA_SCREENSHOT=<dir>  saves PNGs of the real window into <dir>.
//  LUMINA_UI_TEST=1         injects real mouse/key events and asserts effects.
// Runs on the GUI thread, waiting via QTest::qWait (event loop keeps spinning).
class UiTest : public QObject
{
    Q_OBJECT
public:
    UiTest(QQuickWindow *window, PlayerController *player, QStringList files, QObject *parent);
    static bool requested();
    void start(); // schedules run(); exits the app with the verdict

private:
    void run();
    int fails = 0, passes = 0;
    QQuickWindow *w;
    PlayerController *p;
    QStringList files;
};
