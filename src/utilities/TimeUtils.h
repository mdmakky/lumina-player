#pragma once

#include <QObject>
#include <QString>

namespace Lumina {

// Small formatting helpers shared between C++ and QML.
class TimeUtils : public QObject
{
    Q_OBJECT

public:
    explicit TimeUtils(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    // Formats seconds as "m:ss" or "h:mm:ss"; invalid input yields "--:--".
    Q_INVOKABLE static QString formatDuration(double seconds);
};

} // namespace Lumina
