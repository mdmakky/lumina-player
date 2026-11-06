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
        // Last-used monitor (by name) first, so geometry / fullscreen land where the user left it.
        const lastScreen = player.setting("window/screen", "")
        const scr = Qt.application.screens.filter(sc => sc.name === lastScreen)
        if (scr.length > 0 && scr[0] !== screen) {
            screen = scr[0]
            x = scr[0].virtualX + Math.round((scr[0].width - width) / 2)
            y = scr[0].virtualY + Math.round((scr[0].height - height) / 2)
        }
        const bl = player.setting("window/borderless", false)
        if (bl === true || bl === "true")
            setBorderless(true)
        const g = player.setting("window/geometry", null)
        if (g && g.width >= minimumWidth && g.height >= minimumHeight) {
            // Keep the restored window fully on the current screen (a saved size from a bigger monitor
            // used to spill past the edges and hide the controls).
            const sw = screen.width, sh = screen.height - 60
            width = Math.min(g.width, sw); height = Math.min(g.height, sh)
            x = Math.max(screen.virtualX, Math.min(g.x, screen.virtualX + sw - width))
            y = Math.max(screen.virtualY + 30, Math.min(g.y, screen.virtualY + sh - height + 30))
        }
        const max = player.setting("window/maximized", false)
        if (max === true || max === "true")
            showMaximized()
        const mini = player.setting("window/mini", false)
        if (mini === true || mini === "true")
            Qt.callLater(toggleMini)
    }
    onScreenChanged: if (screen && visible) player.setSetting("window/screen", screen.name)
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

    // Screen under the window centre (the platform may report a different one on multi-monitor setups).
    function currentScreen() {
        const cx = x + width / 2, cy = y + height / 2
        for (const sc of Qt.application.screens)
            if (cx >= sc.virtualX && cx < sc.virtualX + sc.width && cy >= sc.virtualY && cy < sc.virtualY + sc.height)
                return sc
        return screen
    }

    function toggleFullscreen() {
        if (miniMode) toggleMini()
        if (visibility === Window.FullScreen) {
            showNormal()
        } else {
            // Fullscreen on the monitor the window is on (not the primary one).
            if (visibility === Window.Windowed) { const sc = currentScreen(); if (sc !== screen) screen = sc }
            showFullScreen()
        }
    }

    // Moves the window (and fullscreen) to the next monitor, centred. On Wayland compositors
    // that ignore client positions only the fullscreen case relocates; windowed placement is up to the compositor.
    function nextMonitor() {
        const list = Qt.application.screens
        if (list.length < 2) { toast.show(qsTr("Only one monitor detected")); return }
        const cur = currentScreen()
        const next = list[(list.indexOf(cur) + 1) % list.length]
        const wasFull = visibility === Window.FullScreen
        if (wasFull) showNormal()
        screen = next
        x = next.virtualX + Math.round((next.width - width) / 2)
        y = next.virtualY + Math.round((next.height - height) / 2)
        if (wasFull) Qt.callLater(showFullScreen)
        player.setSetting("window/screen", next.name)
    }

    // Borderless: no title bar/frame; move by dragging the video, resize from the edges.
    property bool borderless: false
    function setBorderless(on) {
        if (miniMode) return
        borderless = on
        flags = on ? (flags | Qt.FramelessWindowHint) : (flags & ~Qt.FramelessWindowHint)
        player.setSetting("window/borderless", on)
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
        onAccepted: player.openUserFiles(selectedUrls)
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

    MediaInfoDialog {
        id: mediaInfoDialog
        objectName: "mediaInfoDialog"
        onAboutToShow: root.menuOpen = true
        onClosed: root.menuOpen = false
    }

    // Click / wheel / drag actions are configurable (Settings > Mouse); see MouseActions.qml.
    MouseActions {
        id: mouseActions
        objectName: "mouseActions"
        anchors.fill: parent
        visible: player.hasMedia
        playlistOpen: root.playlistOpen
        gesturesEnabled: !root.miniMode
        seekStep: settingsDialog.seekStep
        onCloseOverlays: root.playlistOpen = false
        onFullscreenRequested: root.toggleFullscreen()
        onMenuRequested: { root.showChrome(); moreMenu.openAbove(moreButton) }
        onFeedback: (t) => toast.show(t)
        borderless: root.borderless && root.visibility === Window.Windowed
        onWindowMoveRequested: root.startSystemMove()
    }
    PanDragLayer {
        anchors.fill: parent
        enabled: player.hasMedia && !root.miniMode
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
    HomeView {
        anchors.fill: parent
        visible: !player.hasMedia
        onOpenFileRequested: fileDialog.open()
        onOpenUrlRequested: openUrlDialog.open()
        onLibraryRequested: libraryView.open()
        onWindowMoveRequested: if (root.borderless && root.visibility === Window.Windowed) root.startSystemMove()
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
        visible: root.playlistOpen && player.hasMedia && !root.miniMode
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
                        id: moreButton
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
            objectName: "borderlessItem"
            text: qsTr("Borderless window")
            hint: "Ctrl+B"
            mark: "check"
            selected: root.borderless
            enabled: !root.miniMode
            onTriggered: root.setBorderless(!root.borderless)
        }
        MenuRow {
            objectName: "nextMonitorItem"
            text: qsTr("Move to next monitor")
            hint: "Ctrl+Shift+M"
            enabled: Qt.application.screens.length > 1
            onTriggered: root.nextMonitor()
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
            objectName: "pictureItem"
            text: qsTr("Picture…")
            onTriggered: picturePopup.open()
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
            objectName: "mediaInfoItem"
            text: qsTr("Media information…")
            hint: "Ctrl+I"
            enabled: player.hasMedia
            onTriggered: mediaInfoDialog.open()
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

    PicturePopup {
        id: picturePopup
        objectName: "picturePopup"
        x: root.width - width - Theme.spacingLarge
        y: Math.max(8, root.height - height - 150)
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
            // A resting cursor over the bar must not keep it up (like mpv/VLC); only an open menu or an active drag does.
            if (root.menuOpen || root.playlistOpen || timeline.pressed || volumeBar.pressed) restart()  // re-check later
            else root.chromeVisible = false
        }
    }

    Connections {
        target: player.engine
        function onPausedChanged() { root.showChrome() }
    }

    // Volume OSD: any volume change (keys, wheel, laptop keys, OS slider) shows the bar and a toast.
    property bool osdArmed: false // startup volume restore / first OS reading must not flash the bar and a toast
    Timer { interval: 2500; running: true; onTriggered: root.osdArmed = true }
    function volumeOsd() {
        if (!osdArmed || volumeBar.pressed || !player.hasMedia) return
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
        // Qt re-delivers the (unchanged) pointer position every video frame; only a real move may wake the chrome,
        // otherwise a resting cursor keeps the controls visible forever.
        property real lastX: -1
        property real lastY: -1
        cursorShape: root.chromeVisible || !player.hasMedia ? Qt.ArrowCursor : Qt.BlankCursor
        onPointChanged: {
            const p = point.position
            if (Math.abs(p.x - lastX) < 1 && Math.abs(p.y - lastY) < 1) return
            lastX = p.x
            lastY = p.y
            root.showChrome()
        }
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
                player.openUserFiles(drop.urls)
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
        case "zoomIn": case "zoomOut": case "zoomReset": {
            const z = id === "zoomReset" ? 0 : Math.max(-2, Math.min(3, e.picture.zoom + (id === "zoomIn" ? 0.1 : -0.1)))
            e.setPicture("zoom", Math.abs(z) < 0.05 ? 0 : z)
            toast.show(qsTr("Zoom %1%").arg(Math.round(Math.pow(2, e.picture.zoom) * 100))); break
        }
        case "mediaInfo": mediaInfoDialog.open(); break
        case "borderless": root.setBorderless(!root.borderless); break
        case "nextMonitor": root.nextMonitor(); break
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
        case "subPosUp": case "subPosDown": settingsDialog.setSubPos(Math.max(0, Math.min(100, settingsDialog.subPos + (id === "subPosUp" ? -2 : 2)))); break
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
    ResizeGrips {
        objectName: "resizeGrips"
        anchors.fill: parent
        window: root
        z: 1000
        visible: root.borderless && root.visibility === Window.Windowed && !root.miniMode
    }
    Shortcut { sequence: "Ctrl+M";       enabled: player.hasMedia; onActivated: root.toggleMini() }
    Shortcut { sequence: "Esc";          enabled: root.miniMode; onActivated: root.toggleMini() }
}
