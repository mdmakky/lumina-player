import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQml.Models
import Lumina.Player

ApplicationWindow {
    id: root

    width: 1280
    height: 760
    minimumWidth: miniMode ? 240 : 760
    minimumHeight: miniMode ? 135 : 500
    visible: true
    color: Theme.videoBackdrop
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
        const mini = player.setting("window/mini", false)
        if (mini === true || mini === "true")
            Qt.callLater(toggleMini)
    }
    onClosing: {
        if (visibility === Window.FullScreen)
            return
        player.setSetting("window/maximized", visibility === Window.Maximized)
        if (miniMode && normalGeom)
            player.setSetting("window/geometry", normalGeom)
        else if (visibility === Window.Windowed)
            player.setSetting("window/geometry", { x: x, y: y, width: width, height: height })
    }

    // --- UI state ---------------------------------------------------------
    property bool chromeVisible: true
    property bool menuOpen: false
    property bool playlistOpen: false
    property bool miniMode: false
    property var normalGeom: null
    property bool miniPrevOnTop: false
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

    // Mini-player: ~480 px wide at the video's aspect ratio, always on top, minimal chrome.
    function toggleMini() {
        if (!miniMode) {
            if (visibility !== Window.Windowed) return
            normalGeom = { x: x, y: y, width: width, height: height }
            miniPrevOnTop = (flags & Qt.WindowStaysOnTopHint) !== 0
            const vt = player.engine.videoTracks
            const ar = vt.length > 0 && vt[0]["demux-w"] > 0 && vt[0]["demux-h"] > 0
                     ? vt[0]["demux-w"] / vt[0]["demux-h"] : 16 / 9
            miniMode = true
            playlistOpen = false
            width = 480
            height = Math.round(480 / ar)
            flags = flags | Qt.WindowStaysOnTopHint
        } else {
            miniMode = false
            if (!miniPrevOnTop) flags = flags & ~Qt.WindowStaysOnTopHint
            if (normalGeom) {
                x = normalGeom.x; y = normalGeom.y
                width = normalGeom.width; height = normalGeom.height
            }
        }
        player.setSetting("window/mini", miniMode)
    }

    function toggleFullscreen() {
        if (miniMode) toggleMini()
        if (visibility === Window.FullScreen)
            showNormal()
        else
            showFullScreen()
    }

    function relTimeOf(sec) {
        const d = Math.max(0, Date.now() / 1000 - sec)
        if (d < 90) return qsTr("just now")
        if (d < 3600) return qsTr("%1 min ago").arg(Math.round(d / 60))
        if (d < 86400) return qsTr("%1 h ago").arg(Math.round(d / 3600))
        if (d < 86400 * 30) return qsTr("%1 d ago").arg(Math.round(d / 86400))
        return qsTr("long ago")
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

    OpenUrlDialog {
        id: openUrlDialog
        objectName: "openUrlDialog"
        onAboutToShow: root.menuOpen = true
        onClosed: root.menuOpen = false
    }

    // --- Video surface ----------------------------------------------------
    MpvVideoItem {
        id: videoItem
        anchors.fill: parent
        engine: player.engine
        visible: player.hasMedia
    }
    AudioBackdrop { anchors.fill: parent; engine: player.engine; active: player.hasMedia }

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
        // Accumulate so smooth touchpad scrolling gives fine 1% steps (a mouse notch = 120 units = 5%).
        // `inverted` = natural scrolling: flip so fingers/wheel moving "up" always means louder.
        property real wheelAcc: 0
        onWheel: (w) => {
            wheelAcc += w.angleDelta.y * (w.inverted ? -1 : 1)
            const steps = Math.trunc(wheelAcc / 24)
            if (steps === 0) return
            wheelAcc -= steps * 24
            const sys = player.systemVolume >= 0
            const cur = sys ? player.systemVolume : player.engine.volume
            player.setVolumePercent(Math.max(0, Math.min(sys ? 100 : 150, cur + steps)))
        }
    }
    Timer {
        id: clickTimer
        interval: 220
        onTriggered: player.engine.togglePlayPause()
    }

    StatsOverlay {
        objectName: "statsOverlay"
        engine: player.engine
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Theme.spacingLarge
        anchors.topMargin: 72
        visible: player.hasMedia && player.engine.statsEnabled && !root.miniMode
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
            width: Math.min(720, parent.width - 64)
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

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Theme.spacingMedium
            Button {
                objectName: "homeOpenButton"
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
                    color: Theme.accentText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
                Button {
                    objectName: "homeUrlButton"
                    text: qsTr("Open URL…")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeLarge
                    onClicked: openUrlDialog.open()
                    background: Rectangle {
                        implicitWidth: 140
                        implicitHeight: 42
                        radius: 21
                        color: parent.pressed ? Theme.surfacePressed : (parent.hovered ? Theme.surfaceHover : "transparent")
                        border.color: Theme.border
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: Theme.primaryText
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Button {
                objectName: "homeLibraryButton"
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Library")
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
                onClicked: libraryView.open()
                background: Rectangle {
                    implicitWidth: 160
                    implicitHeight: 34
                    radius: 17
                    color: parent.pressed ? Theme.surfacePressed : (parent.hovered ? Theme.surfaceHover : "transparent")
                    border.color: Theme.border
                }
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: Theme.secondaryText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            // ---- Library sections (SQLite); falls back to the plain recent list if DB is unavailable ----
            Component {
                id: historyCard
                Rectangle {
                    id: card
                    required property int index
                    required property string path
                    required property string title
                    required property real progress
                    required property real lastPlayed
                    required property bool favorite
                    objectName: "continueCard_" + index
                    width: 210
                    height: 84
                    radius: Theme.radius
                    color: cardMouse.containsMouse ? Theme.elevated : Theme.surface
                    border.color: Theme.border
                    Behavior on color { ColorAnimation { duration: Theme.animationFast } }

                    MouseArea {
                        id: cardMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        onClicked: (m) => m.button === Qt.RightButton ? player.toggleFavorite(card.path)
                                                                      : player.openHistory(card.path)
                    }
                    Text {
                        anchors { left: parent.left; right: star.left; top: parent.top; margins: Theme.spacingMedium }
                        text: card.title
                        color: Theme.primaryText
                        elide: Text.ElideRight
                        wrapMode: Text.Wrap
                        maximumLineCount: 2
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeMedium
                    }
                    Text {
                        anchors { left: parent.left; bottom: bar.top; margins: Theme.spacingMedium; bottomMargin: Theme.spacingSmall }
                        text: root.relTimeOf(card.lastPlayed)
                        color: Theme.secondaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    Text {
                        id: star
                        objectName: "cardStar_" + card.index
                        anchors { right: parent.right; top: parent.top; margins: Theme.spacingSmall }
                        text: card.favorite ? "★" : "☆"
                        color: card.favorite ? Theme.accentHover : Theme.disabledText
                        font.pixelSize: Theme.fontSizeLarge
                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -4
                            cursorShape: Qt.PointingHandCursor
                            onClicked: player.toggleFavorite(card.path)
                        }
                    }
                    Rectangle { // thin progress bar
                        id: bar
                        visible: card.progress > 0
                        anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                                  leftMargin: Theme.spacingMedium; rightMargin: Theme.spacingMedium
                                  bottomMargin: Theme.spacingMedium }
                        height: 3
                        radius: 2
                        color: Theme.trackIdle
                        Rectangle {
                            width: parent.width * card.progress
                            height: parent.height
                            radius: 2
                            color: Theme.accent
                        }
                    }
                }
            }

            Text {
                visible: player.continueWatching.count > 0
                Layout.topMargin: Theme.spacingLarge
                text: qsTr("CONTINUE WATCHING")
                color: Theme.disabledText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
                font.letterSpacing: 2
            }
            ListView {
                objectName: "continueList"
                visible: player.continueWatching.count > 0
                Layout.fillWidth: true
                Layout.preferredHeight: 84
                orientation: ListView.Horizontal
                spacing: Theme.spacingMedium
                clip: true
                model: player.continueWatching
                delegate: historyCard
            }

            Text {
                visible: player.favorites.count > 0
                Layout.topMargin: Theme.spacingMedium
                text: qsTr("FAVORITES")
                color: Theme.disabledText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
                font.letterSpacing: 2
            }
            ListView {
                objectName: "favoritesList"
                visible: player.favorites.count > 0
                Layout.fillWidth: true
                Layout.preferredHeight: 84
                orientation: ListView.Horizontal
                spacing: Theme.spacingMedium
                clip: true
                model: player.favorites
                delegate: historyCard
            }

            Text {
                visible: player.historyAvailable ? player.history.count > 0 : player.recentFiles.length > 0
                Layout.topMargin: Theme.spacingMedium
                text: qsTr("RECENT")
                color: Theme.disabledText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
                font.letterSpacing: 2
            }

            Repeater { // DB-backed recent list
                model: player.history
                delegate: Rectangle {
                    id: recentRow
                    objectName: "recentRow_" + index
                    required property int index
                    required property string path
                    required property string title
                    required property real lastPlayed
                    required property bool favorite
                    visible: index < 5
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? 40 : 0
                    radius: Theme.radius
                    color: rowMouse.containsMouse ? Theme.surfaceHover : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.animationFast } }

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: 130
                        verticalAlignment: Text.AlignVCenter
                        text: recentRow.title
                        color: Theme.primaryText
                        elide: Text.ElideMiddle
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeMedium
                    }
                    Text {
                        anchors { right: parent.right; rightMargin: 38; verticalCenter: parent.verticalCenter }
                        text: root.relTimeOf(recentRow.lastPlayed)
                        color: Theme.secondaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        onClicked: (m) => m.button === Qt.RightButton ? player.toggleFavorite(recentRow.path)
                                                                      : player.openHistory(recentRow.path)
                    }
                    Text {
                        objectName: "recentStar_" + recentRow.index
                        anchors { right: parent.right; rightMargin: Theme.spacingMedium; verticalCenter: parent.verticalCenter }
                        text: recentRow.favorite ? "★" : "☆"
                        color: recentRow.favorite ? Theme.accentHover : Theme.disabledText
                        font.pixelSize: Theme.fontSizeLarge
                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -6
                            cursorShape: Qt.PointingHandCursor
                            onClicked: player.toggleFavorite(recentRow.path)
                        }
                    }
                }
            }

            Repeater { // QSettings fallback when the library DB is unavailable
                model: player.historyAvailable ? [] : player.recentFiles.slice(0, 5)
                delegate: Rectangle {
                    id: legacyRow
                    objectName: "recentRow_" + index
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true
                    implicitHeight: 40
                    radius: Theme.radius
                    color: legacyMouse.containsMouse ? Theme.surfaceHover : "transparent"
                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingMedium
                        verticalAlignment: Text.AlignVCenter
                        text: root.baseName(legacyRow.modelData)
                        color: Theme.primaryText
                        elide: Text.ElideMiddle
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeMedium
                    }
                    MouseArea {
                        id: legacyMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: player.openFiles([player.fileUrl(legacyRow.modelData)])
                    }
                }
            }
        }
    }

    // --- Network buffering pill ----------------------------------------------
    Rectangle {
        objectName: "bufferingPill"
        visible: player.engine.buffering && player.hasMedia
        anchors.centerIn: parent
        width: bufRow.implicitWidth + 2 * Theme.spacingLarge
        height: bufRow.implicitHeight + 2 * Theme.spacingMedium
        radius: height / 2
        color: Theme.panelOverlay
        border.color: Theme.border
        Row {
            id: bufRow
            anchors.centerIn: parent
            spacing: Theme.spacingMedium
            Text {
                id: spinner
                anchors.verticalCenter: parent.verticalCenter
                text: "◜"
                color: Theme.accent
                font.pixelSize: Theme.fontSizeHeading
                RotationAnimator on rotation {
                    running: player.engine.buffering
                    from: 0; to: 360; duration: 900; loops: Animation.Infinite
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Buffering…") + (player.engine.cacheBufferedSeconds >= 1
                      ? "  " + Math.round(player.engine.cacheBufferedSeconds) + " s" : "")
                color: Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
            }
        }
    }

    // --- Media error banner ----------------------------------------------
    Rectangle {
        z: 50
        visible: player.engine.mediaError !== ""
        anchors.top: parent.top
        anchors.topMargin: player.hasMedia ? 72 : 14
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(errorMetrics.advanceWidth, Math.min(root.width - 96, 640)) + 32
        height: errorText.implicitHeight + 16
        color: Theme.panelOverlay
        border.color: Theme.error
        radius: Theme.radius

        TextMetrics {
            id: errorMetrics
            font: errorText.font
            text: player.engine.mediaError
        }

        Text {
            id: errorText
            anchors.centerIn: parent
            width: parent.width - 32
            text: player.engine.mediaError
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
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
        height: 96
        opacity: root.chromeVisible && player.hasMedia && !root.miniMode ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.scrimStrong }
            GradientStop { position: 0.6; color: Theme.scrimSoft }
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
                color: Theme.videoText
                elide: Text.ElideRight
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeHeading
                font.weight: Font.DemiBold
            }
            IconButton {
                objectName: "openButton"
                iconSource: root.iconBase + "folder-open.svg"
                tip: qsTr("Open file (Ctrl+O)")
                onClicked: fileDialog.open()
            }
            IconButton {
                objectName: "playlistButton"
                iconSource: root.iconBase + "playlist.svg"
                tip: qsTr("Playlist (P)")
                onClicked: root.playlistOpen = !root.playlistOpen
            }
            IconButton {
                objectName: "settingsButton"
                iconSource: root.iconBase + "settings.svg"
                tip: qsTr("Settings (Ctrl+,)")
                onClicked: settingsDialog.open()
            }
        }
    }

    // --- Mini-player chrome: drag the video to move; controls on hover ------------
    DragHandler {
        enabled: root.miniMode
        target: null
        onActiveChanged: if (active) root.startSystemMove()
    }
    Item {
        id: miniChrome
        objectName: "miniChrome"
        anchors.fill: parent
        visible: root.miniMode && player.hasMedia
        HoverHandler { id: miniHover }
        Rectangle {
            anchors.fill: parent
            color: Theme.scrimSoft
            opacity: miniHover.hovered ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.animationFast } }
            IconButton {
                objectName: "miniPlayButton"
                anchors.centerIn: parent
                iconSize: 28
                iconSource: root.iconBase + (player.engine.playing ? "pause.svg" : "play.svg")
                onClicked: player.engine.togglePlayPause()
            }
            Row {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: Theme.spacingSmall
                IconButton {
                    objectName: "miniExpandButton"
                    iconSource: root.iconBase + "fullscreen-exit.svg"
                    tip: qsTr("Back to normal (Ctrl+M)")
                    onClicked: root.toggleMini()
                }
                IconButton {
                    objectName: "miniCloseButton"
                    label: "✕"
                    implicitWidth: 40
                    tip: qsTr("Close")
                    onClicked: root.close()
                }
            }
        }
        Rectangle { // thin progress bar, always visible
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: player.engine.duration > 0 ? parent.width * player.engine.position / player.engine.duration : 0
            height: 3
            color: Theme.accent
        }
    }

    // --- Playlist panel ---------------------------------------------------
    PlaylistPanel {
        id: playlistPanel
        objectName: "playlistPanel"
        visible: root.playlistOpen && !root.miniMode
        anchors.top: parent.top
        anchors.topMargin: 56
        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingMedium
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 144
        radius: Theme.radiusLarge
        color: Theme.panelOverlay
        clip: true
    }

    // --- Bottom controls (floating over the video) -------------------------
    Rectangle {
        id: controlsBar
        objectName: "controlsBar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 136
        opacity: root.chromeVisible && player.hasMedia && !root.miniMode ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.scrimClear }
            GradientStop { position: 0.45; color: Theme.scrimSoft }
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
                    color: Theme.videoText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    // font.features (tnum) needs Qt 6.6; reserve width so digits do not jitter on 6.4.
                    Layout.minimumWidth: 56
                }
                TimeSlider {
                    id: timeline
                    objectName: "timeline"
                    Layout.fillWidth: true
                }
                Text {
                    text: timefmt.formatDuration(player.engine.duration)
                    color: Theme.videoTextSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    // font.features (tnum) needs Qt 6.6; reserve width so digits do not jitter on 6.4.
                    Layout.minimumWidth: 56
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                // Left: volume
                RowLayout {
                    Layout.preferredWidth: 260
                    spacing: Theme.spacingSmall
                    IconButton {
                        objectName: "muteButton"
                        iconSource: root.iconBase + (player.engine.muted ? "volume-mute.svg" : "volume-high.svg")
                        tip: qsTr("Mute (M)")
                        onClicked: player.engine.setMuted(!player.engine.muted)
                    }
                    VolumeSlider {
                        id: volumeBar
                        objectName: "volumeSlider"
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }

                // Center: transport
                IconButton {
                    objectName: "prevButton"
                    iconSource: root.iconBase + "skip-previous.svg"
                    tip: qsTr("Previous (Ctrl+Left)")
                    enabled: player.playlist.count > 1
                    onClicked: player.previous()
                }
                Button {
                    id: playButton
                    objectName: "playButton"
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
                    objectName: "nextButton"
                    iconSource: root.iconBase + "skip-next.svg"
                    tip: qsTr("Next (Ctrl+Right)")
                    enabled: player.playlist.count > 1
                    onClicked: player.next()
                }

                Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }

                // Right: speed, chapters, fullscreen
                RowLayout {
                    Layout.preferredWidth: 260
                    layoutDirection: Qt.RightToLeft
                    spacing: Theme.spacingSmall

                    IconButton {
                        objectName: "moreButton"
                        iconSource: root.iconBase + "more.svg"
                        tip: qsTr("More")
                        onClicked: moreMenu.openAbove(this)
                    }

                    IconButton {
                        objectName: "fullscreenButton"
                        iconSource: root.visibility === Window.FullScreen
                                    ? root.iconBase + "fullscreen-exit.svg"
                                    : root.iconBase + "fullscreen.svg"
                        tip: qsTr("Fullscreen (F)")
                        onClicked: root.toggleFullscreen()
                    }
                    IconButton {
                        objectName: "tracksButton"
                        iconSource: root.iconBase + "subtitles.svg"
                        tip: qsTr("Audio & subtitles")
                        onClicked: trackMenu.openAbove(this)
                    }
                    IconButton {
                        objectName: "chaptersButton"
                        iconSource: root.iconBase + "chapters.svg"
                        tip: qsTr("Chapters (PgUp/PgDn)")
                        visible: player.engine.chapters.length > 0
                        onClicked: chapterMenu.openAbove(this)
                    }
                    IconButton {
                        objectName: "speedButton"
                        label: player.engine.speed.toFixed(2).replace(/0$/, "").replace(/\.$/, "") + "×"
                        tip: qsTr("Speed (X / C / Z)")
                        onClicked: speedMenu.openAbove(this)
                    }
                }
            }
        }
    }

    // --- Menus ---------------------------------------------------------------
    component ThemedPanel: PopupPanel {
        onAboutToShow: root.menuOpen = true
        onClosed: root.menuOpen = false
    }

    ThemedPanel {
        id: speedMenu
        objectName: "speedMenu"
        panelWidth: 180
        Repeater {
            model: [0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0]
            delegate: MenuRow {
                required property var modelData
                objectName: "speedItem_" + modelData
                text: modelData.toFixed(2).replace(/0$/, "").replace(/\.$/, "") + "×"
                mark: "radio"
                selected: Math.abs(player.engine.speed - modelData) < 0.001
                onTriggered: player.engine.setSpeed(modelData)
            }
        }
    }

    ThemedPanel {
        id: chapterMenu
        objectName: "chapterMenu"
        panelWidth: 300
        Repeater {
            model: player.engine.chapters
            delegate: MenuRow {
                required property var modelData
                required property int index
                objectName: "chapterItem_" + index
                text: (modelData.title || qsTr("Chapter %1").arg(index + 1))
                hint: timefmt.formatDuration(modelData.time)
                onTriggered: player.engine.seekChapter(index)
            }
        }
    }

    ThemedPanel {
        id: moreMenu
        objectName: "moreMenu"
        panelWidth: 300
        MenuSectionHeader { text: qsTr("ASPECT RATIO") }
        Flow {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.bottomMargin: 4
            spacing: 6
            Repeater {
                model: ["auto", "16:9", "4:3", "21:9", "1:1"]
                delegate: MenuChip {
                    required property string modelData
                    objectName: "aspectItem_" + modelData
                    text: modelData === "auto" ? qsTr("Auto") : modelData
                    selected: player.engine.aspect === modelData
                    onTriggered: player.engine.setAspect(modelData)
                }
            }
        }
        MenuSectionHeader { text: qsTr("ROTATE") }
        Flow {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.bottomMargin: 4
            spacing: 6
            Repeater {
                model: [0, 90, 180, 270]
                delegate: MenuChip {
                    required property int modelData
                    objectName: "rotateItem_" + modelData
                    text: modelData + "°"
                    selected: player.engine.rotate === modelData
                    onTriggered: player.engine.setRotate(modelData)
                }
            }
        }
        PanelSeparator {}
        MenuRow {
            objectName: "deinterlaceItem"
            text: qsTr("Deinterlace")
            mark: "check"
            selected: player.engine.deinterlace
            onTriggered: player.engine.setDeinterlace(!player.engine.deinterlace)
        }
        MenuRow {
            objectName: "onTopItem"
            text: qsTr("Always on top")
            hint: "T"
            mark: "check"
            selected: (root.flags & Qt.WindowStaysOnTopHint) !== 0
            onTriggered: root.flags = root.flags ^ Qt.WindowStaysOnTopHint
        }
        MenuRow {
            objectName: "miniItem"
            text: qsTr("Mini player")
            hint: "Ctrl+M"
            mark: "check"
            selected: root.miniMode
            onTriggered: root.toggleMini()
        }
        PanelSeparator {}
        MenuRow {
            objectName: "adjustItem"
            text: qsTr("Video adjustments…")
            onTriggered: adjustPopup.open()
        }
        MenuRow {
            objectName: "equalizerItem"
            text: qsTr("Equalizer…")
            onTriggered: eqPopup.open()
        }
        MenuRow {
            objectName: "bookmarksItem"
            text: qsTr("Bookmarks…")
            hint: "B"
            enabled: player.hasMedia
            onTriggered: bookmarksPopup.open()
        }
        PanelSeparator {}
        MenuRow {
            objectName: "screenshotItem"
            text: qsTr("Screenshot")
            hint: "Ctrl+S"
            onTriggered: player.takeScreenshot()
        }
        MenuRow {
            objectName: "openUrlItem"
            text: qsTr("Open URL…")
            hint: "Ctrl+U"
            onTriggered: openUrlDialog.open()
        }
        MenuRow {
            objectName: "resetFileItem"
            text: qsTr("Reset for this file")
            enabled: player.hasMedia
            onTriggered: player.resetFileSettings()
        }
    }

    VideoAdjustPopup {
        id: adjustPopup
        objectName: "adjustPopup"
        x: root.width - width - Theme.spacingLarge
        y: root.height - height - 150
        onAboutToShow: root.menuOpen = true
        onClosed: root.menuOpen = false
    }

    EqualizerPopup {
        id: eqPopup
        objectName: "eqPopup"
        x: root.width - width - Theme.spacingLarge
        y: root.height - height - 150
        onAboutToShow: root.menuOpen = true
        onClosed: root.menuOpen = false
    }

    BookmarksPopup {
        id: bookmarksPopup
        objectName: "bookmarksPopup"
        x: root.width - width - Theme.spacingLarge
        y: root.height - height - 150
        onAboutToShow: root.menuOpen = true
        onClosed: root.menuOpen = false
    }

    // --- Toast (also usable by other features: toast.show(text) or player.toast signal)
    Toast {
        id: toast
        objectName: "toast"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 150
    }
    Connections {
        target: player
        function onToast(text) { toast.show(text) }
    }

    TrackMenu {
        id: trackMenu
        objectName: "trackMenu"
        engine: player.engine
        onAboutToShow: root.menuOpen = true
        onClosed: root.menuOpen = false
    }

    // --- Idle auto-hide + cursor -----------------------------------------------
    Timer {
        id: hideTimer
        interval: settingsDialog.hideDelay * 1000
        repeat: false
        onTriggered: {
            if (!player.engine.playing) return
            if (root.chromeHovered || root.menuOpen || root.playlistOpen) restart()  // re-check once the menu closes
            else root.chromeVisible = false
        }
    }

    Connections {
        target: player.engine
        function onPausedChanged() { root.showChrome() }
    }

    // Volume OSD: any volume change (keys, wheel, laptop keys, OS slider) shows the bar and a toast.
    function volumeOsd() {
        if (volumeBar.pressed) return
        showChrome()
        toast.show(qsTr("Volume %1%").arg(player.systemVolume >= 0 ? player.systemVolume : player.engine.volume))
    }
    Connections {
        target: player
        function onSystemVolumeChanged() { root.volumeOsd() }
    }
    Connections {
        target: player.engine
        enabled: player.systemVolume < 0
        function onVolumeChanged() { root.volumeOsd() }
    }

    HoverHandler {
        cursorShape: root.chromeVisible || !player.hasMedia ? Qt.ArrowCursor : Qt.BlankCursor
        onPointChanged: root.showChrome()
    }

    SettingsDialog {
        id: settingsDialog
        objectName: "settingsDialog"
        onEqualizerRequested: eqPopup.open()
    }

    LibraryView {
        id: libraryView
        objectName: "libraryView"
    }

    // --- Drag & drop ----------------------------------------------------------
    DropArea {
        anchors.fill: parent
        onEntered: (drag) => drag.accept(Qt.CopyAction)
        onDropped: (drop) => {
            if (drop.hasUrls) {
                player.openFiles(drop.urls)
                root.showChrome()
            } else if (drop.hasText) {
                player.openUrl(drop.text)
                root.showChrome()
            }
        }
    }

    // --- Keyboard shortcuts: data-driven by ShortcutManager (spec §10) ----------
    function runAction(id) {
        const e = player.engine
        switch (id) {
        case "playPause": e.togglePlayPause(); break
        case "seekBack": e.seekRelative(-settingsDialog.seekStep); break
        case "seekForward": e.seekRelative(settingsDialog.seekStep); break
        case "volumeUp": case "volumeDown": {
            const cur = player.systemVolume >= 0 ? player.systemVolume : e.volume
            const v = Math.max(0, Math.min(player.systemVolume >= 0 ? 100 : 150, cur + (id === "volumeUp" ? 5 : -5)))
            player.setVolumePercent(v); break
        }
        case "mute": e.setMuted(!e.muted); break
        case "stats": e.statsEnabled = !e.statsEnabled; break
        case "fullscreen": root.toggleFullscreen(); break
        case "alwaysOnTop": root.flags = root.flags ^ Qt.WindowStaysOnTopHint; break
        case "audioTrack": e.cycleAudioTrack(); break
        case "subTrack": e.cycleSubtitleTrack(); break
        case "subShow": e.setSubtitlesVisible(true); break
        case "subHide": e.setSubtitlesVisible(false); break
        case "speedDown": e.setSpeed(Math.max(0.25, e.speed - 0.25)); break
        case "speedUp": e.setSpeed(Math.min(2.0, e.speed + 0.25)); break
        case "speedReset": e.setSpeed(1.0); break
        case "frameForward": e.stepFrame(1); break
        case "frameBack": e.stepFrame(-1); break
        case "subDelayDown": e.setSubtitleDelay(e.subtitleDelay - 0.1); break
        case "subDelayUp": e.setSubtitleDelay(e.subtitleDelay + 0.1); break
        case "chapterNext": root.stepChapter(1); break
        case "chapterPrev": root.stepChapter(-1); break
        case "bookmark": player.addBookmark(); break
        case "playlist": root.playlistOpen = !root.playlistOpen; break
        case "next": player.next(); break
        case "previous": player.previous(); break
        case "loopA": e.setLoopA(); break
        case "loopB": e.setLoopB(); break
        case "loopClear": e.clearLoop(); break
        case "screenshot": player.takeScreenshot(); break
        case "shuffle": player.shuffle = !player.shuffle; break
        case "repeat": player.repeatMode = (player.repeatMode + 1) % 3; break
        case "openFile": fileDialog.open(); break
        case "openUrl": openUrlDialog.open(); break
        case "library": libraryView.open(); break
        case "settings": settingsDialog.open(); break
        case "exitFullscreen": root.showNormal(); break
        }
    }
    function actionEnabled(a) {
        if (shortcuts.capturing) return false
        if (a.media && !player.hasMedia) return false
        if (a.id === "library") return !libraryView.opened && !settingsDialog.opened
        if (a.id === "exitFullscreen")
            return root.visibility === Window.FullScreen && !settingsDialog.opened && !libraryView.opened
        return true
    }
    Instantiator { // not Repeater: Shortcut is not an Item
        model: shortcuts.actions
        delegate: Shortcut {
            required property var modelData
            sequence: modelData.key
            enabled: root.actionEnabled(modelData)
            onActivated: root.runAction(modelData.id)
        }
    }
    Shortcut { sequence: "Ctrl+M";       enabled: player.hasMedia; onActivated: root.toggleMini() }
    Shortcut { sequence: "Esc";          enabled: root.miniMode; onActivated: root.toggleMini() }
}
