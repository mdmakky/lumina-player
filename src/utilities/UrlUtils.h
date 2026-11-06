#pragma once

#include <QString>
#include <QUrl>

namespace Lumina::UrlUtils {

// Trims and validates user input. Accepts http(s), rtmp(s), rtsp, ftp, sftp, smb, nfs and file URLs
// (m3u8 playlists are plain http(s) URLs). Returns an invalid QUrl otherwise.
QUrl parseMediaUrl(const QString &input);

// True for anything that is not a local file (http, rtmp, ...).
// Protocols that only exist when mpv's FFmpeg was built with the matching library.
bool needsOptionalProtocol(const QUrl &url);

bool isRemote(const QUrl &url);
// Scheme allow-list for URLs that come from outside (CLI, single-instance, MPRIS OpenUri, drops):
// local files and plain network media only; mpv-internal protocols (edl, av, lavf, ytdl, fd, memory ...) are rejected.
bool isAllowedMediaUrl(const QUrl &url);

// Display title for a URL: last path segment (percent-decoded), else the host.
QString titleFor(const QUrl &url);

// True when the URL points straight at a media file or stream (known extension, or a
// non-HTTP protocol); false for what is probably a web page that needs yt-dlp.
bool looksLikeDirectMedia(const QUrl &url);

} // namespace Lumina::UrlUtils
