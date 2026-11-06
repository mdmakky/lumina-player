import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Lumina.Player

// Modal settings overlay: sidebar of pages (icons collapse to a rail on narrow
// windows), global search across all pages, one scrolling content column.
// Values persist through player.setSetting and apply live via the engine.
Item {
    id: dlg

    property bool opened: false
    signal equalizerRequested()

    // Current values (loaded from QSettings at startup so Main.qml can use hideDelay / seekStep).
    property bool resume: true
    property bool autoNext: true
    property bool seekPreview: true
    property string hwdec: "auto-safe"
    property real defaultSpeed: 1.0
    property int seekStep: 5
    property string videoAspect: "auto"
    property bool deinterlace: false
    property string shotFormat: "png"
    property string shotDir: ""
    property int defaultVolume: 100
    property int subSize: 40
    property string subColor: "#FFFFFF"
    property real subBorder: 3
    property int subPos: 100
    property string subLangs: "bn,ben,en,eng"
    property real hideDelay: 2.6
    property string themeMode: "dark"
    property bool reducedMotion: false
    property int cacheMiB: 192
    property int readahead: 30
    property string mpvOptions: ""
    property string mpvStatus: ""
    property bool verboseLog: false
    property var about: ({})
    // Read-only probes for the UI self-test.
    readonly property string activeTheme: Theme.mode
    readonly property int animFast: Theme.animationFast
    readonly property int animMedium: Theme.animationMedium
    readonly property int currentPage: SettingsState.page

    readonly property var colorPresets: [
        { label: qsTr("White"),  value: "#FFFFFF", id: "FFFFFF" },
        { label: qsTr("Yellow"), value: "#FFE45C", id: "FFE45C" },
        { label: qsTr("Cyan"),   value: "#6FE3F5", id: "6FE3F5" },
        { label: qsTr("Green"),  value: "#7DE38D", id: "7DE38D" }
    ]
    readonly property var pages: [
        { key: "general",    label: qsTr("General") },
        { key: "playback",   label: qsTr("Playback") },
        { key: "video",      label: qsTr("Video") },
        { key: "audio",      label: qsTr("Audio") },
        { key: "subtitles",  label: qsTr("Subtitles") },
        { key: "appearance", label: qsTr("Appearance") },
        { key: "library",    label: qsTr("Library") },
        { key: "shortcuts",  label: qsTr("Shortcuts") },
        { key: "advanced",   label: qsTr("Advanced") },
        { key: "mouse",      label: qsTr("Mouse") },
        { key: "rendering",  label: qsTr("Rendering") },
        { key: "network",    label: qsTr("Network") },
        { key: "playlist",   label: qsTr("Playlist") },
        { key: "about",      label: qsTr("About") }
    ]
    readonly property var pageItems: [pGeneral, pPlayback, pVideo, pAudio, pSubtitles,
                                      pAppearance, pLibrary, pShortcuts, pAdvanced, pMouse, pRendering, pNetwork, pPlaylist, pAbout]
    property int resetPending: -1   // page awaiting reset confirmation

    function set(key, value) { player.setSetting(key, value) }
    function getBool(key, def) { const v = player.setting(key, def); return v === true || v === "true" }
    function getNum(key, def) { return Number(player.setting(key, def)) }

    function setResume(v) { resume = v; set("playback/resume", v) }
    function setAutoNext(v) { autoNext = v; set("playback/autoNext", v) }
    function setSeekPreview(v) { seekPreview = v; set("ui/seekPreview", v); player.thumbs.enabled = v }
    function setHwdec(v) { hwdec = v; set("playback/hwdec", v); player.engine.setHwdec(v) }
    function setDefaultSpeed(v) { defaultSpeed = v; set("playback/defaultSpeed", v); player.engine.setSpeed(v) }
    function setSeekStep(v) { seekStep = v; set("playback/seekStep", v) }
    function setVideoAspect(v) { videoAspect = v; set("video/aspect", v); player.engine.setAspect(v) }
    function setDeinterlace(v) { deinterlace = v; set("video/deinterlace", v); player.engine.setDeinterlace(v) }
    function setShotFormat(v) { shotFormat = v; set("video/screenshotFormat", v) }
    function setShotDir(v) { shotDir = v; set("video/screenshotDir", v) }
    function setDefaultVolume(v) { defaultVolume = v; set("playback/defaultVolume", v); player.engine.setVolume(v) }
    function setSubSize(v) { subSize = v; set("subtitles/fontSize", v); player.engine.setSubtitleFontSize(v) }
    function setSubColor(v) { subColor = v; set("subtitles/color", v); player.engine.setSubtitleColor(v) }
    function setSubBorder(v) { subBorder = v; set("subtitles/borderSize", v); player.engine.setSubtitleBorderSize(v) }
    function setSubPos(v) { subPos = v; set("subtitles/position", v); player.engine.setSubtitlePosition(v) }
    function setSubLangs(v) { subLangs = v; set("subtitles/languages", v); player.engine.setSubtitleLanguages(v) }
    function setHideDelay(v) { hideDelay = v; set("interface/hideDelay", v) }
    function setThemeMode(v) { themeMode = v; Theme.mode = v; set("interface/theme", v) }
    function setReducedMotion(v) { reducedMotion = v; Theme.reducedMotion = v; set("interface/reducedMotion", v) }
    function setCacheMiB(v) { cacheMiB = v; set("advanced/cacheMiB", v); player.engine.setCacheSize(v) }
    function setReadahead(v) { readahead = v; set("advanced/readahead", v); player.engine.setReadahead(v) }
    function setVerboseLog(v) { verboseLog = v; set("advanced/verboseLog", v); player.setVerboseLogging(v) }
    // Applies "name=value" lines to the running engine and remembers them for the next start.
    function applyMpvOptions(text) {
        mpvOptions = text
        set("advanced/mpvOptions", text)
        let ok = 0
        const errors = []
        for (const line of text.split("\n")) {
            const t = line.trim()
            if (t === "") continue
            const eq = t.indexOf("=")
            const err = eq > 0 ? player.engine.setMpvOption(t.substring(0, eq).trim(), t.substring(eq + 1).trim())
                               : qsTr("expected name=value")
            if (err === "") ++ok
            else errors.push(t + " → " + err)
        }
        mpvStatus = errors.length ? errors.join("\n") : qsTr("Applied %n option(s).", "", ok)
    }

    function resetPageNow(i) {
        switch (i) {
        case 0: setResume(true); app.setLanguage("system"); break
        case 1:
            player.rememberFileSettings = true
            setHwdec("auto-safe"); setDefaultSpeed(1.0); setSeekStep(5); setAutoNext(true); setSeekPreview(true)
            break
        case 2:
            setVideoAspect("auto"); setDeinterlace(false); setShotFormat("png"); setShotDir("")
            for (const [k, v] of [["scale", "balanced"], ["tonemap", "auto"], ["interp", false]]) { player.engine.setPicture(k, v); set("video/" + k, v) }
            break
        case 3:
            setDefaultVolume(100)
            for (const [k, v, f] of [["audio/channels", "auto", player.engine.setAudioChannels], ["audio/normalization", "off", player.engine.setNormalization],
                                     ["audio/passthrough", false, player.engine.setAudioPassthrough], ["audio/visualization", "off", player.engine.setVisualization]]) { f(v); set(k, v) }
            SettingsState.resetTick++
            break
        case 4: setSubSize(40); setSubColor("#FFFFFF"); setSubBorder(3); setSubPos(100); setSubLangs("bn,ben,en,eng"); subStyle.resetStyle(); break
        case 5: setThemeMode("dark"); setReducedMotion(false); setHideDelay(2.6); accentSec.setAccent(""); break
        case 7: shortcuts.resetAll(); break
        case 8:
            setCacheMiB(192); setReadahead(30); setVerboseLog(false)
            mpvOptions = ""; set("advanced/mpvOptions", ""); mpvStatus = qsTr("Cleared. Restart to drop overrides already applied.")
            break
        }
    }
    function pageMatched(i) { return pageItems[i].matched }
    function gotoPage(i) { search.text = ""; SettingsState.page = i; flick.contentY = 0 }

    function open() { opened = true }
    function close() { opened = false }

    Component.onCompleted: loadValues()
    function loadValues() {
        resume = getBool("playback/resume", true)
        autoNext = getBool("playback/autoNext", true)
        seekPreview = getBool("ui/seekPreview", true)
        hwdec = player.setting("playback/hwdec", "auto-safe")
        defaultSpeed = getNum("playback/defaultSpeed", 1.0)
        seekStep = getNum("playback/seekStep", 5)
        videoAspect = player.setting("video/aspect", "auto")
        deinterlace = getBool("video/deinterlace", false)
        shotFormat = player.setting("video/screenshotFormat", "png")
        shotDir = player.setting("video/screenshotDir", "")
        defaultVolume = getNum("playback/defaultVolume", player.engine.volume)
        subSize = getNum("subtitles/fontSize", 40)
        subColor = player.setting("subtitles/color", "#FFFFFF")
        subBorder = getNum("subtitles/borderSize", 3)
        subPos = getNum("subtitles/position", 100)
        subLangs = player.setting("subtitles/languages", "bn,ben,en,eng")
        hideDelay = getNum("interface/hideDelay", 2.6)
        themeMode = player.setting("interface/theme", "dark")
        Theme.mode = themeMode
        reducedMotion = getBool("interface/reducedMotion", false)
        Theme.reducedMotion = reducedMotion
        cacheMiB = getNum("advanced/cacheMiB", 192)
        readahead = getNum("advanced/readahead", 30)
        mpvOptions = player.setting("advanced/mpvOptions", "")
        verboseLog = getBool("advanced/verboseLog", false)
    }

    // After a settings import: re-read the values and push them into the running engine/UI.
    function applyImported() {
        loadValues()
        shortcuts.reload()
        app.setLanguage(player.setting("interface/language", "system"))
        const e = player.engine
        e.setHwdec(hwdec); e.setAspect(videoAspect); e.setDeinterlace(deinterlace)
        e.setSubtitleFontSize(subSize); e.setSubtitleColor(subColor); e.setSubtitleBorderSize(subBorder)
        e.setSubtitlePosition(subPos); e.setSubtitleLanguages(subLangs)
        e.setCacheSize(cacheMiB); e.setReadahead(readahead)
        player.thumbs.enabled = seekPreview
        player.setVerboseLogging(verboseLog)
        if (mpvOptions !== "") applyMpvOptions(mpvOptions)
    }

    onOpenedChanged: {
        if (opened) {
            about = player.aboutInfo()
            Qt.callLater(function () { navRepeater.itemAt(SettingsState.page).forceActiveFocus() })
        } else {
            resetPending = -1
        }
    }

    anchors.fill: parent
    visible: opacity > 0
    opacity: opened ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }

    Shortcut {
        sequence: "Esc"
        enabled: dlg.opened && !shortcuts.capturing
        onActivated: {
            if (dlg.resetPending >= 0) dlg.resetPending = -1
            else if (search.text !== "") search.text = ""
            else dlg.close()
        }
    }

    FolderDialog {
        id: libFolderDialog
        title: qsTr("Add library folder")
        onAccepted: player.addLibraryFolder(selectedFolder)
    }
    FileDialog {
        id: exportDialog
        title: qsTr("Export settings")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Lumina settings (*.json)")]
        onAccepted: {
            const r = player.exportSettings(selectedFile)
            player.toast(r.ok ? qsTr("Settings exported") : qsTr("Export failed: %1").arg(r.error))
        }
    }
    FileDialog {
        id: importDialog
        title: qsTr("Import settings")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Lumina settings (*.json)")]
        onAccepted: {
            const r = player.importSettings(selectedFile)
            if (!r.ok) { player.toast(qsTr("Import failed: %1").arg(r.error)); return }
            dlg.applyImported()
            player.toast(qsTr("Imported %n setting(s). Library folders and window layout apply after a restart.", "", r.count))
        }
    }
    FolderDialog {
        id: shotFolderDialog
        title: qsTr("Screenshot folder")
        onAccepted: dlg.setShotDir(decodeURIComponent(selectedFolder.toString().replace(/^file:\/\/(\/(?=[A-Za-z]:))?/, "")))
    }

    // Scrim: swallows clicks; clicking outside the panel closes.
    Rectangle {
        anchors.fill: parent
        color: Theme.scrimSoft
        MouseArea { anchors.fill: parent; onClicked: dlg.close() }
    }

    component InfoValue: Text {
        Layout.maximumWidth: 360
        horizontalAlignment: Text.AlignRight
        wrapMode: Text.WordWrap
        color: Theme.secondaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMedium
    }

    // --- panel -----------------------------------------------------------------
    Rectangle {
        id: panel
        objectName: "settingsPanel"
        readonly property bool compact: width < 820
        anchors.centerIn: parent
        width: Math.min(940, parent.width - 48)
        height: Math.min(680, parent.height - 48)
        radius: Theme.radiusLarge
        color: "transparent"
        scale: dlg.opened ? 1 : 0.97
        Behavior on scale { NumberAnimation { duration: Theme.animationMedium; easing.type: Easing.OutCubic } }

        Surface { anchors.fill: parent; z: -1; elevation: 2; radius: panel.radius; color: Theme.panelOverlay }
        MouseArea { anchors.fill: parent } // keep clicks inside the panel

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // ---- sidebar -------------------------------------------------------
            Rectangle {
                objectName: "settingsSidebar"
                Layout.fillHeight: true
                Layout.preferredWidth: panel.compact ? 64 : 210
                color: SettingsState.card
                radius: Theme.radiusLarge
                Rectangle { // square off the right edge
                    anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: parent.radius; color: parent.color
                }
                Rectangle { // divider
                    anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: 1; color: Theme.border
                }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingMedium
                    anchors.rightMargin: Theme.spacingMedium + 1
                    spacing: 2
                    Text {
                        visible: !panel.compact
                        Layout.margins: Theme.spacingMedium
                        Layout.bottomMargin: Theme.spacingMedium
                        text: qsTr("Settings")
                        color: Theme.primaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeHeading
                        font.weight: Font.DemiBold
                        Accessible.role: Accessible.Heading
                    }
                    Item { visible: panel.compact; Layout.preferredHeight: 6 }
                    Repeater {
                        id: navRepeater
                        model: dlg.pages
                        delegate: Button {
                            id: nav
                            required property int index
                            required property var modelData
                            readonly property bool current: SettingsState.page === index
                            readonly property bool dimmed: SettingsState.q !== "" && !dlg.pageMatched(index)
                            objectName: "settingsNav_" + index
                            Layout.fillWidth: true
                            implicitHeight: 38
                            hoverEnabled: true
                            focusPolicy: Qt.StrongFocus
                            activeFocusOnTab: true
                            opacity: dimmed ? 0.4 : 1
                            text: modelData.label
                            onClicked: dlg.gotoPage(index)
                            Keys.onUpPressed: if (index > 0) { dlg.gotoPage(index - 1); navRepeater.itemAt(index - 1).forceActiveFocus() }
                            Keys.onDownPressed: if (index < dlg.pages.length - 1) { dlg.gotoPage(index + 1); navRepeater.itemAt(index + 1).forceActiveFocus() }
                            Accessible.role: Accessible.PageTab
                            Accessible.name: modelData.label
                            ThemedToolTip {
                                visible: panel.compact && nav.hovered
                                delay: 400
                                text: nav.modelData.label
                                x: nav.width + 6
                            }
                            background: Rectangle {
                                radius: Theme.radius
                                color: nav.current ? Theme.accentSoft : nav.pressed ? Theme.surfacePressed
                                     : nav.hovered ? Theme.surfaceHover : "transparent"
                                border.width: nav.visualFocus ? 2 : 0
                                border.color: Theme.focusRing
                            }
                            contentItem: Row {
                                spacing: Theme.spacingMedium
                                leftPadding: panel.compact ? 0 : Theme.spacingMedium
                                anchors.centerIn: panel.compact ? parent : undefined
                                Image {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 20; height: 20
                                    sourceSize: Qt.size(20, 20)
                                    source: "qrc:/qt/qml/Lumina/Player/icons/s-" + nav.modelData.key + (nav.current ? "-a" : "") + ".svg"
                                }
                                Text {
                                    visible: !panel.compact
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: nav.modelData.label
                                    color: nav.current ? Theme.accentHover : Theme.primaryText
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSizeMedium
                                    font.weight: nav.current ? Font.Medium : Font.Normal
                                }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // ---- main column ---------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Sticky header: page title, global search, close.
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 68
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 28
                        anchors.rightMargin: Theme.spacingMedium
                        spacing: Theme.spacingMedium
                        Text {
                            objectName: "settingsPageTitle"
                            Layout.fillWidth: true
                            text: SettingsState.q !== "" ? qsTr("Search results") : dlg.pages[SettingsState.page].label
                            elide: Text.ElideRight
                            color: Theme.primaryText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeHeading + 3
                            font.weight: Font.DemiBold
                            Accessible.role: Accessible.Heading
                            Accessible.name: text
                        }
                        TextField {
                            id: search
                            objectName: "settingsSearch"
                            Layout.preferredWidth: panel.compact ? 200 : 250
                            implicitHeight: 34
                            placeholderText: qsTr("Search settings")
                            color: Theme.primaryText
                            placeholderTextColor: Theme.secondaryText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeMedium
                            leftPadding: 12
                            rightPadding: 30
                            selectByMouse: true
                            Accessible.name: qsTr("Search settings")
                            background: Rectangle {
                                radius: Theme.radius
                                color: Theme.elevated
                                border.width: search.activeFocus ? 2 : 1
                                border.color: search.activeFocus ? Theme.focusRing : Theme.border
                            }
                            onTextChanged: {
                                SettingsState.query = text
                                flick.contentY = 0
                                if (SettingsState.q !== "") {
                                    for (let i = 0; i < dlg.pageItems.length; ++i)
                                        if (dlg.pageItems[i].matched) { SettingsState.page = i; break }
                                }
                            }
                            Text {
                                visible: search.text !== ""
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: "✕"
                                color: Theme.secondaryText
                                font.pixelSize: Theme.fontSizeMedium
                                MouseArea {
                                    objectName: "settingsSearchClear"
                                    anchors.fill: parent
                                    anchors.margins: -6
                                    onClicked: search.text = ""
                                }
                            }
                        }
                        Button {
                            id: closeBtn
                            objectName: "settingsCloseButton"
                            implicitWidth: 32; implicitHeight: 32
                            hoverEnabled: true
                            onClicked: dlg.close()
                            Accessible.name: qsTr("Close settings")
                            background: Rectangle {
                                radius: Theme.radius
                                color: closeBtn.hovered ? Theme.surfaceHover : "transparent"
                                border.width: closeBtn.visualFocus ? 2 : 0
                                border.color: Theme.focusRing
                            }
                            contentItem: Text {
                                text: "✕"
                                color: Theme.secondaryText
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeLarge
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    Rectangle { // shadow line once content scrolls under the header
                        anchors.bottom: parent.bottom
                        width: parent.width; height: 1
                        color: Theme.border
                        opacity: flick.contentY > 2 ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: Theme.animationFast } }
                    }
                }

                // ---- scrolling content -----------------------------------------
                Flickable {
                    id: flick
                    objectName: "settingsScroll"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: width
                    contentHeight: pageCol.implicitHeight + 40
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {
                        id: vbar
                        policy: flick.contentHeight > flick.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                        implicitWidth: 10
                        contentItem: Rectangle {
                            implicitWidth: 6
                            radius: 3
                            color: vbar.pressed ? Theme.accent : Theme.secondaryText
                            opacity: vbar.pressed ? 1 : vbar.hovered ? 0.7 : 0.4
                        }
                        background: Item {}
                    }

                    Connections {
                        target: SettingsState
                        function onReveal(item) {
                            const y = item.mapToItem(flick.contentItem, 0, 0).y
                            if (y < flick.contentY + 4)
                                flick.contentY = Math.max(0, y - 12)
                            else if (y + item.height > flick.contentY + flick.height - 4)
                                flick.contentY = Math.max(0, Math.min(flick.contentHeight - flick.height, y + item.height - flick.height + 12))
                        }
                    }

                    ColumnLayout {
                        id: pageCol
                        x: Math.max(24, (flick.width - width) / 2)
                        y: 8
                        width: Math.min(640, flick.width - 48)
                        spacing: Theme.spacingLarge

                        Text {
                            objectName: "settingsNoResults"
                            visible: SettingsState.q !== "" && !dlg.pageItems.some(p => p.matched)
                            Layout.fillWidth: true
                            Layout.topMargin: 40
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("No settings match “%1”.").arg(SettingsState.query.trim())
                            color: Theme.secondaryText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeLarge
                        }

                        // ---- General ----
                        SettingsPage {
                            id: pGeneral
                            title: qsTr("General"); pageIndex: 0
                            onResetRequested: dlg.resetPending = pageIndex
                            SettingSection {
                                title: qsTr("Language")
                                ComboRow {
                                    name: "languageCombo"
                                    title: qsTr("Interface language")
                                    description: qsTr("Language of menus and buttons. Changes apply immediately.")
                                    keywords: "language locale translation bangla bengali english bn"
                                    model: [
                                        { label: qsTr("System default"), value: "system" },
                                        { label: "English", value: "en" },
                                        { label: "বাংলা", value: "bn" }
                                    ]
                                    value: app.language
                                    onPicked: (v) => app.setLanguage(v)
                                }
                            }
                            SettingSection {
                                title: qsTr("Backup")
                                ButtonRow {
                                    name: "exportSettings"
                                    title: qsTr("Settings file")
                                    description: qsTr("Save or load shortcuts, subtitle style, theme and language as one JSON file. History, library and resume positions are not included.")
                                    keywords: "export import backup settings json config transfer"
                                    buttonText: qsTr("Import settings…")
                                    button2Text: qsTr("Export settings…")
                                    onClicked: importDialog.open()
                                    onClicked2: exportDialog.open()
                                }
                            }
                            SettingSection {
                                title: qsTr("Startup and history")
                                SwitchRow {
                                    name: "resumeSwitch"
                                    title: qsTr("Remember playback position")
                                    description: qsTr("Resume files where you left off.")
                                    keywords: "resume continue history last position"
                                    checked: dlg.resume
                                    onToggled: (v) => dlg.setResume(v)
                                }
                                ButtonRow {
                                    name: "clearRecent"
                                    title: qsTr("Recent files")
                                    description: qsTr("Forget the list of recently opened files shown on the home screen.")
                                    keywords: "history clear recent forget privacy"
                                    buttonText: qsTr("Clear recent files")
                                    danger: true
                                    buttonEnabled: player.recentFiles.length > 0
                                    onClicked: { player.clearRecentFiles(); player.toast(qsTr("Recent files cleared")) }
                                }
                            }
                        }

                        // ---- Playback ----
                        SettingsPage {
                            id: pPlayback
                            title: qsTr("Playback"); pageIndex: 1
                            onResetRequested: dlg.resetPending = pageIndex
                            SettingSection {
                                title: qsTr("Decoding")
                                SettingRow {
                                    title: qsTr("Hardware decoding")
                                    description: qsTr("Auto-safe uses only well-tested decoders. Off is the most compatible.")
                                    keywords: "hwdec gpu vaapi videotoolbox decoder acceleration"
                                    SegmentedControl {
                                        name: "hwdecOption"
                                        accessibleName: qsTr("Hardware decoding")
                                        model: [
                                            { label: qsTr("Auto-safe"), value: "auto-safe" },
                                            { label: qsTr("Off"), value: "no" },
                                            { label: qsTr("Auto"), value: "auto" }
                                        ]
                                        value: dlg.hwdec
                                        onPicked: (v) => dlg.setHwdec(v)
                                    }
                                }
                            }
                            SettingSection {
                                title: qsTr("Controls")
                                ComboRow {
                                    name: "defaultSpeed"
                                    title: qsTr("Default speed")
                                    description: qsTr("Playback speed applied at startup and when you change it here.")
                                    keywords: "rate faster slower tempo"
                                    model: [
                                        { label: "0.5×", value: 0.5 }, { label: "0.75×", value: 0.75 },
                                        { label: qsTr("1× (normal)"), value: 1 }, { label: "1.25×", value: 1.25 },
                                        { label: "1.5×", value: 1.5 }, { label: "2×", value: 2 }
                                    ]
                                    value: dlg.defaultSpeed
                                    onPicked: (v) => dlg.setDefaultSpeed(v)
                                }
                                StepperRow {
                                    name: "seekStep"
                                    title: qsTr("Seek step")
                                    description: qsTr("How far the Left and Right arrow keys skip.")
                                    keywords: "skip jump arrow forward backward seconds"
                                    value: dlg.seekStep; from: 1; to: 60; step: 1; suffix: " s"
                                    onPicked: (v) => dlg.setSeekStep(v)
                                }
                                SwitchRow {
                                    name: "rememberFileSwitch"
                                    title: qsTr("Remember per-file settings")
                                    description: qsTr("Restore each file's video adjustments, aspect, rotation, audio and subtitle tracks, delays, speed and volume when you reopen it.")
                                    keywords: "profile per file video adjust brightness track subtitle delay speed volume remember"
                                    checked: player.rememberFileSettings
                                    onToggled: (v) => player.rememberFileSettings = v
                                }
                                SwitchRow {
                                    name: "autoNextSwitch"
                                    title: qsTr("Play next item automatically")
                                    description: qsTr("Continue with the next file in the queue when one ends.")
                                    keywords: "autoplay queue playlist advance"
                                    checked: dlg.autoNext
                                    onToggled: (v) => dlg.setAutoNext(v)
                                }
                                SwitchRow {
                                    name: "seekPreviewSwitch"
                                    title: qsTr("Show seek preview")
                                    description: qsTr("Show a thumbnail of the frame under the pointer when hovering or dragging the timeline.")
                                    keywords: "thumbnail hover timeline scrub tooltip frame"
                                    checked: dlg.seekPreview
                                    onToggled: (v) => dlg.setSeekPreview(v)
                                }
                            }
                        }

                        // ---- Video ----
                        SettingsPage {
                            id: pVideo
                            title: qsTr("Video"); pageIndex: 2
                            onResetRequested: dlg.resetPending = pageIndex
                            SettingSection {
                                title: qsTr("Picture")
                                ComboRow {
                                    name: "videoAspect"
                                    title: qsTr("Default aspect ratio")
                                    description: qsTr("Auto keeps each file's own shape.")
                                    keywords: "ratio stretch widescreen 16:9 4:3"
                                    model: [
                                        { label: qsTr("Auto"), value: "auto" }, { label: "16:9", value: "16:9" },
                                        { label: "4:3", value: "4:3" }, { label: "21:9", value: "21:9" },
                                        { label: "1:1", value: "1:1" }
                                    ]
                                    value: dlg.videoAspect
                                    onPicked: (v) => dlg.setVideoAspect(v)
                                }
                                SwitchRow {
                                    name: "deinterlaceSwitch"
                                    title: qsTr("Deinterlace")
                                    description: qsTr("Remove combing from interlaced broadcast or DVD sources.")
                                    keywords: "interlace comb broadcast dvd"
                                    checked: dlg.deinterlace
                                    onToggled: (v) => dlg.setDeinterlace(v)
                                }
                            }
                            VideoExtraRows {}
                            SettingSection {
                                title: qsTr("Screenshots")
                                SettingRow {
                                    title: qsTr("Format")
                                    description: qsTr("PNG is lossless; JPEG files are smaller.")
                                    keywords: "screenshot image png jpg jpeg capture"
                                    SegmentedControl {
                                        name: "shotFormat"
                                        accessibleName: qsTr("Screenshot format")
                                        model: [ { label: "PNG", value: "png" }, { label: "JPEG", value: "jpg" } ]
                                        value: dlg.shotFormat
                                        onPicked: (v) => dlg.setShotFormat(v)
                                    }
                                }
                                ButtonRow {
                                    name: "shotFolder"
                                    title: qsTr("Folder")
                                    description: dlg.shotDir !== "" ? dlg.shotDir : player.screenshotDir()
                                    keywords: "screenshot directory save location path"
                                    buttonText: qsTr("Choose…")
                                    button2Text: dlg.shotDir !== "" ? qsTr("Use default") : ""
                                    onClicked: shotFolderDialog.open()
                                    onClicked2: dlg.setShotDir("")
                                }
                            }
                        }

                        // ---- Audio ----
                        SettingsPage {
                            id: pAudio
                            title: qsTr("Audio"); pageIndex: 3
                            onResetRequested: dlg.resetPending = pageIndex
                            SettingSection {
                                title: qsTr("Sound")
                                SliderRow {
                                    name: "defaultVolume"
                                    title: qsTr("Default volume")
                                    description: qsTr("Volume used when Lumina starts. Above 100% boosts quiet files.")
                                    keywords: "loudness level startup boost"
                                    value: dlg.defaultVolume; from: 0; to: 150; step: 5; suffix: "%"
                                    onMoved: (v) => dlg.setDefaultVolume(v)
                                }
                                ButtonRow {
                                    name: "openEqualizer"
                                    title: qsTr("Equalizer")
                                    description: qsTr("Ten-band equalizer with presets, opened over the player.")
                                    keywords: "eq bass treble frequency tone"
                                    buttonText: qsTr("Open equalizer")
                                    onClicked: { dlg.close(); dlg.equalizerRequested() }
                                }
                            }
                            AudioOutputSection {}
                        }

                        // ---- Subtitles ----
                        SettingsPage {
                            id: pSubtitles
                            title: qsTr("Subtitles"); pageIndex: 4
                            onResetRequested: dlg.resetPending = pageIndex
                            SettingSection {
                                title: qsTr("Appearance")
                                StepperRow {
                                    name: "subSize"
                                    title: qsTr("Font size")
                                    keywords: "text caption"
                                    value: dlg.subSize; from: 20; to: 120; step: 5
                                    onPicked: (v) => dlg.setSubSize(v)
                                }
                                SettingRow {
                                    title: qsTr("Color")
                                    keywords: "colour text yellow white"
                                    SegmentedControl {
                                        name: "subColor"
                                        accessibleName: qsTr("Subtitle color")
                                        model: dlg.colorPresets
                                        value: dlg.subColor.toUpperCase()
                                        onPicked: (v) => dlg.setSubColor(v)
                                    }
                                }
                                StepperRow {
                                    name: "subBorder"
                                    title: qsTr("Border size")
                                    description: qsTr("Outline thickness that keeps text readable on bright scenes.")
                                    keywords: "outline stroke"
                                    value: dlg.subBorder; from: 0; to: 8; step: 0.5; decimals: 1
                                    onPicked: (v) => dlg.setSubBorder(v)
                                }
                                SliderRow {
                                    name: "subPos"
                                    title: qsTr("Vertical position")
                                    description: qsTr("0 is the top of the video, 100 the bottom.")
                                    keywords: "placement height move up down"
                                    value: dlg.subPos; from: 0; to: 100; step: 5
                                    onMoved: (v) => dlg.setSubPos(v)
                                }
                            }
                            SubtitleStyleRows { id: subStyle; dlg: dlg; thePlayer: player }
                            SettingSection {
                                title: qsTr("Language")
                                SettingRow {
                                    title: qsTr("Preferred languages")
                                    description: qsTr("Used to pick a subtitle track automatically when a file has several.")
                                    keywords: "bangla bengali english track automatic default slang"
                                    SegmentedControl {
                                        name: "subLang"
                                        accessibleName: qsTr("Preferred subtitle languages")
                                        model: [
                                            { label: qsTr("Bangla + English"), value: "bn,ben,en,eng", id: "bnen" },
                                            { label: qsTr("English"), value: "en,eng", id: "en" },
                                            { label: qsTr("Bangla"), value: "bn,ben", id: "bn" }
                                        ]
                                        value: dlg.subLangs
                                        onPicked: (v) => dlg.setSubLangs(v)
                                    }
                                }
                            }
                        }

                        // ---- Appearance ----
                        SettingsPage {
                            id: pAppearance
                            title: qsTr("Appearance"); pageIndex: 5
                            onResetRequested: dlg.resetPending = pageIndex
                            SettingSection {
                                title: qsTr("Theme")
                                SettingRow {
                                    title: qsTr("Theme")
                                    description: qsTr("Video controls stay dark in every theme.")
                                    keywords: "dark light oled black color mode night"
                                    SegmentedControl {
                                        name: "themeOption"
                                        accessibleName: qsTr("Theme")
                                        model: [
                                            { label: qsTr("Dark"), value: "dark" },
                                            { label: qsTr("Light"), value: "light" },
                                            { label: qsTr("OLED black"), value: "oled" }
                                        ]
                                        value: dlg.themeMode
                                        onPicked: (v) => dlg.setThemeMode(v)
                                    }
                                }
                                SwitchRow {
                                    name: "reducedMotionSwitch"
                                    title: qsTr("Reduce motion")
                                    description: qsTr("Turns off fades and slide animations.")
                                    keywords: "animation accessibility transitions"
                                    checked: dlg.reducedMotion
                                    onToggled: (v) => dlg.setReducedMotion(v)
                                }
                            }
                            AccentSection { id: accentSec; dlg: dlg }
                            SettingSection {
                                title: qsTr("Player controls")
                                StepperRow {
                                    name: "hideDelay"
                                    title: qsTr("Hide controls after")
                                    description: qsTr("Idle time before the controls fade out during playback.")
                                    keywords: "auto-hide idle overlay timeout"
                                    value: dlg.hideDelay; from: 1; to: 10; step: 0.5; decimals: 1; suffix: " s"
                                    onPicked: (v) => dlg.setHideDelay(v)
                                }
                            }
                        }

                        // ---- Library ----
                        SettingsPage {
                            id: pLibrary
                            title: qsTr("Library"); pageIndex: 6
                            resettable: false
                            SettingSection {
                                title: qsTr("Media folders")
                                ButtonRow {
                                    name: "libAddFolder"
                                    title: qsTr("Add a folder")
                                    description: player.libraryAvailable ? qsTr("Lumina scans it in the background and keeps the list up to date.")
                                                                         : qsTr("The library database is unavailable; playback is not affected.")
                                    keywords: "library scan media videos watch directory"
                                    buttonText: qsTr("Add folder…")
                                    primary: true
                                    buttonEnabled: player.libraryAvailable
                                    onClicked: libFolderDialog.open()
                                }
                                ButtonRow {
                                    name: "libRescan"
                                    title: qsTr("Rescan")
                                    description: player.libraryScanning ? qsTr("Scanning… %n file(s) found", "", player.libraryProgress)
                                                                        : qsTr("Look for added or removed files now.")
                                    keywords: "refresh update library"
                                    buttonText: qsTr("Rescan now")
                                    buttonEnabled: player.libraryAvailable && player.libraryFolders.length > 0 && !player.libraryScanning
                                    onClicked: player.rescanLibrary()
                                }
                                SettingRow {
                                    stacked: true
                                    title: qsTr("Watched folders")
                                    description: player.libraryFolders.length === 0 ? qsTr("No folders yet.")
                                                 : qsTr("Removing a folder leaves its files untouched.")
                                    keywords: "library folders remove list"
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        Repeater {
                                            model: player.libraryFolders
                                            delegate: RowLayout {
                                                id: folderRow
                                                required property int index
                                                required property string modelData
                                                Layout.fillWidth: true
                                                spacing: Theme.spacingMedium
                                                Text {
                                                    Layout.fillWidth: true
                                                    text: folderRow.modelData
                                                    elide: Text.ElideMiddle
                                                    color: Theme.primaryText
                                                    font.family: Theme.fontFamily
                                                    font.pixelSize: Theme.fontSizeMedium
                                                }
                                                SettingsButton {
                                                    objectName: "libFolderRemove_" + folderRow.index
                                                    implicitHeight: 28
                                                    text: qsTr("Remove")
                                                    Accessible.name: qsTr("Remove folder %1").arg(folderRow.modelData)
                                                    onClicked: player.removeLibraryFolder(folderRow.modelData)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // ---- Shortcuts ----
                        SettingsPage {
                            id: pShortcuts
                            title: qsTr("Shortcuts"); pageIndex: 7
                            resetLabel: qsTr("Reset all shortcuts to defaults")
                            onResetRequested: dlg.resetPending = pageIndex
                            SettingSection {
                                title: qsTr("Keyboard")
                                ShortcutsEditor { dialogOpen: dlg.opened }
                            }
                        }

                        // ---- Advanced ----
                        SettingsPage {
                            id: pAdvanced
                            title: qsTr("Advanced"); pageIndex: 8
                            onResetRequested: dlg.resetPending = pageIndex
                            SettingSection {
                                title: qsTr("Buffering")
                                ComboRow {
                                    name: "cacheSize"
                                    title: qsTr("Cache size")
                                    description: qsTr("Memory used to buffer the file; half is kept for seeking backwards.")
                                    keywords: "demuxer buffer memory ram network"
                                    model: [
                                        { label: "64 MiB", value: 64 }, { label: "128 MiB", value: 128 },
                                        { label: qsTr("192 MiB (default)"), value: 192 },
                                        { label: "512 MiB", value: 512 }, { label: "1024 MiB", value: 1024 }
                                    ]
                                    value: dlg.cacheMiB
                                    onPicked: (v) => dlg.setCacheMiB(v)
                                }
                                StepperRow {
                                    name: "readahead"
                                    title: qsTr("Read-ahead")
                                    description: qsTr("Seconds of media to load ahead of the playhead.")
                                    keywords: "demuxer buffer prefetch"
                                    value: dlg.readahead; from: 5; to: 120; step: 5; suffix: " s"
                                    onPicked: (v) => dlg.setReadahead(v)
                                }
                            }
                            SettingSection {
                                title: qsTr("Diagnostics")
                                SwitchRow {
                                    name: "verboseLogSwitch"
                                    title: qsTr("Verbose logging")
                                    description: qsTr("Print debug messages to the console. Useful when reporting a problem.")
                                    keywords: "log level debug console diagnostics"
                                    checked: dlg.verboseLog
                                    onToggled: (v) => dlg.setVerboseLog(v)
                                }
                                ButtonRow {
                                    name: "openLogsAdvanced"
                                    title: qsTr("Log files")
                                    description: player.logsPath
                                    keywords: "log folder diagnostics report bug"
                                    buttonText: qsTr("Open logs folder")
                                    onClicked: player.openLogsFolder()
                                }
                            }
                            SettingSection {
                                title: qsTr("Engine overrides")
                                SettingRow {
                                    stacked: true
                                    restartNeeded: true
                                    title: qsTr("mpv options")
                                    description: qsTr("Warning: for experts. One name=value per line, for example “deband=yes”. Wrong values can break playback; some options only take effect after a restart.")
                                    keywords: "mpv config override custom engine properties"
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.spacingMedium
                                        ScrollView {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 84
                                            clip: true
                                            background: Rectangle {
                                                radius: Theme.radius
                                                color: Theme.elevated
                                                border.width: mpvField.activeFocus ? 2 : 1
                                                border.color: mpvField.activeFocus ? Theme.focusRing : Theme.border
                                            }
                                            TextArea {
                                                id: mpvField
                                                objectName: "mpvOptionsField"
                                                text: dlg.mpvOptions
                                                placeholderText: "deband=yes"
                                                placeholderTextColor: Theme.disabledText
                                                color: Theme.primaryText
                                                selectByMouse: true
                                                font.family: Theme.fontMono
                                                font.pixelSize: Theme.fontSizeMedium
                                                wrapMode: TextEdit.NoWrap
                                                leftPadding: 10; topPadding: 8
                                                Accessible.name: qsTr("mpv options")
                                                background: Item {}
                                            }
                                        }
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: Theme.spacingMedium
                                            Text {
                                                Layout.fillWidth: true
                                                objectName: "mpvOptionsStatus"
                                                text: dlg.mpvStatus
                                                wrapMode: Text.WordWrap
                                                color: dlg.mpvStatus.indexOf("→") >= 0 ? Theme.error : Theme.success
                                                font.family: Theme.fontFamily
                                                font.pixelSize: Theme.fontSizeSmall
                                            }
                                            SettingsButton {
                                                objectName: "mpvOptionsApply"
                                                text: qsTr("Apply")
                                                primary: true
                                                onClicked: dlg.applyMpvOptions(mpvField.text)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        MouseSettingsPage { id: pMouse; pageIndex: 9 }
                        RenderingPage { id: pRendering; pageIndex: 10; dlg: dlg }
                        NetworkPage { id: pNetwork; pageIndex: 11; dlg: dlg }
                        PlaylistPage { id: pPlaylist; pageIndex: 12; dlg: dlg }

                        // ---- About ----
                        SettingsPage {
                            id: pAbout
                            title: qsTr("About"); pageIndex: 13
                            resettable: false
                            SettingSection {
                                title: qsTr("Lumina Player")
                                SettingRow {
                                    title: qsTr("Version")
                                    keywords: "build release"
                                    InfoValue { text: dlg.about.version || Qt.application.version }
                                }
                                SettingRow {
                                    title: qsTr("License")
                                    description: qsTr("Free software under the GNU General Public License v3.0.")
                                    keywords: "gpl gnu open source copyright"
                                    SettingsButton {
                                        text: qsTr("GPL-3.0")
                                        onClicked: Qt.openUrlExternally("https://www.gnu.org/licenses/gpl-3.0.html")
                                    }
                                }
                                SettingRow {
                                    title: qsTr("Source code")
                                    description: "github.com/mdmakky/lumina-player"
                                    keywords: "github repository issues bug report website"
                                    SettingsButton {
                                        objectName: "aboutRepo"
                                        text: qsTr("Open on GitHub")
                                        onClicked: Qt.openUrlExternally("https://github.com/mdmakky/lumina-player")
                                    }
                                }
                            }
                            SettingSection {
                                title: qsTr("System")
                                SettingRow {
                                    title: qsTr("Qt")
                                    keywords: "framework toolkit"
                                    InfoValue { text: dlg.about.qt || "" }
                                }
                                SettingRow {
                                    title: qsTr("mpv")
                                    keywords: "engine libmpv ffmpeg"
                                    InfoValue { text: ((dlg.about.mpv || "") + (dlg.about.ffmpeg ? "\nFFmpeg " + dlg.about.ffmpeg : "")) }
                                }
                                SettingRow {
                                    title: qsTr("Current decoder")
                                    description: qsTr("Updates while a file is playing.")
                                    keywords: "hwdec hardware software codec"
                                    InfoValue { text: dlg.about.decoder ? dlg.about.decoder : qsTr("Software / none active") }
                                }
                                SettingRow {
                                    title: qsTr("Renderer")
                                    keywords: "gpu graphics opengl video output"
                                    InfoValue { text: dlg.about.renderer || qsTr("Not initialised yet") }
                                }
                                SettingRow {
                                    title: qsTr("Operating system")
                                    keywords: "platform cpu architecture"
                                    InfoValue { text: dlg.about.os || "" }
                                }
                                ButtonRow {
                                    name: "openLogsAbout"
                                    title: qsTr("Log files")
                                    description: qsTr("Rotating logs, useful when reporting a problem.")
                                    keywords: "log folder diagnostics report bug"
                                    buttonText: qsTr("Open logs folder")
                                    onClicked: player.openLogsFolder()
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- reset confirmation ---------------------------------------------------
        Rectangle {
            id: confirm
            objectName: "settingsResetConfirmLayer"
            anchors.fill: parent
            radius: panel.radius
            visible: dlg.resetPending >= 0
            color: Theme.scrimSoft
            MouseArea { anchors.fill: parent; onClicked: dlg.resetPending = -1 }
            Rectangle {
                anchors.centerIn: parent
                width: Math.min(380, parent.width - 48)
                height: confirmCol.implicitHeight + 40
                radius: Theme.radiusLarge
                color: "transparent"
                Surface { anchors.fill: parent; z: -1; elevation: 2; color: Theme.elevated }
                MouseArea { anchors.fill: parent }
                ColumnLayout {
                    id: confirmCol
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: Theme.spacingMedium
                    Text {
                        Layout.fillWidth: true
                        text: dlg.resetPending >= 0 ? qsTr("Reset %1 settings?").arg(dlg.pages[dlg.resetPending].label) : ""
                        wrapMode: Text.WordWrap
                        color: Theme.primaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeLarge
                        font.weight: Font.DemiBold
                        Accessible.role: Accessible.AlertMessage
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Only this page is restored to its default values. Other pages are not affected.")
                        wrapMode: Text.WordWrap
                        color: Theme.secondaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeMedium
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        Layout.topMargin: Theme.spacingSmall
                        spacing: Theme.spacingMedium
                        SettingsButton {
                            objectName: "settingsResetCancel"
                            text: qsTr("Cancel")
                            onClicked: dlg.resetPending = -1
                        }
                        SettingsButton {
                            id: confirmBtn
                            objectName: "settingsResetConfirm"
                            text: qsTr("Reset")
                            primary: true
                            onClicked: { const i = dlg.resetPending; dlg.resetPending = -1; dlg.resetPageNow(i) }
                        }
                    }
                }
            }
            onVisibleChanged: if (visible) confirmBtn.forceActiveFocus()
        }
    }
}
