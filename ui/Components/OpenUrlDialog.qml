import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// "Open URL…": paste a network address (http/https/rtmp/rtsp/ftp/m3u8) and play it.
Popup {
    id: dlg

    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(560, (parent ? parent.width : 560) - 48)
    padding: Theme.spacingLarge
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    readonly property bool valid: player.isValidUrl(field.text)

    function submit() {
        if (player.openUrl(field.text))
            close()
        else
            attempted = true
    }
    property bool attempted: false

    onAboutToShow: { field.text = ""; attempted = false }
    onOpened: field.forceActiveFocus()

    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.5) }
    background: Rectangle {
        color: Theme.surface
        border.color: Theme.border
        radius: Theme.radiusLarge
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingMedium

        Text {
            text: qsTr("Open URL")
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeHeading
            font.weight: Font.DemiBold
        }
        Text {
            Layout.fillWidth: true
            text: player.engine.ytdlAvailable
                  ? qsTr("Paste a link to a video, a stream (HLS .m3u8, DASH .mpd, RTMP, RTSP, SMB, NFS, SFTP) or a web page supported by yt-dlp.")
                  : qsTr("Paste a link to a video or a stream (HLS .m3u8, DASH .mpd, RTMP, RTSP, SMB, NFS, SFTP). Web pages such as YouTube need yt-dlp installed.")
            wrapMode: Text.Wrap
            color: Theme.secondaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
        }

        TextField {
            id: field
            objectName: "urlField"
            Layout.fillWidth: true
            placeholderText: qsTr("https://example.com/video.mp4")
            selectByMouse: true
            color: Theme.primaryText
            placeholderTextColor: Theme.disabledText
            selectionColor: Theme.accent
            selectedTextColor: Theme.accentText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeLarge
            leftPadding: Theme.spacingMedium
            rightPadding: Theme.spacingMedium
            implicitHeight: 40
            background: Rectangle {
                radius: Theme.radius
                color: Theme.elevated
                border.width: field.activeFocus ? 2 : 1
                border.color: dlg.attempted && !dlg.valid ? Theme.error
                              : field.activeFocus ? Theme.focusRing : Theme.border
            }
            onAccepted: dlg.submit()
            onTextChanged: dlg.attempted = false
        }
        Text {
            objectName: "urlError"
            visible: dlg.attempted && !dlg.valid
            text: qsTr("Enter a full address starting with http://, https://, rtmp://, rtsp://, ftp://, sftp://, smb:// or nfs://")
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            color: Theme.error
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }

        RowLayout {
            visible: player.recentUrls.length > 0
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: qsTr("RECENT")
                color: Theme.disabledText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
                font.letterSpacing: 2
            }
            SettingsButton {
                objectName: "clearUrlsButton"
                text: qsTr("Clear")
                onClicked: player.clearRecentUrls()
            }
        }
        Repeater {
            model: player.recentUrls.slice(0, 5)
            delegate: Rectangle {
                id: row
                required property int index
                required property string modelData
                objectName: "recentUrl_" + index
                Layout.fillWidth: true
                implicitHeight: 32
                radius: Theme.radiusSmall
                color: rowMouse.containsMouse ? Theme.surfaceHover : "transparent"
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingMedium
                    anchors.rightMargin: Theme.spacingMedium
                    verticalAlignment: Text.AlignVCenter
                    text: row.modelData
                    elide: Text.ElideMiddle
                    color: Theme.primaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeMedium
                }
                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { field.text = row.modelData; dlg.submit() }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingSmall
            Item { Layout.fillWidth: true }
            SettingsButton {
                text: qsTr("Cancel")
                onClicked: dlg.close()
            }
            SettingsButton {
                objectName: "urlOpenButton"
                text: qsTr("Open")
                primary: true
                onClicked: dlg.submit()
            }
        }
    }
}
