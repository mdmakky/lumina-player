import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Lumina.Player

// Optional media library: full-window overlay with sections (tabs), list or poster-grid view,
// filters/sorting, saved playlists and duplicate cleanup. Talks to the controller only
// (player.library, player.libraryPlaylists, player.addLibraryFolder, ...).
Item {
    id: view

    property bool opened: false
    property bool gridMode: false
    property var selected: ({})      // path -> true
    property int selCount: 0
    property var menuRow: ({})       // row the context menu / dialogs act on
    readonly property bool inPlaylists: player.library.tab === 7
    readonly property bool inDuplicates: player.library.tab === 8

    function open() { opened = true; search.forceActiveFocus() }
    function close() { opened = false; search.focus = false }

    function relTime(sec) {
        const d = Math.max(0, Date.now() / 1000 - sec)
        if (d < 90) return qsTr("just now")
        if (d < 3600) return qsTr("%1 min ago").arg(Math.round(d / 60))
        if (d < 86400) return qsTr("%1 h ago").arg(Math.round(d / 3600))
        if (d < 86400 * 30) return qsTr("%1 d ago").arg(Math.round(d / 86400))
        return new Date(sec * 1000).toLocaleDateString(Qt.locale(), Locale.ShortFormat)
    }
    function fmtSize(b) {
        b = Number(b)
        if (b >= 1073741824) return (b / 1073741824).toFixed(1) + " GB"
        if (b >= 1048576) return Math.round(b / 1048576) + " MB"
        return Math.max(1, Math.round(b / 1024)) + " KB"
    }
    function fmtDur(s) { return s > 0 ? timefmt.formatDuration(s) : "" }
    function resLabel(h) {
        return h <= 0 ? "" : h >= 2000 ? "4K" : h >= 1400 ? "1440p" : h >= 1000 ? "1080p" : h >= 700 ? "720p" : h + "p"
    }
    // Second line of a row/tile: resolution / codec / length / size for video, artist / album for music.
    function detail(kind, h, vcodec, dur, size, artist, album, year) {
        const p = []
        if (kind === 1) {
            if (artist) p.push(artist)
            if (album) p.push(year > 0 ? album + " (" + year + ")" : album)
        } else {
            if (resLabel(h)) p.push(resLabel(h))
            if (vcodec) p.push(vcodec.toUpperCase())
        }
        if (fmtDur(dur)) p.push(fmtDur(dur))
        if (kind !== 1 || p.length === 0) p.push(fmtSize(size))
        return p.join("  ·  ")
    }
    function toggleSel(path) {
        const n = {}
        for (const k in selected) n[k] = true
        if (n[path]) delete n[path]; else n[path] = true
        selected = n
        selCount = Object.keys(n).length
    }
    function clearSel() { selected = ({}); selCount = 0 }
    function rowPaths() { return selCount > 0 ? Object.keys(selected) : (menuRow.path ? [menuRow.path] : []) }
    function openMenu(item, row) {
        menuRow = row
        rowMenu.popup(item, 0, item.height)
    }
    function askConfirm(title, text, action) {
        confirm.heading = title
        confirm.body = text
        confirm.action = action
        confirm.open()
    }
    function addToPlaylist(paths) { pickPopup.paths = paths; pickPopup.open() }
    function applyGrid(on) { gridMode = on; player.setSetting("library/grid", on) }

    Component.onCompleted: gridMode = String(player.setting("library/grid", false)) === "true"

    Connections {
        target: player.library
        function onTabChanged() { view.clearSel() }
    }

    anchors.fill: parent
    visible: opacity > 0
    opacity: opened ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }

    Shortcut { sequence: "Esc"; enabled: view.opened && !confirm.opened && !pickPopup.opened && !nameDialog.opened; onActivated: view.close() }

    FolderDialog {
        id: folderDialog
        title: qsTr("Add library folder")
        onAccepted: player.addLibraryFolder(selectedFolder)
    }
    FileDialog {
        id: importDialog
        title: qsTr("Import playlist")
        nameFilters: [qsTr("Playlists (*.m3u *.m3u8)")]
        onAccepted: {
            const id = player.libraryPlaylists.importM3u(selectedFile)
            if (id > 0) player.library.playlistId = id
        }
    }
    FileDialog {
        id: exportDialog
        title: qsTr("Export playlist")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "m3u"
        nameFilters: [qsTr("Playlists (*.m3u *.m3u8)")]
        onAccepted: player.libraryPlaylists.exportM3u(player.library.playlistId, selectedFile)
    }

    component Pill: Button {
        id: pill
        property bool active: false
        implicitHeight: 30
        implicitWidth: Math.max(34, pillLabel.implicitWidth + 24)
        hoverEnabled: true
        background: Rectangle {
            radius: Theme.radius
            color: pill.active ? Theme.accentSoft
                 : pill.pressed ? Theme.surfacePressed
                 : pill.hovered ? Theme.surfaceHover : Theme.elevated
            border.width: pill.visualFocus || pill.active ? 1 : 0
            border.color: pill.visualFocus ? Theme.focusRing : Theme.accent
        }
        contentItem: Text {
            id: pillLabel
            text: pill.text
            color: !pill.enabled ? Theme.disabledText : pill.active ? Theme.accentHover : Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // Modal card used by the confirm / name / playlist-picker dialogs.
    component Card: Popup {
        modal: true
        focus: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(440, (parent ? parent.width : 440) - 48)
        padding: Theme.spacingLarge
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        Overlay.modal: Rectangle { color: Theme.scrimSoft }
        background: Rectangle { color: Theme.surface; border.color: Theme.border; radius: Theme.radiusLarge }
    }
    component CardTitle: Text {
        color: Theme.primaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeHeading
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    // Round selection marker (also the "multi-select" entry point).
    component Check: Rectangle {
        id: chk
        property bool marked: false
        signal toggled()
        width: 20; height: 20; radius: 10
        color: marked ? Theme.accent : Theme.scrimSoft
        border.color: marked ? Theme.accent : Theme.videoTextSecondary
        Text {
            anchors.centerIn: parent
            visible: chk.marked
            text: "✓"
            color: Theme.accentText
            font.pixelSize: 12
        }
        MouseArea { anchors.fill: parent; anchors.margins: -6; cursorShape: Qt.PointingHandCursor; onClicked: chk.toggled() }
    }

    // --- context menu for a row/tile ---
    Menu {
        id: rowMenu
        padding: 6
        background: Rectangle { implicitWidth: 230; color: Theme.surface; border.color: Theme.border; radius: Theme.radius }
        ThemedMenuItem { text: qsTr("Play"); onTriggered: { player.playLibrary(view.menuRow.index); view.close() } }
        ThemedMenuItem {
            text: view.menuRow.favorite ? qsTr("Remove from favorites") : qsTr("Add to favorites")
            onTriggered: player.library.toggleFavorite(view.menuRow.path)
        }
        ThemedMenuItem { text: qsTr("Add to playlist…"); onTriggered: view.addToPlaylist(view.rowPaths()) }
        ThemedMenuItem {
            visible: view.inPlaylists
            height: visible ? implicitHeight : 0
            text: qsTr("Remove from playlist")
            onTriggered: player.libraryPlaylists.removeItem(player.library.playlistId, view.menuRow.index)
        }
        ThemedMenuItem { text: qsTr("Open file location"); onTriggered: player.library.revealInFolder(view.menuRow.path) }
        ThemedMenuItem { text: qsTr("Remove from library"); onTriggered: player.library.removeFromLibrary(view.menuRow.path) }
        ThemedMenuItem {
            text: qsTr("Delete file from disk…")
            onTriggered: view.askConfirm(qsTr("Delete this file?"),
                qsTr("“%1” will be moved to the trash. This cannot be undone from Lumina.").arg(view.menuRow.title),
                function() { player.library.deleteFromDisk(view.menuRow.path) })
        }
    }

    Card {
        id: confirm
        objectName: "libraryConfirm"
        property string heading
        property string body
        property var action: null
        contentItem: ColumnLayout {
            spacing: Theme.spacingMedium
            CardTitle { text: confirm.heading }
            Text {
                Layout.fillWidth: true
                text: confirm.body
                wrapMode: Text.Wrap
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Pill { text: qsTr("Cancel"); onClicked: confirm.close() }
                Pill { objectName: "libraryConfirmOk"; text: qsTr("Delete"); active: true; onClicked: { confirm.close(); if (confirm.action) confirm.action() } }
            }
        }
    }

    // Create / rename a playlist.
    Card {
        id: nameDialog
        objectName: "libraryNameDialog"
        property bool renaming: false
        property var paths: []         // when creating: add these to the new playlist
        onAboutToShow: nameField.text = renaming ? playlistName() : ""
        onOpened: nameField.forceActiveFocus()
        function playlistName() { return player.libraryPlaylists.nameOf(player.library.playlistId) }
        function submit() {
            const n = nameField.text.trim()
            if (n === "") return
            if (renaming) player.libraryPlaylists.rename(player.library.playlistId, n)
            else {
                const id = player.libraryPlaylists.create(n)
                if (id > 0 && paths.length > 0) { player.libraryPlaylists.addPaths(id, paths); view.clearSel() }
                else if (id > 0) player.library.playlistId = id
            }
            close()
        }
        contentItem: ColumnLayout {
            spacing: Theme.spacingMedium
            CardTitle { text: nameDialog.renaming ? qsTr("Rename playlist") : qsTr("New playlist") }
            TextField {
                id: nameField
                objectName: "libraryNameField"
                Layout.fillWidth: true
                implicitHeight: Theme.controlHeight
                placeholderText: qsTr("Playlist name")
                placeholderTextColor: Theme.disabledText
                color: Theme.primaryText
                selectByMouse: true
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
                onAccepted: nameDialog.submit()
                background: Rectangle { radius: Theme.radius; color: Theme.background; border.color: nameField.activeFocus ? Theme.focusRing : Theme.border }
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Pill { text: qsTr("Cancel"); onClicked: nameDialog.close() }
                Pill { objectName: "libraryNameOk"; text: nameDialog.renaming ? qsTr("Rename") : qsTr("Create"); active: true; onClicked: nameDialog.submit() }
            }
        }
    }

    // Pick the playlist that receives `paths`.
    Card {
        id: pickPopup
        objectName: "libraryPickPlaylist"
        property var paths: []
        contentItem: ColumnLayout {
            spacing: Theme.spacingSmall
            CardTitle { text: qsTr("Add %n item(s) to playlist", "", pickPopup.paths.length) }
            Repeater {
                model: player.libraryPlaylists
                delegate: Pill {
                    required property int playlistId
                    required property string name
                    required property int itemCount
                    Layout.fillWidth: true
                    text: name + "  (" + itemCount + ")"
                    onClicked: { player.libraryPlaylists.addPaths(playlistId, pickPopup.paths); view.clearSel(); pickPopup.close() }
                }
            }
            Pill {
                Layout.fillWidth: true
                text: qsTr("New playlist…")
                active: true
                onClicked: { nameDialog.renaming = false; nameDialog.paths = pickPopup.paths; pickPopup.close(); nameDialog.open() }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.background
        // swallow input so nothing underneath reacts
        MouseArea { anchors.fill: parent; hoverEnabled: true }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingLarge * 2
        spacing: Theme.spacingMedium

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMedium

            Text {
                text: qsTr("Library")
                color: Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: 26
                font.weight: Font.Light
                font.letterSpacing: 2
            }
            Text {
                text: qsTr("%n item(s)", "", player.library.count)
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
                Layout.alignment: Qt.AlignBottom
                Layout.bottomMargin: 4
            }
            Item { Layout.fillWidth: true }
            Pill {
                objectName: "libraryAddFolder"
                text: qsTr("Add folder…")
                active: true
                enabled: player.libraryAvailable
                onClicked: folderDialog.open()
            }
            Pill {
                objectName: "libraryRescan"
                text: qsTr("Rescan")
                enabled: player.libraryAvailable && player.libraryFolders.length > 0 && !player.libraryScanning
                onClicked: player.rescanLibrary()
            }
            Pill {
                objectName: "libraryClose"
                text: qsTr("Close")
                onClicked: view.close()
            }
        }

        // Sections.
        Flow {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall
            Repeater {
                model: [qsTr("All"), qsTr("Videos"), qsTr("Music"), qsTr("Favorites"), qsTr("Recently added"),
                        qsTr("Recently played"), qsTr("Folders"), qsTr("Playlists"), qsTr("Duplicates")]
                delegate: Pill {
                    required property int index
                    required property string modelData
                    objectName: "libraryTab_" + index
                    text: modelData
                    active: player.library.tab === index
                    onClicked: player.library.tab = index
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            TextField {
                id: search
                objectName: "librarySearch"
                Layout.fillWidth: true
                implicitHeight: Theme.controlHeight
                placeholderText: qsTr("Search title, artist, album or folder")
                placeholderTextColor: Theme.disabledText
                color: Theme.primaryText
                selectByMouse: true
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
                leftPadding: Theme.spacingLarge
                onTextChanged: player.library.search = text
                background: Rectangle {
                    radius: Theme.radius
                    color: Theme.surface
                    border.color: search.activeFocus ? Theme.focusRing : Theme.border
                }
            }
            Pill { objectName: "libraryViewList"; text: qsTr("List"); active: !view.gridMode; onClicked: view.applyGrid(false) }
            Pill { objectName: "libraryViewGrid"; text: qsTr("Grid"); active: view.gridMode; onClicked: view.applyGrid(true) }
        }

        // Sorting and filters.
        Flow {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall
            Pill { objectName: "librarySortTitle";    text: qsTr("Title");       active: player.library.sortBy === 0 && !player.library.groupByFolder; onClicked: { player.library.groupByFolder = false; player.library.sortBy = 0 } }
            Pill { objectName: "librarySortDate";     text: qsTr("Newest");      active: player.library.sortBy === 1 && !player.library.groupByFolder; onClicked: { player.library.groupByFolder = false; player.library.sortBy = 1 } }
            Pill { objectName: "librarySortDuration"; text: qsTr("Longest");     active: player.library.sortBy === 3 && !player.library.groupByFolder; onClicked: { player.library.groupByFolder = false; player.library.sortBy = 3 } }
            Pill { objectName: "librarySortPlayed";   text: qsTr("Last played"); active: player.library.sortBy === 4 && !player.library.groupByFolder; onClicked: { player.library.groupByFolder = false; player.library.sortBy = 4 } }
            Pill { objectName: "libraryGroup";        text: qsTr("By folder");   active: player.library.groupByFolder; onClicked: player.library.groupByFolder = !player.library.groupByFolder }
            Item { width: Theme.spacingLarge; height: 1 }
            Pill {
                objectName: "libraryFilterRes"
                readonly property var steps: [0, 720, 1080, 2160]
                text: [qsTr("Any resolution"), qsTr("720p and up"), qsTr("1080p and up"), qsTr("4K")][Math.max(0, steps.indexOf(player.library.minHeight))]
                active: player.library.minHeight !== 0
                onClicked: player.library.minHeight = steps[(steps.indexOf(player.library.minHeight) + 1) % steps.length]
            }
            Pill {
                objectName: "libraryFilterDuration"
                text: [qsTr("Any length"), qsTr("Under 10 min"), qsTr("10 – 60 min"), qsTr("Over 1 hour")][player.library.durationBucket]
                active: player.library.durationBucket !== 0
                onClicked: player.library.durationBucket = (player.library.durationBucket + 1) % 4
            }
            Pill {
                objectName: "libraryFilterWatched"
                readonly property var steps: [0, 2, 1]
                text: [qsTr("Watched or not"), qsTr("Watched"), qsTr("Unwatched")][player.library.watched]
                active: player.library.watched !== 0
                onClicked: player.library.watched = steps[(steps.indexOf(player.library.watched) + 1) % steps.length]
            }
        }

        // Playlists tab: pick / create / manage a saved playlist.
        Flow {
            objectName: "libraryPlaylistBar"
            visible: view.inPlaylists
            Layout.fillWidth: true
            spacing: Theme.spacingSmall
            Repeater {
                model: player.libraryPlaylists
                delegate: Pill {
                    required property int index
                    required property int playlistId
                    required property string name
                    required property int itemCount
                    objectName: "libraryPlaylist_" + index
                    text: name + "  (" + itemCount + ")"
                    active: player.library.playlistId === playlistId
                    onClicked: player.library.playlistId = playlistId
                }
            }
            Pill { objectName: "libraryPlaylistNew"; text: qsTr("New playlist…"); active: true; onClicked: { nameDialog.renaming = false; nameDialog.paths = []; nameDialog.open() } }
            Pill { objectName: "libraryPlaylistImport"; text: qsTr("Import .m3u…"); onClicked: importDialog.open() }
            Item { width: Theme.spacingLarge; height: 1 }
            Pill {
                text: qsTr("Play"); enabled: player.library.playlistId > 0 && player.library.count > 0
                onClicked: { player.playLibrary(0); view.close() }
            }
            Pill { text: qsTr("Rename…"); enabled: player.library.playlistId > 0; onClicked: { nameDialog.renaming = true; nameDialog.open() } }
            Pill { text: qsTr("Export .m3u…"); enabled: player.library.playlistId > 0; onClicked: exportDialog.open() }
            Pill {
                objectName: "libraryPlaylistDelete"
                text: qsTr("Delete playlist"); enabled: player.library.playlistId > 0
                onClicked: view.askConfirm(qsTr("Delete this playlist?"),
                    qsTr("Only the playlist is removed; your media files are not touched."),
                    function() { const id = player.library.playlistId; player.library.playlistId = 0; player.libraryPlaylists.remove(id) })
            }
        }

        // Duplicates tab explanation.
        Text {
            visible: view.inDuplicates
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("Files with the same size, length and content fingerprint are grouped together. “Remove from library” never touches the file; deleting asks first and moves it to the trash.")
            color: Theme.secondaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }

        // Selection actions.
        RowLayout {
            objectName: "librarySelectionBar"
            visible: view.selCount > 0
            Layout.fillWidth: true
            spacing: Theme.spacingSmall
            Text {
                text: qsTr("%n selected", "", view.selCount)
                color: Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
            }
            Pill { objectName: "libraryAddSelected"; text: qsTr("Add to playlist…"); active: true; onClicked: view.addToPlaylist(Object.keys(view.selected)) }
            Pill { text: qsTr("Play selected"); onClicked: { player.openFiles(player.library.urlsOf(Object.keys(view.selected))); view.close() } }
            Pill { text: qsTr("Clear selection"); onClicked: view.clearSel() }
            Item { Layout.fillWidth: true }
        }

        // Scan progress (indeterminate bar + count) while the worker runs.
        ColumnLayout {
            objectName: "libraryProgress"
            visible: player.libraryScanning
            Layout.fillWidth: true
            spacing: 4
            Rectangle {
                Layout.fillWidth: true
                height: 3
                radius: 1.5
                color: Theme.trackIdle
                clip: true
                Rectangle {
                    id: sweep
                    width: parent.width * 0.25
                    height: parent.height
                    radius: 1.5
                    color: Theme.accent
                    SequentialAnimation on x {
                        running: player.libraryScanning && view.visible
                        loops: Animation.Infinite
                        NumberAnimation { from: -sweep.width; to: sweep.parent.width; duration: 1100; easing.type: Easing.InOutQuad }
                    }
                }
            }
            Text {
                text: qsTr("Scanning… %n file(s)", "", player.libraryProgress)
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
            }
        }
        Text {
            objectName: "libraryProbing"
            visible: player.libraryProbing && !player.libraryScanning
            text: qsTr("Reading media details in the background…")
            color: Theme.disabledText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }

        // Library folders (click x to remove from the library; files on disk are untouched).
        Flow {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall
            visible: player.libraryFolders.length > 0
            Repeater {
                model: player.libraryFolders
                delegate: Rectangle {
                    required property string modelData
                    required property int index
                    height: 26
                    width: chipText.implicitWidth + 36
                    radius: 13
                    color: Theme.surface
                    border.color: Theme.border
                    Text {
                        id: chipText
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData
                        width: Math.min(implicitWidth, 360)
                        elide: Text.ElideMiddle
                        color: Theme.secondaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    Text {
                        objectName: "libraryFolderRemove_" + index
                        anchors.right: parent.right
                        anchors.rightMargin: 9
                        anchors.verticalCenter: parent.verticalCenter
                        text: "×"
                        color: removeArea.containsMouse ? Theme.error : Theme.disabledText
                        font.pixelSize: Theme.fontSizeLarge
                        MouseArea {
                            id: removeArea
                            anchors.fill: parent
                            anchors.margins: -6
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: player.removeLibraryFolder(modelData)
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                objectName: "libraryList"
                anchors.fill: parent
                visible: !view.gridMode
                clip: true
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds
                model: player.library
                ScrollBar.vertical: ScrollBar { }
                section.property: view.inDuplicates ? "size" : player.library.grouped ? "folder" : ""
                section.criteria: ViewSection.FullString
                section.delegate: Text {
                    required property string section
                    width: list.width
                    height: 34
                    verticalAlignment: Text.AlignBottom
                    bottomPadding: 4
                    text: view.inDuplicates ? qsTr("Duplicates · %1 each").arg(view.fmtSize(section)) : section
                    elide: Text.ElideMiddle
                    color: Theme.disabledText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    font.letterSpacing: 1
                }

                delegate: Rectangle {
                    id: row
                    required property int index
                    required property string path
                    required property string title
                    required property string folder
                    required property string folderName
                    required property real addedAt
                    required property real duration
                    required property real size
                    required property int kind
                    required property int videoWidth
                    required property int videoHeight
                    required property string vcodec
                    required property string artist
                    required property string album
                    required property int year
                    required property bool favorite
                    readonly property bool isSel: view.selected[path] === true
                    objectName: "libraryRow_" + index
                    width: list.width
                    height: 56
                    radius: Theme.radius
                    color: isSel ? Theme.accentSoft : rowMouse.containsMouse ? Theme.elevated : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.animationFast } }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: function(m) {
                            if (m.button === Qt.RightButton) view.openMenu(row, row)
                            else if (m.modifiers & (Qt.ControlModifier | Qt.MetaModifier)) view.toggleSel(row.path)
                            else { player.playLibrary(row.index); view.close() }
                        }
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingLarge
                        anchors.rightMargin: Theme.spacingLarge
                        spacing: Theme.spacingLarge
                        Check {
                            objectName: "libraryCheck_" + row.index
                            marked: row.isSel
                            opacity: row.isSel || rowMouse.containsMouse || view.selCount > 0 ? 1 : 0.35
                            onToggled: view.toggleSel(row.path)
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                Layout.fillWidth: true
                                text: row.title
                                elide: Text.ElideRight
                                color: Theme.primaryText
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeLarge
                            }
                            Text {
                                Layout.fillWidth: true
                                text: view.detail(row.kind, row.videoHeight, row.vcodec, row.duration, row.size, row.artist, row.album, row.year)
                                elide: Text.ElideRight
                                color: Theme.secondaryText
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeSmall
                            }
                        }
                        Text {
                            Layout.preferredWidth: 160
                            text: view.inDuplicates ? row.folder : row.folderName
                            elide: view.inDuplicates ? Text.ElideMiddle : Text.ElideRight
                            color: Theme.secondaryText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeMedium
                        }
                        Text {
                            Layout.preferredWidth: 90
                            horizontalAlignment: Text.AlignRight
                            text: view.relTime(row.addedAt)
                            color: Theme.disabledText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeSmall
                        }
                        Text {
                            text: row.favorite ? "★" : "☆"
                            color: row.favorite ? Theme.accentHover : Theme.disabledText
                            font.pixelSize: Theme.fontSizeLarge
                            MouseArea { anchors.fill: parent; anchors.margins: -6; cursorShape: Qt.PointingHandCursor; onClicked: player.library.toggleFavorite(row.path) }
                        }
                        Text {
                            objectName: "libraryMore_" + row.index
                            text: "⋯"
                            color: Theme.secondaryText
                            font.pixelSize: Theme.fontSizeHeading
                            MouseArea { anchors.fill: parent; anchors.margins: -6; cursorShape: Qt.PointingHandCursor; onClicked: view.openMenu(row, row) }
                        }
                    }
                }
            }

            GridView {
                id: grid
                objectName: "libraryGrid"
                anchors.fill: parent
                visible: view.gridMode
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: player.library
                readonly property int cols: Math.max(1, Math.floor(width / 210))
                cellWidth: Math.floor(width / cols)
                cellHeight: 176
                ScrollBar.vertical: ScrollBar { }

                delegate: Item {
                    id: tile
                    required property int index
                    required property string path
                    required property string title
                    required property string folderName
                    required property real duration
                    required property real size
                    required property int kind
                    required property int videoHeight
                    required property string vcodec
                    required property string artist
                    required property string album
                    required property int year
                    required property bool favorite
                    required property bool hasCover
                    required property string thumb
                    readonly property bool isSel: view.selected[path] === true
                    objectName: "libraryTile_" + index
                    width: grid.cellWidth
                    height: grid.cellHeight

                    MouseArea {
                        id: tileMouse
                        anchors.fill: parent
                        anchors.margins: 4
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: function(m) {
                            if (m.button === Qt.RightButton) view.openMenu(tile, tile)
                            else if (m.modifiers & (Qt.ControlModifier | Qt.MetaModifier)) view.toggleSel(tile.path)
                            else { player.playLibrary(tile.index); view.close() }
                        }
                    }
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 4
                        radius: Theme.radius
                        color: tile.isSel ? Theme.accentSoft : tileMouse.containsMouse ? Theme.elevated : "transparent"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 6
                            Rectangle {
                                id: poster
                                Layout.fillWidth: true
                                Layout.preferredHeight: width * 9 / 16
                                radius: Theme.radiusSmall
                                color: Theme.surface
                                clip: true
                                Text { // placeholder while the frame is pending (or missing)
                                    anchors.centerIn: parent
                                    text: tile.kind === 1 ? "♪" : "▶"
                                    color: Theme.disabledText
                                    font.pixelSize: 28
                                }
                                Image {
                                    anchors.fill: parent
                                    asynchronous: true
                                    fillMode: Image.PreserveAspectCrop
                                    // lazily requested: only tiles that exist (visible) ask the provider
                                    source: tile.kind === 0 || tile.hasCover ? tile.thumb : ""
                                    opacity: status === Image.Ready ? 1 : 0
                                    Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }
                                }
                                Rectangle { // length badge
                                    visible: tile.duration > 0
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    anchors.margins: 5
                                    radius: 3
                                    color: Theme.scrimSoft
                                    width: durText.implicitWidth + 10
                                    height: durText.implicitHeight + 4
                                    Text {
                                        id: durText
                                        anchors.centerIn: parent
                                        text: view.fmtDur(tile.duration)
                                        color: Theme.videoText
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fontSizeSmall
                                    }
                                }
                                Check {
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.margins: 6
                                    marked: tile.isSel
                                    visible: tile.isSel || tileMouse.containsMouse || view.selCount > 0
                                    onToggled: view.toggleSel(tile.path)
                                }
                                Text {
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 6
                                    text: tile.favorite ? "★" : (tileMouse.containsMouse ? "☆" : "")
                                    color: tile.favorite ? Theme.accentHover : Theme.videoText
                                    font.pixelSize: Theme.fontSizeLarge
                                    MouseArea { anchors.fill: parent; anchors.margins: -6; cursorShape: Qt.PointingHandCursor; onClicked: player.library.toggleFavorite(tile.path) }
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: tile.title
                                elide: Text.ElideRight
                                color: Theme.primaryText
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeMedium
                            }
                            Text {
                                Layout.fillWidth: true
                                text: view.detail(tile.kind, tile.videoHeight, tile.vcodec, 0, tile.size, tile.artist, tile.album, tile.year)
                                elide: Text.ElideRight
                                color: Theme.secondaryText
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeSmall
                            }
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                width: parent.width * 0.7
                visible: player.library.count === 0
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeLarge
                text: !player.libraryAvailable ? qsTr("The library is unavailable. Playback still works.")
                    : view.inPlaylists && player.library.playlistId === 0 ? qsTr("Create a playlist, or pick one above.")
                    : view.inPlaylists ? qsTr("This playlist is empty. Select files in the library and use “Add to playlist…”.")
                    : view.inDuplicates && !player.libraryProbing ? qsTr("No duplicates found.")
                    : player.libraryFolders.length === 0 ? qsTr("Add a folder to build your library.")
                    : player.libraryScanning ? qsTr("Scanning…")
                    : qsTr("No matching media.")
            }
        }
    }
}
