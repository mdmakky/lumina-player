// Every string in lumina_bn.ts must be translated (no "unfinished"/empty) and
// keep the same %1/%n placeholders as its English source.
#include <QFile>
#include <QRegularExpression>
#include <QTest>
#include <QXmlStreamReader>

class TstTranslations : public QObject
{
    Q_OBJECT
private slots:
    void bengaliIsComplete()
    {
        QFile f(QStringLiteral(LUMINA_TS_DIR "/lumina_bn.ts"));
        QVERIFY(f.open(QIODevice::ReadOnly));
        QXmlStreamReader xml(&f);
        const QRegularExpression ph(QStringLiteral("%(n|\\d)"));
        auto placeholders = [&](const QString &s) {
            QStringList l;
            for (auto it = ph.globalMatch(s); it.hasNext();)
                l << it.next().captured();
            l.sort();
            l.removeDuplicates();
            return l;
        };
        int count = 0;
        QString source;
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement())
                continue;
            if (xml.name() == u"source") {
                source = xml.readElementText();
            } else if (xml.name() == u"translation") {
                const bool unfinished = xml.attributes().value(QStringLiteral("type")) == QLatin1String("unfinished");
                QStringList forms;
                while (!(xml.isEndElement() && xml.name() == u"translation")) {
                    xml.readNext();
                    if (xml.isStartElement() && xml.name() == u"numerusform")
                        forms << xml.readElementText();
                    else if (xml.isCharacters() && !xml.isWhitespace())
                        forms << xml.text().toString();
                }
                ++count;
                QVERIFY2(!unfinished, qPrintable("unfinished: " + source));
                QVERIFY2(!forms.isEmpty(), qPrintable("empty: " + source));
                for (const QString &t : forms) {
                    QVERIFY2(!t.isEmpty(), qPrintable("empty: " + source));
                    QVERIFY2(placeholders(t) == placeholders(source), qPrintable("placeholders differ: " + source));
                }
            }
        }
        QVERIFY2(!xml.hasError(), qPrintable(xml.errorString()));
        QVERIFY(count > 250);
    }
};

QTEST_APPLESS_MAIN(TstTranslations)
#include "tst_Translations.moc"
