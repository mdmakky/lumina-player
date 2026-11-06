import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Settings > Network: timeouts, user agent and yt-dlp quality for streams.
// Buffering (cache size / read-ahead) lives in Advanced; `dlg.gotoPage` links there.
SettingsPage {
    id: page
    property var dlg
    property int advancedPage: 8
    title: qsTr("Network")
    resettable: false

    property int timeout: 15
    property string userAgent: ""
    property string ytdlQuality: "1080"

    function ytdlFormat(q) {
        return q === "best" ? "bestvideo+bestaudio/best"
                            : "bestvideo[height<=?" + q + "]+bestaudio/best[height<=?" + q + "]/best"
    }
    function apply() {
        player.engine.setMpvOption("network-timeout", String(timeout))
        if (userAgent !== "") player.engine.setMpvOption("user-agent", userAgent)
        player.engine.setMpvOption("ytdl-format", ytdlFormat(ytdlQuality))
    }
    function setTimeout_(v) { timeout = v; player.setSetting("network/timeout", v); apply() }
    function setUserAgent(v) { userAgent = v.trim(); player.setSetting("network/userAgent", userAgent); apply() }
    function setQuality(v) { ytdlQuality = v; player.setSetting("network/ytdlQuality", v); apply() }

    Component.onCompleted: {
        timeout = Number(player.setting("network/timeout", 15))
        userAgent = player.setting("network/userAgent", "")
        ytdlQuality = player.setting("network/ytdlQuality", "1080")
        // Only override the engine defaults when the user changed something.
        if (player.setting("network/timeout", null) !== null || userAgent !== ""
                || player.setting("network/ytdlQuality", null) !== null)
            apply()
    }

    SettingSection {
        title: qsTr("Buffering")
        ButtonRow {
            name: "networkBufferingLink"
            title: qsTr("Cache size and read-ahead")
            description: qsTr("These live in Advanced > Buffering.")
            keywords: "cache buffer readahead memory demuxer"
            buttonText: qsTr("Open Advanced")
            onClicked: if (page.dlg) page.dlg.gotoPage(page.advancedPage)
        }
    }
    SettingSection {
        title: qsTr("Connection")
        StepperRow {
            name: "networkTimeout"
            title: qsTr("Network timeout")
            description: qsTr("Seconds to wait for a stream before giving up.")
            keywords: "connect timeout stream http"
            value: page.timeout; from: 5; to: 120; step: 5; suffix: " s"
            onPicked: (v) => page.setTimeout_(v)
        }
        SettingRow {
            stacked: true
            title: qsTr("User agent")
            description: qsTr("Sent to servers when streaming. Leave empty for the default.")
            keywords: "http header browser identify"
            TextField {
                objectName: "networkUserAgent"
                Layout.fillWidth: true
                text: page.userAgent
                placeholderText: qsTr("Default")
                placeholderTextColor: Theme.disabledText
                color: Theme.primaryText
                selectByMouse: true
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
                Accessible.name: qsTr("User agent")
                background: Rectangle {
                    implicitHeight: 32
                    radius: Theme.radius
                    color: Theme.elevated
                    border.width: parent.activeFocus ? 2 : 1
                    border.color: parent.activeFocus ? Theme.focusRing : Theme.border
                }
                onEditingFinished: page.setUserAgent(text)
            }
        }
    }
    SettingSection {
        title: qsTr("Web videos (yt-dlp)")
        SettingRow {
            title: qsTr("yt-dlp")
            description: player.engine.ytdlAvailable ? qsTr("Found. Page links such as YouTube can be played.")
                                                     : qsTr("Not found. Install yt-dlp to play page links.")
            keywords: "youtube youtube-dl extractor web"
            Text {
                objectName: "ytdlPath"
                Layout.maximumWidth: 360
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideMiddle
                text: player.engine.ytdlAvailable ? player.engine.ytdlPath : qsTr("Not installed")
                color: player.engine.ytdlAvailable ? Theme.success : Theme.secondaryText
                font.family: Theme.fontMono
                font.pixelSize: Theme.fontSizeSmall
            }
        }
        SettingRow {
            title: qsTr("Video quality")
            description: qsTr("Highest resolution requested from web videos.")
            keywords: "ytdl format resolution 720 1080 best"
            SegmentedControl {
                name: "ytdlQuality"
                accessibleName: qsTr("Video quality")
                model: [
                    { label: "720p", value: "720" },
                    { label: "1080p", value: "1080" },
                    { label: qsTr("Best"), value: "best" }
                ]
                value: page.ytdlQuality
                onPicked: (v) => page.setQuality(v)
            }
        }
    }
}
