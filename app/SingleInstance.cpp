#include "SingleInstance.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QUrl>

SingleInstance::SingleInstance(const QString &name, QObject *parent)
    : QObject(parent)
    , m_name(name)
{
}

bool SingleInstance::forward(const QStringList &arguments) const
{
    QLocalSocket socket;
    socket.connectToServer(m_name);
    if (!socket.waitForConnected(500))
        return false;

    // The receiver has a different cwd: make local paths absolute.
    QJsonArray array;
    for (const QString &arg : arguments) {
        const QUrl url(arg);
        array.append(url.scheme().size() > 1 ? arg : QFileInfo(arg).absoluteFilePath());
    }
    socket.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
    socket.flush();
    socket.disconnectFromServer();
    return socket.state() == QLocalSocket::UnconnectedState
           || socket.waitForDisconnected(1000);
}

bool SingleInstance::listen()
{
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption); // 0700 socket in shared /tmp on Linux
    if (!m_server->listen(m_name)) {
        QLocalServer::removeServer(m_name); // stale socket from a crash
        if (!m_server->listen(m_name))
            return false;
    }

    connect(m_server, &QLocalServer::newConnection, this, [this]() {
        while (QLocalSocket *socket = m_server->nextPendingConnection()) {
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QLocalSocket::disconnected, this, [this, socket]() {
                QStringList args;
                const QJsonArray array = QJsonDocument::fromJson(socket->readAll()).array();
                for (const QJsonValue &v : array)
                    args << v.toString();
                emit argumentsReceived(args);
            });
        }
    });
    return true;
}
