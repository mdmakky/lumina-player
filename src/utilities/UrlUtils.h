#pragma once

#include <QString>
#include <QUrl>

namespace Lumina::UrlUtils {

// Trims and validates user input. Accepts http(s), rtmp(s), rtsp, ftp and file URLs
// (m3u8 playlists are plain http(s) URLs). Returns an invalid QUrl otherwise.
QUrl parseMediaUrl(const QString &input);

// True for anything that is not a local file (http, rtmp, ...).
bool isRemote(const QUrl &url);

// Display title for a URL: last path segment (percent-decoded), else the host.
QString titleFor(const QUrl &url);

// True when the URL points straight at a media file or stream (known extension, or a
// non-HTTP protocol); false for what is probably a web page that needs yt-dlp.
bool looksLikeDirectMedia(const QUrl &url);

} // namespace Lumina::UrlUtils
