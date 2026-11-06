#include "ShortcutManager.h"

#include <QCoreApplication>
#include <QKeySequence>
#include <QSettings>
#include <QVariantMap>

namespace {
QString norm(const QString &s) { return QKeySequence(s, QKeySequence::PortableText).toString(QKeySequence::PortableText); }
QString settingsKey(const QString &id) { return QStringLiteral("shortcuts/") + id; }
}

// Names are kept as English source text and translated on read, so a live
// language switch only needs changed() (see retranslate()).
QString ShortcutManager::trSrc(const QString &src) { return QCoreApplication::translate("Shortcuts", src.toUtf8().constData()); }

ShortcutManager::ShortcutManager(QObject *parent) : QObject(parent)
{
    // id, name, category, default, needs media
    const struct { const char *id, *name, *cat, *key; bool media; } table[] = {
        {"playPause",    QT_TRANSLATE_NOOP("Shortcuts", "Play / pause"),         QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "Space", true},
        {"seekBack",     QT_TRANSLATE_NOOP("Shortcuts", "Seek back 5 s"),        QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "Left", true},
        {"seekForward",  QT_TRANSLATE_NOOP("Shortcuts", "Seek forward 5 s"),     QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "Right", true},
        {"frameForward", QT_TRANSLATE_NOOP("Shortcuts", "Next frame"),           QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  ".", true},
        {"frameBack",    QT_TRANSLATE_NOOP("Shortcuts", "Previous frame"),       QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  ",", true},
        {"speedDown",    QT_TRANSLATE_NOOP("Shortcuts", "Slower"),               QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "X", true},
        {"speedUp",      QT_TRANSLATE_NOOP("Shortcuts", "Faster"),               QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "C", true},
        {"speedReset",   QT_TRANSLATE_NOOP("Shortcuts", "Normal speed"),         QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "Z", true},
        {"loopA",        QT_TRANSLATE_NOOP("Shortcuts", "Set loop start (A)"),   QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "[", true},
        {"loopB",        QT_TRANSLATE_NOOP("Shortcuts", "Set loop end (B)"),     QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "]", true},
        {"loopClear",    QT_TRANSLATE_NOOP("Shortcuts", "Clear A-B loop"),       QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "Backspace", true},
        {"chapterNext",  QT_TRANSLATE_NOOP("Shortcuts", "Next chapter"),         QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "PgDown", true},
        {"chapterPrev",  QT_TRANSLATE_NOOP("Shortcuts", "Previous chapter"),     QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "PgUp", true},
        {"bookmark",     QT_TRANSLATE_NOOP("Shortcuts", "Add bookmark"),         QT_TRANSLATE_NOOP("Shortcuts", "Playback"),  "B", true},
        {"volumeUp",     QT_TRANSLATE_NOOP("Shortcuts", "Volume up"),            QT_TRANSLATE_NOOP("Shortcuts", "Audio"),     "Up", true},
        {"volumeDown",   QT_TRANSLATE_NOOP("Shortcuts", "Volume down"),          QT_TRANSLATE_NOOP("Shortcuts", "Audio"),     "Down", true},
        {"mute",         QT_TRANSLATE_NOOP("Shortcuts", "Mute"),                 QT_TRANSLATE_NOOP("Shortcuts", "Audio"),     "M", true},
        {"audioTrack",   QT_TRANSLATE_NOOP("Shortcuts", "Next audio track"),     QT_TRANSLATE_NOOP("Shortcuts", "Audio"),     "A", true},
        {"subTrack",     QT_TRANSLATE_NOOP("Shortcuts", "Next subtitle track"),  QT_TRANSLATE_NOOP("Shortcuts", "Subtitles"), "S", true},
        {"subShow",      QT_TRANSLATE_NOOP("Shortcuts", "Show subtitles"),       QT_TRANSLATE_NOOP("Shortcuts", "Subtitles"), "W", true},
        {"subHide",      QT_TRANSLATE_NOOP("Shortcuts", "Hide subtitles"),       QT_TRANSLATE_NOOP("Shortcuts", "Subtitles"), "Shift+W", true},
        {"subDelayDown", QT_TRANSLATE_NOOP("Shortcuts", "Subtitle delay -0.1 s"), QT_TRANSLATE_NOOP("Shortcuts", "Subtitles"), "G", true},
        {"subDelayUp",   QT_TRANSLATE_NOOP("Shortcuts", "Subtitle delay +0.1 s"), QT_TRANSLATE_NOOP("Shortcuts", "Subtitles"), "H", true},
        {"zoomIn",       QT_TRANSLATE_NOOP("Shortcuts", "Zoom in"),              QT_TRANSLATE_NOOP("Shortcuts", "Video"),     "+", true},
        {"zoomOut",      QT_TRANSLATE_NOOP("Shortcuts", "Zoom out"),             QT_TRANSLATE_NOOP("Shortcuts", "Video"),     "-", true},
        {"zoomReset",    QT_TRANSLATE_NOOP("Shortcuts", "Reset zoom"),           QT_TRANSLATE_NOOP("Shortcuts", "Video"),     "0", true},
        {"fullscreen",   QT_TRANSLATE_NOOP("Shortcuts", "Toggle fullscreen"),    QT_TRANSLATE_NOOP("Shortcuts", "Window"),    "F", false},
        {"exitFullscreen", QT_TRANSLATE_NOOP("Shortcuts", "Exit fullscreen"),    QT_TRANSLATE_NOOP("Shortcuts", "Window"),    "Esc", false},
        {"alwaysOnTop",  QT_TRANSLATE_NOOP("Shortcuts", "Always on top"),        QT_TRANSLATE_NOOP("Shortcuts", "Window"),    "T", false},
        {"stats",        QT_TRANSLATE_NOOP("Shortcuts", "Playback statistics"),  QT_TRANSLATE_NOOP("Shortcuts", "Window"),    "I", true},
        {"screenshot",   QT_TRANSLATE_NOOP("Shortcuts", "Take screenshot"),      QT_TRANSLATE_NOOP("Shortcuts", "Window"),    "Ctrl+S", true},
        {"openFile",     QT_TRANSLATE_NOOP("Shortcuts", "Open file"),            QT_TRANSLATE_NOOP("Shortcuts", "Library"),   "Ctrl+O", false},
        {"openUrl",      QT_TRANSLATE_NOOP("Shortcuts", "Open URL"),             QT_TRANSLATE_NOOP("Shortcuts", "Library"),   "Ctrl+U", false},
        {"library",      QT_TRANSLATE_NOOP("Shortcuts", "Open library"),         QT_TRANSLATE_NOOP("Shortcuts", "Library"),   "L", false},
        {"settings",     QT_TRANSLATE_NOOP("Shortcuts", "Open settings"),        QT_TRANSLATE_NOOP("Shortcuts", "Library"),   "Ctrl+,", false},
        {"playlist",     QT_TRANSLATE_NOOP("Shortcuts", "Toggle playlist"),      QT_TRANSLATE_NOOP("Shortcuts", "Playlist"),  "P", false},
        {"next",         QT_TRANSLATE_NOOP("Shortcuts", "Next in playlist"),     QT_TRANSLATE_NOOP("Shortcuts", "Playlist"),  "Ctrl+Right", true},
        {"previous",     QT_TRANSLATE_NOOP("Shortcuts", "Previous in playlist"), QT_TRANSLATE_NOOP("Shortcuts", "Playlist"),  "Ctrl+Left", true},
        {"shuffle",      QT_TRANSLATE_NOOP("Shortcuts", "Toggle shuffle"),       QT_TRANSLATE_NOOP("Shortcuts", "Playlist"),  "Shift+S", false},
        {"repeat",       QT_TRANSLATE_NOOP("Shortcuts", "Cycle repeat mode"),    QT_TRANSLATE_NOOP("Shortcuts", "Playlist"),  "R", false},
    };
    QSettings s;
    for (const auto &t : table) {
        Action a{QString::fromLatin1(t.id), QString::fromLatin1(t.name),
                 QString::fromLatin1(t.cat), norm(QString::fromLatin1(t.key)), {}, t.media};
        a.key = s.contains(settingsKey(a.id)) ? norm(s.value(settingsKey(a.id)).toString()) : a.defKey;
        m_actions.append(a);
    }
}

ShortcutManager::Action *ShortcutManager::find(const QString &id)
{
    for (auto &a : m_actions) if (a.id == id) return &a;
    return nullptr;
}
const ShortcutManager::Action *ShortcutManager::find(const QString &id) const
{
    for (const auto &a : m_actions) if (a.id == id) return &a;
    return nullptr;
}

QVariantList ShortcutManager::actions() const
{
    QVariantList l;
    for (const auto &a : m_actions)
        l << QVariantMap{{"id", a.id}, {"name", trSrc(a.name)}, {"category", trSrc(a.category)}, {"defaultKey", a.defKey},
                         {"key", a.key}, {"media", a.media}};
    return l;
}

void ShortcutManager::setCapturing(bool c)
{
    if (c == m_capturing) return;
    m_capturing = c;
    emit capturingChanged();
}

QString ShortcutManager::sequence(const QString &id) const { auto *a = find(id); return a ? a->key : QString(); }
QString ShortcutManager::defaultSequence(const QString &id) const { auto *a = find(id); return a ? a->defKey : QString(); }
QString ShortcutManager::nameOf(const QString &id) const { auto *a = find(id); return a ? trSrc(a->name) : QString(); }

QString ShortcutManager::conflict(const QString &id, const QString &seq) const
{
    const QString n = norm(seq);
    if (n.isEmpty()) return {};
    for (const auto &a : m_actions)
        if (a.id != id && a.key == n) return a.id;
    return {};
}

void ShortcutManager::store(const Action &a)
{
    QSettings s;
    if (a.key == a.defKey) s.remove(settingsKey(a.id));
    else s.setValue(settingsKey(a.id), a.key); // "" = deliberately unbound
}

QString ShortcutManager::setSequence(const QString &id, const QString &seq)
{
    Action *a = find(id);
    if (!a) return {};
    a->key = norm(seq);
    store(*a);
    emit changed();
    return a->key;
}

void ShortcutManager::swap(const QString &id, const QString &seq)
{
    Action *a = find(id);
    if (!a) return;
    if (Action *other = find(conflict(id, seq))) {
        other->key = a->key;
        store(*other);
    }
    setSequence(id, seq);
}

void ShortcutManager::reset(const QString &id)
{
    if (Action *a = find(id)) { a->key = a->defKey; store(*a); emit changed(); }
}

void ShortcutManager::resetAll()
{
    for (auto &a : m_actions) { a.key = a.defKey; store(a); }
    emit changed();
}

QString ShortcutManager::fromKeyEvent(int key, int modifiers) const
{
    if (key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Meta
        || key == Qt::Key_AltGr || key == Qt::Key_unknown)
        return {};
    return QKeySequence(key | (modifiers & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)))
        .toString(QKeySequence::PortableText);
}
