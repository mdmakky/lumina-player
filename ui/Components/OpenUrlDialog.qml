import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// "Open URL…": paste a network address (http/https/rtmp/rtsp/ftp/m3u8) and play it.
ModalCard {
    id: dlg

    title: qsTr("Open URL")
    subtitle: player.engine.ytdlAvailable
              ? qsTr("Paste a link to a video, a stream (HLS .m3u8, DASH .mpd, RTMP, RTSP, SMB, NFS, SFTP) or a web page supported by yt-dlp.")
              : qsTr("Paste a link to a video or a stream (HLS .m3u8, DASH .mpd, RTMP, RTSP, SMB, NFS, SFTP). Web pages such as YouTube need yt-dlp installed.")
    cardWidth: 560

    readonly property bool valid: player.isValidUrl(field.text)
    property bool attempted: false

    function submit() {
        if (player.openUrl(field.text))
            close()
        else
            attempted = true
    }

    onAboutToShow: { field.text = ""; attempted = false }
    onOpened: field.forceActiveFocus()

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
        leftPadding: 44
        rightPadding: 16
        implicitHeight: 46
        Accessible.name: qsTr("Address")
        background: Rectangle {
            radius: height / 2
            color: Theme.background
            border.width: field.activeFocus ? 2 : 1
            border.color: dlg.attempted && !dlg.valid ? Theme.error
                          : field.activeFocus ? Theme.focusRing : Theme.border
            Glyph { x: 16; anchors.verticalCenter: parent.verticalCenter; name: "link"; size: 16; tint: Theme.secondaryText }
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
        Layout.topMargin: Theme.spacingSmall
        Text {
            Layout.fillWidth: true
            text: qsTr("Recent")
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            font.weight: Font.DemiBold
        }
        SettingsButton {
            objectName: "clearUrlsButton"
            text: qsTr("Clear")
            implicitHeight: 28
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
            Layout.topMargin: -6
            implicitHeight: 36
            radius: Theme.radius
            color: rowMouse.containsMouse ? Theme.surfaceHover : "transparent"
            Behavior on color { ColorAnimation { duration: Theme.animationFast } }
            Glyph { x: 10; anchors.verticalCenter: parent.verticalCenter; name: "link"; size: 14; tint: Theme.disabledText }
            Text {
                anchors { fill: parent; leftMargin: 34; rightMargin: 12 }
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

    ModalFooter {
        SettingsButton { text: qsTr("Cancel"); onClicked: dlg.close() }
        SettingsButton { objectName: "urlOpenButton"; text: qsTr("Open"); primary: true; onClicked: dlg.submit() }
    }
}
