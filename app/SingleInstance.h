#pragma once

#include <QObject>
#include <QStringList>

class QLocalServer;

// Ensures one running Lumina per user. A second launch forwards its file
// arguments to the first instance over a local socket and then exits.
class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(const QString &name, QObject *parent = nullptr);

    // Sends arguments to an already-running instance. True if one accepted them.
    bool forward(const QStringList &arguments) const;

    // Becomes the primary instance (removes any stale socket first).
    bool listen();

signals:
    // Emitted in the primary instance; arguments may be empty (just raise).
    void argumentsReceived(const QStringList &arguments);

private:
    QString m_name;
    QLocalServer *m_server = nullptr;
};
