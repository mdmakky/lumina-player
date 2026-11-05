#include <QtTest>

#include "utilities/TimeUtils.h"

using Lumina::TimeUtils;

class TestTimeUtils : public QObject
{
    Q_OBJECT

private slots:
    void formatsShortDurations();
    void formatsHourLongDurations();
    void handlesInvalidInput();
    void roundsToNearestSecond();
};

void TestTimeUtils::formatsShortDurations()
{
    QCOMPARE(TimeUtils::formatDuration(0.0), QStringLiteral("0:00"));
    QCOMPARE(TimeUtils::formatDuration(5.0), QStringLiteral("0:05"));
    QCOMPARE(TimeUtils::formatDuration(59.9), QStringLiteral("1:00"));
    QCOMPARE(TimeUtils::formatDuration(83.0), QStringLiteral("1:23"));
    QCOMPARE(TimeUtils::formatDuration(3599.0), QStringLiteral("59:59"));
}

void TestTimeUtils::formatsHourLongDurations()
{
    QCOMPARE(TimeUtils::formatDuration(3600.0), QStringLiteral("1:00:00"));
    QCOMPARE(TimeUtils::formatDuration(3661.0), QStringLiteral("1:01:01"));
    QCOMPARE(TimeUtils::formatDuration(7325.4), QStringLiteral("2:02:05"));
}

void TestTimeUtils::handlesInvalidInput()
{
    QCOMPARE(TimeUtils::formatDuration(-1.0), QStringLiteral("--:--"));
    QCOMPARE(TimeUtils::formatDuration(qQNaN()), QStringLiteral("--:--"));
    QCOMPARE(TimeUtils::formatDuration(qInf()), QStringLiteral("--:--"));
}

void TestTimeUtils::roundsToNearestSecond()
{
    QCOMPARE(TimeUtils::formatDuration(2.4), QStringLiteral("0:02"));
    QCOMPARE(TimeUtils::formatDuration(2.6), QStringLiteral("0:03"));
}

QTEST_MAIN(TestTimeUtils)
#include "tst_TimeUtils.moc"
