#include "utilities/AudioFilters.h"

#include <QTest>

using namespace Lumina::AudioFilters;

class TstAudioFilters : public QObject
{
    Q_OBJECT
private slots:
    void normalization()
    {
        QVERIFY(normalizationFilter("off").isEmpty());
        QVERIFY(normalizationFilter("bogus").isEmpty());
        QVERIFY(normalizationFilter("night").startsWith("dynaudnorm"));
        QVERIFY(normalizationFilter("movie").startsWith("loudnorm"));
    }
    void chainComposes()
    {
        const QStringList eq{"equalizer=f=31:t=o:w=1:g=3.0"};
        QCOMPARE(chain({}, "off"), QString());
        QCOMPARE(afValue(chain({}, "off")), QString());
        QVERIFY(chain(eq, "off").startsWith("equalizer"));
        QVERIFY(!chain(eq, "off").contains("dynaudnorm"));
        const QString both = chain(eq, "night");
        QVERIFY(both.indexOf("equalizer") < both.indexOf("dynaudnorm")); // neither clobbers the other
        QVERIFY(afValue(chain({}, "movie")).startsWith("lavfi=[loudnorm"));
    }
    void visualization()
    {
        QVERIFY(visualizationGraph("off", "").isEmpty());
        QCOMPARE(visualizationGraph("spectrum", ""),
                 QString("[aid1]asplit[ao][t];[t]showcqt=s=1280x720:fps=30,format=yuv420p[vo]"));
        QVERIFY(visualizationGraph("waveform", "loudnorm=I=-16").startsWith("[aid1]loudnorm=I=-16,asplit"));
    }
};

QTEST_APPLESS_MAIN(TstAudioFilters)
#include "tst_AudioFilters.moc"
