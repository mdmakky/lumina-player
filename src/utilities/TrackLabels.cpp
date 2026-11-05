#include "TrackLabels.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>

namespace Lumina::TrackLabels {

static QString tr(const char *s) { return QCoreApplication::translate("TrackLabels", s); }

QString languageName(const QString &codeIn)
{
    // Each row: name, then every ISO 639-1/-2B/-2T code that maps to it.
    static const QHash<QString, QString> map = [] {
        struct Row { const char *name; const char *codes; };
        static const Row rows[] = {
            {"English", "en eng"}, {"Bangla", "bn ben"}, {"Hindi", "hi hin"}, {"Japanese", "ja jpn"},
            {"Spanish", "es spa"}, {"French", "fr fra fre"}, {"German", "de deu ger"}, {"Arabic", "ar ara"},
            {"Russian", "ru rus"}, {"Chinese", "zh zho chi cmn"}, {"Cantonese", "yue"}, {"Korean", "ko kor"},
            {"Portuguese", "pt por"}, {"Italian", "it ita"}, {"Turkish", "tr tur"}, {"Tamil", "ta tam"},
            {"Urdu", "ur urd"}, {"Telugu", "te tel"}, {"Marathi", "mr mar"}, {"Malayalam", "ml mal"},
            {"Kannada", "kn kan"}, {"Punjabi", "pa pan"}, {"Gujarati", "gu guj"}, {"Nepali", "ne nep"},
            {"Sinhala", "si sin"}, {"Dutch", "nl nld dut"}, {"Polish", "pl pol"}, {"Swedish", "sv swe"},
            {"Norwegian", "no nor nob nb"}, {"Danish", "da dan"}, {"Finnish", "fi fin"}, {"Greek", "el ell gre"},
            {"Czech", "cs ces cze"}, {"Hungarian", "hu hun"}, {"Romanian", "ro ron rum"}, {"Ukrainian", "uk ukr"},
            {"Hebrew", "he heb"}, {"Persian", "fa fas per"}, {"Thai", "th tha"}, {"Vietnamese", "vi vie"},
            {"Indonesian", "id ind"}, {"Malay", "ms msa may"},
        };
        QHash<QString, QString> m;
        for (const Row &r : rows)
            for (const QString &c : QString::fromLatin1(r.codes).split(u' '))
                m.insert(c, QString::fromLatin1(r.name));
        return m;
    }();
    // "en-US" / "pt_BR" -> base code
    const QString code = codeIn.trimmed().toLower().section(QRegularExpression(QStringLiteral("[-_]")), 0, 0);
    return map.value(code);
}

QString channelLayout(int n)
{
    switch (n) {
    case 0: return {};
    case 1: return tr("Mono");
    case 2: return tr("Stereo");
    case 6: return QStringLiteral("5.1");
    case 8: return QStringLiteral("7.1");
    default: return QStringLiteral("%1 ch").arg(n);
    }
}

QString codecName(const QString &c)
{
    static const QHash<QString, QString> map = {
        {"eac3", "E-AC3"}, {"truehd", "TrueHD"}, {"opus", "Opus"}, {"vorbis", "Vorbis"},
        {"subrip", "SRT"}, {"webvtt", "WebVTT"}, {"mov_text", "Text"}, {"hdmv_pgs_subtitle", "PGS"},
        {"dvd_subtitle", "VobSub"}, {"dvb_subtitle", "DVB"}, {"text", "Text"},
    };
    return map.value(c, c.toUpper());
}

QVariantList describeTracks(const QVariantList &tracks, bool audio)
{
    QVariantList out;
    QHash<QString, int> seen; // label+detail -> occurrences so far
    for (const QVariant &v : tracks) {
        QVariantMap t = v.toMap();
        const QString rawLang = t.value("lang").toString().trimmed().toLower();
        const QString lang = languageName(rawLang);
        const QString title = t.value("title").toString().trimmed();
        const QString low = title.toLower();
        const bool external = t.value("external").toBool();
        const bool commentary = low.contains(QLatin1String("commentary"));

        QString label = lang;
        if (label.isEmpty()) {
            if (external)
                label = tr("External");
            else if (!title.isEmpty() && !commentary)
                label = title;
            else if (rawLang.isEmpty() || rawLang == QLatin1String("und") || rawLang == QLatin1String("unknown"))
                label = tr("Unknown");
            else
                label = rawLang.toUpper(); // unmapped code: better than nothing
        }

        QStringList parts;
        if (!t.value("codec").toString().isEmpty())
            parts << codecName(t.value("codec").toString());
        if (audio) {
            const QString lay = channelLayout(t.value("demux-channel-count").toInt());
            if (!lay.isEmpty())
                parts << lay;
        }
        if (commentary)
            parts << tr("Commentary");
        else if (!title.isEmpty() && title != label && low != QLatin1String("default") && low != QLatin1String("forced"))
            parts << title;
        else if (external && lang.isEmpty() && !t.value("external-filename").toString().isEmpty())
            parts << QFileInfo(t.value("external-filename").toString()).fileName();
        if (external && !lang.isEmpty())
            parts << tr("External");
        if (t.value("forced").toBool() || low == QLatin1String("forced"))
            parts << tr("Forced");
        if (t.value("default").toBool())
            parts << tr("Default");
        parts.removeDuplicates();

        const QString detail = parts.join(QStringLiteral(" · "));
        const int n = ++seen[label + QLatin1Char('\n') + detail];
        if (n > 1)
            label += QStringLiteral(" #%1").arg(n);
        t.insert("label", label);
        t.insert("detail", detail);
        out.append(t);
    }
    return out;
}

} // namespace Lumina::TrackLabels
