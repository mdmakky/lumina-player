#include "UrlUtils.h"

#include <QStringList>

namespace Lumina::UrlUtils {

QUrl parseMediaUrl(const QString &input)
{
    const QString s = input.trimmed();
    if (s.isEmpty() || s.contains(QLatin1Char(' ')))
        return {};
    const QUrl url(s, QUrl::StrictMode);
    static const QStringList schemes{"http", "https", "rtmp", "rtmps", "rtmpe", "rtsp", "rtsps", "ftp", "sftp", "smb", "nfs", "file"};
    const QString scheme = url.scheme().toLower();
    if (!url.isValid() || !schemes.contains(scheme))
        return {};
    if (scheme == QLatin1String("file") ? url.path().isEmpty() : url.host().isEmpty())
        return {};
    return url;
}

bool needsOptionalProtocol(const QUrl &url)
{
    static const QStringList p{"smb", "nfs", "sftp", "ftp"};
    return p.contains(url.scheme().toLower());
}

bool isRemote(const QUrl &url)
{
    return !url.isEmpty() && !url.isLocalFile() && !url.scheme().isEmpty();
}

QString titleFor(const QUrl &url)
{
    const QStringList parts = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    return parts.isEmpty() ? url.host() : QUrl::fromPercentEncoding(parts.last().toUtf8());
}

bool looksLikeDirectMedia(const QUrl &url)
{
    const QString scheme = url.scheme().toLower();
    if (scheme != QLatin1String("http") && scheme != QLatin1String("https"))
        return true;
    static const QStringList exts{"mp4", "m4v", "mkv", "webm", "mov", "avi", "flv", "wmv", "ts", "m2ts",
                                  "m3u8", "m3u", "mpd", "mp3", "flac", "m4a", "ogg", "opus", "wav", "aac"};
    return exts.contains(url.path().section(QLatin1Char('.'), -1).toLower());
}


bool isAllowedMediaUrl(const QUrl &url)
{
    if (url.isEmpty() || url.isLocalFile())
        return true;
    const QString scheme = url.scheme().toLower();
    if (scheme.isEmpty() || scheme.size() == 1) // relative path or a Windows drive letter
        return !url.path().isEmpty() || !url.toString().isEmpty();
    static const QStringList allowed{"http", "https", "rtmp", "rtmps", "rtmpe", "rtsp", "rtsps", "ftp", "sftp", "smb", "nfs"};
    return allowed.contains(scheme);
}

}
