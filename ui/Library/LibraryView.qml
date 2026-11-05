import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Lumina.Player

// Optional media library: full-window overlay listing indexed files. Talks to
// the controller only (player.library model, player.addLibraryFolder, ...).
Item {
    id: view

    property bool opened: false
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

    anchors.fill: parent
    visible: opacity > 0
    opacity: opened ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }

    Shortcut { sequence: "Esc"; enabled: view.opened; onActivated: view.close() }

    FolderDialog {
        id: folderDialog
        title: qsTr("Add library folder")
        onAccepted: player.addLibraryFolder(selectedFolder)
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
            color: pill.active ? Theme.accentHover : Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
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

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            TextField {
                id: search
                objectName: "librarySearch"
                Layout.fillWidth: true
                implicitHeight: Theme.controlHeight
                placeholderText: qsTr("Search title or folder")
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
            Pill { objectName: "librarySortTitle"; text: qsTr("Title");  active: player.library.sortBy === 0 && !player.library.groupByFolder; onClicked: { player.library.groupByFolder = false; player.library.sortBy = 0 } }
            Pill { objectName: "librarySortDate";  text: qsTr("Newest"); active: player.library.sortBy === 1 && !player.library.groupByFolder; onClicked: { player.library.groupByFolder = false; player.library.sortBy = 1 } }
            Pill { objectName: "libraryGroup";     text: qsTr("By folder"); active: player.library.groupByFolder; onClicked: player.library.groupByFolder = !player.library.groupByFolder }
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

        ListView {
            id: list
            objectName: "libraryList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2
            boundsBehavior: Flickable.StopAtBounds
            model: player.library
            ScrollBar.vertical: ScrollBar { }
            section.property: player.library.groupByFolder ? "folder" : ""
            section.criteria: ViewSection.FullString
            section.delegate: Text {
                required property string section
                width: list.width
                height: 34
                verticalAlignment: Text.AlignBottom
                bottomPadding: 4
                text: section
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
                required property string folderName
                required property real addedAt
                objectName: "libraryRow_" + index
                width: list.width
                height: 48
                radius: Theme.radius
                color: rowMouse.containsMouse ? Theme.elevated : "transparent"
                Behavior on color { ColorAnimation { duration: Theme.animationFast } }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { player.playLibrary(row.index); view.close() }
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingLarge
                    anchors.rightMargin: Theme.spacingLarge
                    spacing: Theme.spacingLarge
                    Text {
                        Layout.fillWidth: true
                        text: row.title
                        elide: Text.ElideRight
                        color: Theme.primaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeLarge
                    }
                    Text {
                        Layout.preferredWidth: 160
                        text: row.folderName
                        elide: Text.ElideRight
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
                }
            }

            Text {
                anchors.centerIn: parent
                visible: list.count === 0
                horizontalAlignment: Text.AlignHCenter
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeLarge
                text: !player.libraryAvailable ? qsTr("The library is unavailable. Playback still works.")
                    : player.libraryFolders.length === 0 ? qsTr("Add a folder to build your library.")
                    : player.libraryScanning ? qsTr("Scanning…")
                    : qsTr("No matching media.")
            }
        }
    }
}
