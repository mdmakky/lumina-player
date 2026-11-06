#pragma once

#include <QString>
#include <QStringList>

// Pure helpers that build mpv/lavfi audio filter strings (unit-tested, no mpv dependency).
namespace Lumina::AudioFilters {

// Volume normalization presets: "off" | "night" (strong compression) | "movie" (EBU R128 loudness).
inline QString normalizationFilter(const QString &mode)
{
    if (mode == QLatin1String("night"))
        return QStringLiteral("dynaudnorm=f=150:g=15:p=0.5:m=30:s=10");
    if (mode == QLatin1String("movie"))
        return QStringLiteral("loudnorm=I=-16:TP=-1.5:LRA=11");
    return {};
}

// Visualization source filter for audio-only files; empty for "off"/unknown.
inline QString visualizationFilter(const QString &mode)
{
    if (mode == QLatin1String("spectrum"))
        return QStringLiteral("showcqt=s=1280x720:fps=30,format=yuv420p");
    if (mode == QLatin1String("waveform"))
        return QStringLiteral("showwaves=s=1280x720:mode=cline:rate=30:colors=0x8ab4ff,format=yuv420p");
    if (mode == QLatin1String("scope"))
        return QStringLiteral("avectorscope=s=720x720:rate=30:zoom=1.5,format=yuv420p");
    if (mode == QLatin1String("freqs"))
        return QStringLiteral("showfreqs=s=1280x720:mode=bar:fscale=log:rate=30,format=yuv420p");
    return {};
}

// Comma-joined lavfi chain (equalizer first, then normalization); empty when nothing is active.
inline QString chain(const QStringList &eqBands, const QString &normMode)
{
    QStringList f = eqBands;
    const QString n = normalizationFilter(normMode);
    if (!n.isEmpty())
        f << n;
    return f.join(QLatin1Char(','));
}

// Value for mpv's `af` property.
inline QString afValue(const QString &chain)
{
    return chain.isEmpty() ? QString() : QStringLiteral("lavfi=[%1]").arg(chain);
}

// Value for mpv's `lavfi-complex`: plays the first audio track through `chain` and draws it as video.
inline QString visualizationGraph(const QString &mode, const QString &chain)
{
    const QString v = visualizationFilter(mode);
    if (v.isEmpty())
        return {};
    return QStringLiteral("[aid1]%1asplit[ao][t];[t]%2[vo]")
        .arg(chain.isEmpty() ? QString() : chain + QLatin1Char(','), v);
}

} // namespace Lumina::AudioFilters
