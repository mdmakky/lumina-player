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
    color: "black"
    title: player.hasMedia && player.engine.mediaTitle !== ""
           ? player.engine.mediaTitle + " — Lumina" : "Lumina Player"

    // --- Window state persistence -------------------------------------------
    Component.onCompleted: {
        const g = player.setting("window/geometry", null)
        if (g && g.width >= minimumWidth && g.height >= minimumHeight) {
            x = g.x; y = g.y; width = g.width; height = g.height
        }
        const max = player.setting("window/maximized", false)
        if (max === true || max === "true")
            showMaximized()
    }
    onClosing: {
        if (visibility === Window.FullScreen)
            return
        player.setSetting("window/maximized", visibility === Window.Maximized)
        if (visibility === Window.Windowed)
            player.setSetting("window/geometry", { x: x, y: y, width: width, height: height })
    }

    // --- UI state ---------------------------------------------------------
    property bool chromeVisible: true
    property bool menuOpen: false
    property bool playlistOpen: false
    readonly property bool chromeHovered: topHover.hovered || bottomHover.hovered
    readonly property string iconBase: "qrc:/qt/qml/Lumina/Player/icons/"

    function showChrome() {
        chromeVisible = true
        hideTimer.restart()
    }

    // Jump to the next/previous chapter relative to the playhead.
    function stepChapter(dir) {
        const ch = player.engine.chapters
        const pos = player.engine.position
        let cur = -1
        for (let i = 0; i < ch.length; ++i)
            if (ch[i].time <= pos + 0.5) cur = i
        const target = dir > 0 ? cur + 1 : (pos - ch[Math.max(cur, 0)].time > 3 ? cur : cur - 1)
        if (ch.length > 0 && target >= 0 && target < ch.length)
            player.engine.seekChapter(target)
    }

    function toggleFullscreen() {
        if (visibility === Window.FullScreen)
            showNormal()
        else
            showFullScreen()
    }

    function baseName(path) {
        const parts = path.split("/")
        return parts[parts.length - 1]
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

    // Click toggles play/pause, double-click toggles fullscreen.
    MouseArea {
        anchors.fill: parent
        visible: player.hasMedia
        onClicked: {
            if (root.playlistOpen) root.playlistOpen = false
            else clickTimer.restart()
        }
        onDoubleClicked: {
            clickTimer.stop()
            root.toggleFullscreen()
        }
    }
    Timer {
        id: clickTimer
        interval: 220
        onTriggered: player.engine.togglePlayPause()
    }

    // --- Home / empty state -----------------------------------------------
    Rectangle {
        anchors.fill: parent
        visible: !player.hasMedia
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.background }
            GradientStop { position: 1.0; color: Theme.surface }
        }

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(460, parent.width - 64)
            spacing: Theme.spacingLarge

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Lumina")
                color: Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: 44
                font.weight: Font.Light
                font.letterSpacing: 6
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: Theme.spacingMedium
                text: qsTr("Drop a video here, or open a file to begin")
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeLarge
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Open File")
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeLarge
                onClicked: fileDialog.open()
                background: Rectangle {
                    implicitWidth: 160
                    implicitHeight: 42
                    radius: 21
                    color: parent.pressed ? Theme.accentPressed
                                          : (parent.hovered ? Theme.accentHover : Theme.accent)
                    Behavior on color { ColorAnimation { duration: Theme.animationFast } }
                }
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: Theme.primaryText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Text {
                visible: player.recentFiles.length > 0
                Layout.topMargin: Theme.spacingLarge
                text: qsTr("RECENT")
                color: Theme.disabledText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
                font.letterSpacing: 2
            }

            Repeater {
                model: player.recentFiles.slice(0, 5)
                delegate: Rectangle {
                    id: recentRow
                    required property string modelData
                    Layout.fillWidth: true
                    implicitHeight: 40
                    radius: Theme.radius
                    color: rowMouse.containsMouse ? Theme.surfaceHover : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.animationFast } }

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingMedium
                        verticalAlignment: Text.AlignVCenter
                        text: root.baseName(recentRow.modelData)
                        color: Theme.primaryText
                        elide: Text.ElideMiddle
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeMedium
                    }
                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: player.openFiles([Qt.url("file://" + recentRow.modelData)])
                    }
                }
            }
        }
    }

    // --- Media error banner ----------------------------------------------
    Rectangle {
        visible: player.engine.mediaError !== ""
        anchors.top: parent.top
        anchors.topMargin: 72
        anchors.horizontalCenter: parent.horizontalCenter
        width: errorText.implicitWidth + 32
        height: errorText.implicitHeight + 16
        color: Theme.panelOverlay
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

    // --- Top bar (title + quick actions) ----------------------------------
    Rectangle {
        id: topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 80
        opacity: root.chromeVisible && player.hasMedia ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.scrimStrong }
            GradientStop { position: 1.0; color: Theme.scrimClear }
        }

        HoverHandler { id: topHover }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.spacingLarge
            anchors.topMargin: Theme.spacingMedium
            spacing: Theme.spacingSmall

            Text {
                Layout.fillWidth: true
                text: player.engine.mediaTitle
                color: Theme.primaryText
                elide: Text.ElideRight
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeHeading
                font.weight: Font.DemiBold
            }
            IconButton {
                iconSource: root.iconBase + "folder-open.svg"
                tip: qsTr("Open file (Ctrl+O)")
                onClicked: fileDialog.open()
            }
            IconButton {
                iconSource: root.iconBase + "playlist.svg"
                tip: qsTr("Playlist (P)")
                onClicked: root.playlistOpen = !root.playlistOpen
            }
        }
    }

    // --- Playlist panel ---------------------------------------------------
    PlaylistPanel {
        id: playlistPanel
        visible: root.playlistOpen
        anchors.top: parent.top
        anchors.topMargin: 56
        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingMedium
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 120
        radius: Theme.radiusLarge
        color: Theme.panelOverlay
        clip: true
    }

    // --- Bottom controls (floating over the video) -------------------------
    Rectangle {
        id: controlsBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 112
        opacity: root.chromeVisible && player.hasMedia ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.scrimClear }
            GradientStop { position: 1.0; color: Theme.scrimStrong }
        }

        HoverHandler { id: bottomHover }

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: Theme.spacingLarge
            anchors.rightMargin: Theme.spacingLarge
            anchors.bottomMargin: Theme.spacingMedium
            spacing: 2

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMedium

                Text {
                    text: timefmt.formatDuration(player.engine.position)
                    color: Theme.primaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    font.features: { "tnum": 1 }
                }
                TimeSlider {
                    id: timeline
                    Layout.fillWidth: true
                }
                Text {
                    text: timefmt.formatDuration(player.engine.duration)
                    color: Theme.secondaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    font.features: { "tnum": 1 }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                // Left: volume
                RowLayout {
                    Layout.preferredWidth: 220
                    spacing: Theme.spacingSmall
                    IconButton {
                        iconSource: root.iconBase + (player.engine.muted ? "volume-mute.svg" : "volume-high.svg")
                        tip: qsTr("Mute (M)")
                        onClicked: player.engine.setMuted(!player.engine.muted)
                    }
                    VolumeSlider {
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }

                // Center: transport
                IconButton {
                    iconSource: root.iconBase + "skip-previous.svg"
                    tip: qsTr("Previous (Ctrl+Left)")
                    enabled: player.playlist.count > 1
                    onClicked: player.previous()
                }
                Button {
                    id: playButton
                    implicitWidth: 48
                    implicitHeight: 48
                    onClicked: player.engine.togglePlayPause()
                    background: Rectangle {
                        radius: width / 2
                        color: playButton.pressed ? Theme.accentPressed
                                                  : (playButton.hovered ? Theme.accentHover : Theme.accent)
                        scale: playButton.hovered ? 1.06 : 1.0
                        Behavior on scale { NumberAnimation { duration: Theme.animationFast } }
                        Behavior on color { ColorAnimation { duration: Theme.animationFast } }
                    }
                    contentItem: Image {
                        source: root.iconBase + (player.engine.playing ? "pause.svg" : "play.svg")
                        sourceSize: Qt.size(22, 22)
                        fillMode: Image.Pad
                        horizontalAlignment: Image.AlignHCenter
                        verticalAlignment: Image.AlignVCenter
                    }
                }
                IconButton {
                    iconSource: root.iconBase + "skip-next.svg"
                    tip: qsTr("Next (Ctrl+Right)")
                    enabled: player.playlist.count > 1
                    onClicked: player.next()
                }

                Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }

                // Right: speed, chapters, fullscreen
                RowLayout {
                    Layout.preferredWidth: 220
                    layoutDirection: Qt.RightToLeft
                    spacing: Theme.spacingSmall

                    IconButton {
                        iconSource: root.visibility === Window.FullScreen
                                    ? root.iconBase + "fullscreen-exit.svg"
                                    : root.iconBase + "fullscreen.svg"
                        tip: qsTr("Fullscreen (F)")
                        onClicked: root.toggleFullscreen()
                    }
                    IconButton {
                        iconSource: root.iconBase + "chapters.svg"
                        tip: qsTr("Chapters (PgUp/PgDn)")
                        visible: player.engine.chapters.length > 0
                        onClicked: chapterMenu.popup(this, 0, -chapterMenu.height - 8)
                    }
                    IconButton {
                        label: player.engine.speed.toFixed(2).replace(/0$/, "").replace(/\.$/, "") + "×"
                        tip: qsTr("Speed (X / C / Z)")
                        onClicked: speedMenu.popup(this, 0, -speedMenu.height - 8)
                    }
                }
            }
        }
    }

    // --- Menus ---------------------------------------------------------------
    component ThemedMenu: Menu {
        padding: 6
        onAboutToShow: root.menuOpen = true
        onClosed: root.menuOpen = false
        background: Rectangle {
            implicitWidth: 180
            color: Theme.panelOverlay
            border.color: Theme.border
            radius: Theme.radius
        }
    }

    ThemedMenu {
        id: speedMenu
        Repeater {
            model: [0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0]
            delegate: ThemedMenuItem {
                required property var modelData
                text: modelData.toFixed(2).replace(/0$/, "").replace(/\.$/, "") + "×"
                checkable: true
                checked: Math.abs(player.engine.speed - modelData) < 0.001
                onTriggered: player.engine.setSpeed(modelData)
            }
        }
    }

    ThemedMenu {
        id: chapterMenu
        Repeater {
            model: player.engine.chapters
            delegate: ThemedMenuItem {
                required property var modelData
                required property int index
                text: (modelData.title || qsTr("Chapter %1").arg(index + 1))
                      + "   " + timefmt.formatDuration(modelData.time)
                onTriggered: player.engine.seekChapter(index)
            }
        }
    }

    // --- Idle auto-hide + cursor -----------------------------------------------
    Timer {
        id: hideTimer
        interval: 2600
        repeat: false
        onTriggered: {
            if (player.engine.playing && !root.chromeHovered && !root.menuOpen && !root.playlistOpen)
                root.chromeVisible = false
        }
    }

    Connections {
        target: player.engine
        function onPausedChanged() { root.showChrome() }
    }

    HoverHandler {
        cursorShape: root.chromeVisible || !player.hasMedia ? Qt.ArrowCursor : Qt.BlankCursor
        onPointChanged: root.showChrome()
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
    Shortcut { sequence: "PgDown";       enabled: player.hasMedia; onActivated: root.stepChapter(1) }
    Shortcut { sequence: "PgUp";         enabled: player.hasMedia; onActivated: root.stepChapter(-1) }
    Shortcut { sequence: "P";            onActivated: root.playlistOpen = !root.playlistOpen }
    Shortcut { sequence: "Ctrl+Right";   enabled: player.hasMedia; onActivated: player.next() }
    Shortcut { sequence: "Ctrl+Left";    enabled: player.hasMedia; onActivated: player.previous() }
    Shortcut { sequence: "Ctrl+O";       onActivated: fileDialog.open() }
    Shortcut { sequence: "Esc";          enabled: root.visibility === Window.FullScreen; onActivated: root.showNormal() }
}
