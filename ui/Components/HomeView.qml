import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Start screen (no media loaded): primary actions, continue-watching, favourites, recent files.
// Talks to the controller only; the host wires the three request signals.
Rectangle {
    id: home

    signal openFileRequested()
    signal openUrlRequested()
    signal libraryRequested()
    signal windowMoveRequested()

    readonly property bool hasHistory: player.historyAvailable ? player.history.count > 0 : player.recentFiles.length > 0
    readonly property int contentWidth: Math.min(920, width - 64)

    gradient: Gradient {
        GradientStop { position: 0.0; color: Theme.background }
        GradientStop { position: 1.0; color: Theme.surface }
    }

    function relTime(sec) {
        const d = Math.max(0, Date.now() / 1000 - sec)
        if (d < 90) return qsTr("just now")
        if (d < 3600) return qsTr("%1 min ago").arg(Math.round(d / 60))
        if (d < 86400) return qsTr("%1 h ago").arg(Math.round(d / 3600))
        if (d < 86400 * 30) return qsTr("%1 d ago").arg(Math.round(d / 86400))
        return qsTr("long ago")
    }
    function ext(path) {
        const m = /\.([A-Za-z0-9]{1,5})$/.exec(path)
        return m ? m[1].toUpperCase() : ""
    }
    function hue(title) {
        let h = 0
        for (let i = 0; i < title.length; ++i) h = (h * 31 + title.charCodeAt(i)) % 360
        return h / 360
    }

    MouseArea { // drag-to-move for borderless mode
        anchors.fill: parent
        onPressed: home.windowMoveRequested()
    }

    // ---- Small building blocks ---------------------------------------------------------------
    component HomeButton: Button {
        id: hb
        property bool primary: false
        property string glyph: ""
        implicitHeight: 38
        implicitWidth: hbRow.implicitWidth + 32
        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        Accessible.name: hb.text
        background: Rectangle {
            radius: height / 2
            color: hb.primary ? (hb.pressed ? Theme.accentPressed : hb.hovered ? Theme.accentHover : Theme.accent)
                              : (hb.pressed ? Theme.surfacePressed : hb.hovered ? Theme.surfaceHover : "transparent")
            border.width: hb.primary ? 0 : 1
            border.color: hb.visualFocus ? Theme.focusRing : Theme.border
            Behavior on color { ColorAnimation { duration: Theme.animationFast } }
        }
        contentItem: Item {
            implicitWidth: hbRow.implicitWidth
            Row {
                id: hbRow
                anchors.centerIn: parent
                spacing: 8
                Glyph { name: hb.glyph; size: 16; anchors.verticalCenter: parent.verticalCenter; tint: hb.primary ? Theme.accentText : Theme.primaryText; filled: hb.glyph === "play" }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: hb.text
                    color: hb.primary ? Theme.accentText : Theme.primaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeMedium
                    font.weight: Font.Medium
                }
            }
        }
    }

    component StarButton: Item {
        id: sb
        property bool on: false
        property string objName: ""
        signal toggled()
        objectName: objName
        width: 32; height: 32
        Accessible.role: Accessible.Button
        Accessible.name: on ? qsTr("Remove from favorites") : qsTr("Add to favorites")
        Glyph {
            anchors.centerIn: parent
            name: "star"
            size: 16
            filled: sb.on
            tint: sb.on ? Theme.accentHover : Theme.secondaryText
            opacity: sb.on || starMa.containsMouse ? 1 : 0.7
        }
        MouseArea { id: starMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: sb.toggled() }
    }

    component SectionTitle: Text {
        Layout.topMargin: 36
        Layout.bottomMargin: 4
        color: Theme.primaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeLarge
        font.weight: Font.DemiBold
    }

    // ---- Cards -------------------------------------------------------------------------------
    Component {
        id: historyCard
        Item {
            id: card
            required property int index
            required property string path
            required property string title
            required property real progress
            required property real lastPlayed
            required property bool favorite
            objectName: "continueCard_" + index
            width: 232
            height: 156
            Accessible.role: Accessible.Button
            Accessible.name: card.title

            Rectangle {
                anchors.fill: parent
                radius: Theme.radiusLarge
                color: cardMouse.containsMouse ? Theme.elevated : Theme.surface
                border.color: cardMouse.containsMouse ? Theme.accent : Theme.border
                Behavior on color { ColorAnimation { duration: Theme.animationFast } }
                Behavior on border.color { ColorAnimation { duration: Theme.animationFast } }

                // Generated poster: a calm gradient seeded by the title, so each file looks distinct.
                Rectangle {
                    id: art
                    x: 1; y: 1
                    width: parent.width - 2
                    height: 92
                    radius: Theme.radiusLarge - 1
                    readonly property color colA: Qt.hsla(home.hue(card.title), 0.42, Theme.isLight ? 0.62 : 0.30, 1)
                    readonly property color colB: Qt.hsla((home.hue(card.title) + 0.08) % 1, 0.5, Theme.isLight ? 0.50 : 0.17, 1)
                    gradient: Gradient {
                        GradientStop { position: 0; color: art.colA }
                        GradientStop { position: 1; color: art.colB }
                    }
                    Rectangle { // square off the bottom corners
                        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                        height: parent.radius
                        color: art.colB
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: 40; height: 40; radius: 20
                        color: Qt.rgba(0, 0, 0, 0.45)
                        opacity: cardMouse.containsMouse ? 1 : 0.0
                        scale: cardMouse.containsMouse ? 1 : 0.85
                        Behavior on opacity { NumberAnimation { duration: Theme.animationFast } }
                        Behavior on scale { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }
                        Glyph { anchors.centerIn: parent; anchors.horizontalCenterOffset: 1; name: "play"; filled: true; size: 18; tint: "#FFFFFF" }
                    }
                    Rectangle {
                        visible: home.ext(card.path) !== ""
                        x: 10; y: 10
                        width: extLabel.implicitWidth + 12; height: 20; radius: 5
                        color: Qt.rgba(0, 0, 0, 0.42)
                        Text {
                            id: extLabel
                            anchors.centerIn: parent
                            text: home.ext(card.path)
                            color: "#F4F5F7"
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.6
                        }
                    }
                    Rectangle { // progress along the bottom edge of the art
                        visible: card.progress > 0
                        anchors { left: parent.left; bottom: parent.bottom }
                        height: 3
                        width: parent.width
                        color: Qt.rgba(1, 1, 1, 0.22)
                        Rectangle { width: parent.width * card.progress; height: parent.height; color: Theme.accentHover }
                    }
                }
                Text {
                    anchors { left: parent.left; right: parent.right; top: art.bottom; leftMargin: 12; rightMargin: 12; topMargin: 10 }
                    text: card.title
                    elide: Text.ElideRight
                    color: Theme.primaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeMedium
                    font.weight: Font.Medium
                }
                Text {
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: 12; rightMargin: 12; bottomMargin: 12 }
                    text: card.progress > 0.01 ? qsTr("%1  ·  %2% watched").arg(home.relTime(card.lastPlayed)).arg(Math.round(card.progress * 100))
                                               : home.relTime(card.lastPlayed)
                    elide: Text.ElideRight
                    color: Theme.secondaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                }
                MouseArea {
                    id: cardMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: Qt.PointingHandCursor
                    onClicked: (m) => m.button === Qt.RightButton ? player.toggleFavorite(card.path) : player.openHistory(card.path)
                }
                StarButton {
                    objName: "cardStar_" + card.index
                    anchors { right: art.right; top: art.top; margins: 4 }
                    on: card.favorite
                    onToggled: player.toggleFavorite(card.path)
                }
            }
        }
    }

    // ---- Page --------------------------------------------------------------------------------
    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: page.implicitHeight + 64
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: page
            x: Math.round((flick.width - width) / 2)
            y: 24
            width: home.contentWidth
            spacing: 0

            // Top bar: wordmark left, actions right.
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMedium

                Rectangle {
                    width: 30; height: 30; radius: 9
                    color: Theme.accent
                    Glyph { anchors.centerIn: parent; anchors.horizontalCenterOffset: 1; name: "play"; filled: true; size: 14; tint: Theme.accentText }
                }
                Text {
                    text: qsTr("Lumina")
                    color: Theme.primaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeHeading + 2
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.4
                    Layout.fillWidth: true
                }
                HomeButton { objectName: "homeLibraryButton"; text: qsTr("Library"); glyph: "library"; onClicked: home.libraryRequested() }
                HomeButton { objectName: "homeUrlButton"; text: qsTr("Open URL…"); glyph: "link"; onClicked: home.openUrlRequested() }
                HomeButton { objectName: "homeOpenButton"; text: qsTr("Open File"); glyph: "folder"; primary: true; onClicked: home.openFileRequested() }
            }

            // Empty state: one large, obvious drop target.
            Rectangle {
                visible: !home.hasHistory
                Layout.fillWidth: true
                Layout.topMargin: 56
                Layout.preferredHeight: 300
                radius: Theme.radiusLarge + 4
                color: dropMa.containsMouse ? Theme.elevated : Theme.surface
                border.color: dropMa.containsMouse ? Theme.accent : Theme.border
                Behavior on color { ColorAnimation { duration: Theme.animationFast } }
                MouseArea { id: dropMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: home.openFileRequested() }
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 10
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 64; height: 64; radius: 32
                        color: Theme.accentSoft
                        Glyph { anchors.centerIn: parent; anchors.horizontalCenterOffset: 2; name: "play"; filled: true; size: 26; tint: Theme.accentHover }
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 6
                        text: qsTr("Drop a video here")
                        color: Theme.primaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeHeading + 4
                        font.weight: Font.DemiBold
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("or click to choose a file. MKV, MP4, AVI, WebM and most other formats play.")
                        color: Theme.secondaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeMedium
                    }
                }
            }

            SectionTitle {
                objectName: "continueTitle"
                visible: player.continueWatching.count > 0
                text: qsTr("Continue watching")
            }
            ListView {
                objectName: "continueList"
                visible: player.continueWatching.count > 0
                Layout.fillWidth: true
                Layout.preferredHeight: 160
                orientation: ListView.Horizontal
                spacing: Theme.spacingMedium
                clip: true
                model: player.continueWatching
                delegate: historyCard
            }

            SectionTitle { visible: player.favorites.count > 0; text: qsTr("Favorites") }
            ListView {
                objectName: "favoritesList"
                visible: player.favorites.count > 0
                Layout.fillWidth: true
                Layout.preferredHeight: 160
                orientation: ListView.Horizontal
                spacing: Theme.spacingMedium
                clip: true
                model: player.favorites
                delegate: historyCard
            }

            SectionTitle { visible: home.hasHistory; text: qsTr("Recent") }

            Repeater { // DB-backed recent list
                model: player.history
                delegate: Rectangle {
                    id: row
                    objectName: "recentRow_" + index
                    required property int index
                    required property string path
                    required property string title
                    required property real lastPlayed
                    required property bool favorite
                    visible: index < 6
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? 52 : 0
                    radius: Theme.radius + 2
                    color: rowMouse.containsMouse ? Theme.surfaceHover : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.animationFast } }
                    Accessible.role: Accessible.Button
                    Accessible.name: row.title

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        onClicked: (m) => m.button === Qt.RightButton ? player.toggleFavorite(row.path) : player.openHistory(row.path)
                    }
                    RowLayout {
                        anchors { fill: parent; leftMargin: 10; rightMargin: 6 }
                        spacing: 14
                        Rectangle {
                            width: 32; height: 32; radius: 8
                            color: Qt.hsla(home.hue(row.title), 0.4, Theme.isLight ? 0.62 : 0.28, 1)
                            Glyph { anchors.centerIn: parent; anchors.horizontalCenterOffset: 1; name: "play"; filled: true; size: 13; tint: "#F4F5F7" }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: row.title
                            elide: Text.ElideMiddle
                            color: Theme.primaryText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeMedium
                            font.weight: Font.Medium
                        }
                        Text {
                            visible: home.ext(row.path) !== ""
                            text: home.ext(row.path)
                            color: Theme.disabledText
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.6
                        }
                        Text {
                            Layout.preferredWidth: 96
                            horizontalAlignment: Text.AlignRight
                            text: home.relTime(row.lastPlayed)
                            color: Theme.secondaryText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeSmall
                        }
                        StarButton {
                            objName: "recentStar_" + row.index
                            on: row.favorite
                            onToggled: player.toggleFavorite(row.path)
                        }
                    }
                    Rectangle { // hairline between rows
                        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: 10; rightMargin: 10 }
                        height: 1
                        color: Theme.border
                        opacity: rowMouse.containsMouse ? 0 : 0.5
                    }
                }
            }

            Repeater { // QSettings fallback when the library DB is unavailable
                model: player.historyAvailable ? [] : player.recentFiles.slice(0, 6)
                delegate: Rectangle {
                    id: legacyRow
                    objectName: "recentRow_" + index
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true
                    implicitHeight: 48
                    radius: Theme.radius + 2
                    color: legacyMouse.containsMouse ? Theme.surfaceHover : "transparent"
                    Text {
                        anchors { fill: parent; leftMargin: 14; rightMargin: 14 }
                        verticalAlignment: Text.AlignVCenter
                        text: legacyRow.modelData.split("/").pop()
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

            Text {
                visible: home.hasHistory
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 40
                text: qsTr("Drop a file anywhere to play it")
                color: Theme.disabledText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
            }
        }
    }
}
