# Security policy

## Supported versions

Only the latest release receives security fixes. Lumina is in preview (0.3.x).

## Reporting a vulnerability

Please **do not** open a public issue for a security problem.

Use GitHub's private reporting: open the repository's **Security** tab and choose **Report a vulnerability**
(a private security advisory). If that option is not available, open a public issue that says only that you
have a security report and ask for a private contact, without any details.

Please include the version, your platform, and steps to reproduce. You will get a reply as soon as the
maintainer can; this is a volunteer project, so there is no guaranteed response time.

## What the project already does

See [docs/SECURITY_REVIEW.md](docs/SECURITY_REVIEW.md): a URL scheme allow-list for files and links that come
from outside the app, a deny-list for dangerous mpv options, a private data directory, no telemetry, and an
update check that is off by default and downloads nothing.
