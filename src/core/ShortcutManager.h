#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

// Rebindable keyboard shortcuts (spec §10). Holds the action table, user
// overrides (QSettings "shortcuts/<id>"), and conflict detection. QML binds a
// Shortcut per entry of `actions` and dispatches by id; sequences are
// QKeySequence PortableText strings ("Ctrl+S"); an empty string means unbound.
class ShortcutManager : public QObject
{
    Q_OBJECT
    // List of {id, name, category, defaultKey, key, media(bool: needs loaded media)}.
    Q_PROPERTY(QVariantList actions READ actions NOTIFY changed)
    // True while the settings page is capturing a key press (suspends all shortcuts).
    Q_PROPERTY(bool capturing READ capturing WRITE setCapturing NOTIFY capturingChanged)

public:
    explicit ShortcutManager(QObject *parent = nullptr);

    QVariantList actions() const;
    bool capturing() const { return m_capturing; }
    void setCapturing(bool c);

    Q_INVOKABLE QString sequence(const QString &id) const;
    Q_INVOKABLE QString defaultSequence(const QString &id) const;
    // Id of another action already using `seq`, or "" (empty seq never conflicts).
    Q_INVOKABLE QString conflict(const QString &id, const QString &seq) const;
    Q_INVOKABLE void retranslate() { emit changed(); }
    Q_INVOKABLE QString nameOf(const QString &id) const;
    // Binds without checking conflicts (see conflict()/swap()). Returns the normalized sequence.
    Q_INVOKABLE QString setSequence(const QString &id, const QString &seq);
    // Binds `seq` to `id`; the conflicting action (if any) gets id's previous sequence.
    Q_INVOKABLE void swap(const QString &id, const QString &seq);
    Q_INVOKABLE void reset(const QString &id);
    Q_INVOKABLE void resetAll();
    // Re-reads every binding from QSettings (after a settings import).
    Q_INVOKABLE void reload();
    // Key event -> normalized sequence; "" for modifier-only keys.
    Q_INVOKABLE QString fromKeyEvent(int key, int modifiers) const;

signals:
    void changed();
    void capturingChanged();

private:
    static QString trSrc(const QString &src);
    struct Action { QString id, name, category, defKey, key; bool media; };
    Action *find(const QString &id);
    const Action *find(const QString &id) const;
    void store(const Action &a);

    QVector<Action> m_actions;
    bool m_capturing = false;
};
