import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Lumina.Player

ApplicationWindow {
    id: root

    width: 1280
    height: 760
    minimumWidth: 760
    minimumHeight: 500
    visible: true
    color: Theme.background
    title: player.hasMedia && player.engine.mediaTitle !== ""
           ? player.engine.mediaTitle + " — Lumina" : "Lumina Player"

    // --- UI state ---------------------------------------------------------
    property bool chromeVisible: true
    property bool pointerInChrome: false
    property bool speedMenuOpen: false

    function showChrome() {
        chromeVisible = true
        hideTimer.restart()
    }

    function toggleFullscreen() {
        if (visibility === Window.FullScreen)
            showNormal()
        else
            showFullScreen()
    }

    // --- File opening -----------------------------------------------------
    FileDialog {
        id: fileDialog
        title: qsTr("Open Media")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("Media files (*.mkv *.mp4 *.mov *.avi *.wmv *.webm *.ts *.m2ts *.flv *.mp3 *.flac *.m4a *.wav *.ogg *.opus)"),
            qsTr("All files (*)")
        ]
        onAccepted: player.openFiles(selectedUrls)
    }

    // --- Video surface ----------------------------------------------------
    MpvVideoItem {
        id: videoItem
        anchors.fill: parent
        engine: player.engine
        visible: player.hasMedia
    }

    // --- Empty state ------------------------------------------------------
    Item {
        anchors.fill: parent
        visible: !player.hasMedia

        Column {
            anchors.centerIn: parent
            spacing: Theme.spacingLarge

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Lumina Player")
                color: Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: 28
                font.weight: Font.DemiBold
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Open a file to start watching")
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeLarge
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Open File")
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeLarge
                onClicked: fileDialog.open()

                background: Rectangle {
                    color: parent.pressed ? Theme.accentPressed
                                          : (parent.hovered ? Theme.accentHover : Theme.accent)
                    radius: Theme.radius
                }
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 20
                    rightPadding: 20
                    topPadding: 8
                    bottomPadding: 8
                }
            }
        }
    }

    // --- Media error banner ----------------------------------------------
    Rectangle {
        visible: player.engine.mediaError !== ""
        anchors.top: parent.top
        anchors.topMargin: 48
        anchors.horizontalCenter: parent.horizontalCenter
        width: errorText.implicitWidth + 32
        height: errorText.implicitHeight + 16
        color: Theme.elevated
        border.color: Theme.error
        radius: Theme.radius

        Text {
            id: errorText
            anchors.centerIn: parent
            text: player.engine.mediaError
            color: Theme.error
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
        }
    }

    // --- Playback controls overlay ----------------------------------------
    Rectangle {
        id: controlsBar
        visible: chromeVisible
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 72
        color: Theme.surface
        border.color: Theme.border

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onPositionChanged: root.showChrome()
            onEntered: root.pointerInChrome = true
            onExited: root.pointerInChrome = false
        }

        Column {
            anchors.fill: parent
            anchors.margins: Theme.spacingMedium

            TimeSlider {
                id: timeline
                width: parent.width
            }

            RowLayout {
                id: buttonRow
                width: parent.width
                height: parent.height - timeline.height - Theme.spacingSmall
                spacing: Theme.spacingSmall

                IconButton {
                    iconSource: player.engine.playing
                                ? "qrc:/qt/qml/Lumina/Player/icons/pause.svg"
                                : "qrc:/qt/qml/Lumina/Player/icons/play.svg"
                    visible: player.hasMedia
                    onClicked: player.engine.togglePlayPause()
                }

                IconButton {
                    iconSource: "qrc:/qt/qml/Lumina/Player/icons/stop.svg"
                    visible: player.hasMedia
                    onClicked: player.engine.stop()
                }

                Text {
                    visible: player.hasMedia
                    anchors.verticalCenter: parent.verticalCenter
                    text: timefmt.formatDuration(player.engine.position)
                          + " / " + timefmt.formatDuration(player.engine.duration)
                    color: Theme.secondaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                }

                Item { Layout.fillWidth: true; Layout.preferredHeight: 1 } // flexible space

                IconButton {
                    label: player.engine.speed.toFixed(2).replace(/0$/, "").replace(/\.$/, "") + "×"
                    visible: player.hasMedia
                    onClicked: speedMenu.popup(this, 0, -speedMenu.height - 8)
                }

                IconButton {
                    iconSource: player.engine.muted
                                ? "qrc:/qt/qml/Lumina/Player/icons/volume-mute.svg"
                                : "qrc:/qt/qml/Lumina/Player/icons/volume-high.svg"
                    visible: player.hasMedia
                    onClicked: player.engine.setMuted(!player.engine.muted)
                }

                Slider {
                    id: volumeSlider
                    visible: player.hasMedia
                    anchors.verticalCenter: parent.verticalCenter
                    from: 0
                    to: 150
                    value: player.engine.volume
                    onMoved: player.engine.setVolume(value)
                    implicitWidth: 90
                    implicitHeight: 20

                    background: Rectangle {
                        x: volumeSlider.leftPadding
                        y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                        width: volumeSlider.availableWidth
                        height: 4
                        radius: 2
                        color: Theme.elevated
                        Rectangle {
                            width: volumeSlider.visualPosition * parent.width
                            height: parent.height
                            radius: 2
                            color: Theme.accent
                        }
                    }
                    handle: Rectangle {
                        x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                        y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                        width: 10
                        height: 10
                        radius: 5
                        color: Theme.primaryText
                    }
                }

                IconButton {
                    iconSource: "qrc:/qt/qml/Lumina/Player/icons/folder-open.svg"
                    onClicked: fileDialog.open()
                }

                IconButton {
                    iconSource: root.visibility === Window.FullScreen
                                ? "qrc:/qt/qml/Lumina/Player/icons/fullscreen-exit.svg"
                                : "qrc:/qt/qml/Lumina/Player/icons/fullscreen.svg"
                    onClicked: root.toggleFullscreen()
                }
            }
        }
    }

    Menu {
        id: speedMenu
        onAboutToShow: root.speedMenuOpen = true
        onClosed: root.speedMenuOpen = false

        Repeater {
            model: [0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0]
            delegate: MenuItem {
                required property var modelData
                text: modelData.toFixed(2).replace(/0$/, "").replace(/\.$/, "") + "×"
                font.family: Theme.fontFamily
                checkable: true
                checked: Math.abs(player.engine.speed - modelData) < 0.001
                onTriggered: player.engine.setSpeed(modelData)
            }
        }
    }

    // --- Idle auto-hide ----------------------------------------------------
    Timer {
        id: hideTimer
        interval: 2600
        running: player.hasMedia && root.visibility === Window.FullScreen
        repeat: false
        onTriggered: {
            if (!root.pointerInChrome && !root.speedMenuOpen)
                root.chromeVisible = false
        }
    }

    // Track chrome hover for auto-hide.
    Connections {
        target: controlsBar
        function onVisibleChanged() {
            if (controlsBar.visible && root.visibility === Window.FullScreen)
                hideTimer.restart()
        }
    }

    // --- Mouse interaction --------------------------------------------------
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
        propagateComposedEvents: true
        onPositionChanged: root.showChrome()
    }

    // Double-click toggles fullscreen; single click reveals chrome.
    MouseArea {
        anchors.fill: parent
        z: -1
        onDoubleClicked: root.toggleFullscreen()
    }

    // --- Drag & drop ----------------------------------------------------------
    DropArea {
        anchors.fill: parent
        onEntered: (drag) => drag.accept(Qt.CopyAction)
        onDropped: (drop) => {
            if (drop.hasUrls) {
                player.openFiles(drop.urls)
                root.showChrome()
            }
        }
    }

    // --- Keyboard shortcuts (spec §10 defaults) --------------------------------
    Shortcut { sequence: "Space";        enabled: player.hasMedia; onActivated: player.engine.togglePlayPause() }
    Shortcut { sequence: "Left";         enabled: player.hasMedia; onActivated: player.engine.seekRelative(-5) }
    Shortcut { sequence: "Right";        enabled: player.hasMedia; onActivated: player.engine.seekRelative(5) }
    Shortcut { sequence: "Up";           enabled: player.hasMedia; onActivated: player.engine.setVolume(Math.min(150, player.engine.volume + 5)) }
    Shortcut { sequence: "Down";         enabled: player.hasMedia; onActivated: player.engine.setVolume(Math.max(0, player.engine.volume - 5)) }
    Shortcut { sequence: "M";            enabled: player.hasMedia; onActivated: player.engine.setMuted(!player.engine.muted) }
    Shortcut { sequence: "F";            onActivated: root.toggleFullscreen() }
    Shortcut { sequence: "T";            onActivated: root.flags = root.flags ^ Qt.WindowStaysOnTopHint }
    Shortcut { sequence: "A";            enabled: player.hasMedia; onActivated: player.engine.cycleAudioTrack() }
    Shortcut { sequence: "S";            enabled: player.hasMedia; onActivated: player.engine.cycleSubtitleTrack() }
    Shortcut { sequence: "W";            enabled: player.hasMedia; onActivated: player.engine.setSubtitlesVisible(true) }
    Shortcut { sequence: "Shift+W";      enabled: player.hasMedia; onActivated: player.engine.setSubtitlesVisible(false) }
    Shortcut { sequence: "X";            enabled: player.hasMedia; onActivated: player.engine.setSpeed(Math.max(0.25, player.engine.speed - 0.25)) }
    Shortcut { sequence: "C";            enabled: player.hasMedia; onActivated: player.engine.setSpeed(Math.min(2.0, player.engine.speed + 0.25)) }
    Shortcut { sequence: "Z";            enabled: player.hasMedia; onActivated: player.engine.setSpeed(1.0) }
    Shortcut { sequence: "Period";       enabled: player.hasMedia; onActivated: player.engine.stepFrame(1) }
    Shortcut { sequence: "Comma";        enabled: player.hasMedia; onActivated: player.engine.stepFrame(-1) }
    Shortcut { sequence: "Ctrl+O";       onActivated: fileDialog.open() }
    Shortcut { sequence: "Esc";          enabled: root.visibility === Window.FullScreen; onActivated: root.showNormal() }
}
