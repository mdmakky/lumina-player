import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Keyboard shortcut list with click-to-rebind, conflict prompt and per-action reset.
// Lives inside a SettingSection card (so it also exposes `matched` for global search).
ColumnLayout {
    id: scPage

    property bool dialogOpen: false

    property string capId: ""      // action being rebound
    property string pendingSeq: "" // candidate waiting on a conflict decision
    property string pendingWith: ""
    property string query: ""
    readonly property bool conflicted: pendingWith !== ""
    readonly property bool matched: SettingsState.q === ""
        || "shortcuts keyboard keys hotkeys keybindings rebind".indexOf(SettingsState.q) >= 0

    // Flat model: category header rows + matching action rows.
    readonly property var rows: {
        const q = query.trim().toLowerCase()
        const out = []
        let cat = ""
        for (const a of shortcuts.actions) {
            if (q && (a.name + " " + a.category + " " + a.key).toLowerCase().indexOf(q) < 0)
                continue
            if (a.category !== cat) { cat = a.category; out.push({ header: true, name: cat }) }
            out.push(a)
        }
        return out
    }

    function endCapture() { capId = ""; pendingSeq = ""; pendingWith = ""; shortcuts.capturing = false }
    function beginCapture(id) {
        capId = id; pendingSeq = ""; pendingWith = ""
        shortcuts.capturing = true
        captureKeys.forceActiveFocus()
    }
    function commit(swap) {
        if (swap) shortcuts.swap(capId, pendingSeq)
        else shortcuts.setSequence(capId, pendingSeq)
        endCapture()
    }
    onDialogOpenChanged: if (!dialogOpen) endCapture()
    Connections {
        target: SettingsState
        function onPageChanged() { scPage.endCapture() }
    }

    Layout.fillWidth: true
    Layout.margins: Theme.spacingLarge
    spacing: Theme.spacingMedium
    visible: matched

    TextField {
        id: scSearch
        objectName: "shortcutSearch"
        Layout.fillWidth: true
        implicitHeight: 34
        placeholderText: qsTr("Filter actions or keys")
        color: Theme.primaryText
        placeholderTextColor: Theme.secondaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMedium
        leftPadding: 12
        Accessible.name: qsTr("Filter shortcut actions")
        background: Rectangle {
            radius: Theme.radius
            color: Theme.elevated
            border.width: scSearch.activeFocus ? 2 : 1
            border.color: scSearch.activeFocus ? Theme.focusRing : Theme.border
        }
        onTextChanged: scPage.query = text
        onActiveFocusChanged: if (activeFocus) scPage.endCapture()
    }

    ColumnLayout {
        id: list
        objectName: "shortcutList"
        Layout.fillWidth: true
        spacing: 0
        Repeater {
            model: scPage.rows
            delegate: Item {
                id: row
                required property var modelData
                readonly property bool isHeader: modelData.header === true
                readonly property bool capturing: !isHeader && scPage.capId === modelData.id
                objectName: isHeader ? "" : "shortcutRow_" + modelData.id
                Layout.fillWidth: true
                Layout.preferredHeight: isHeader ? 32 : 36

                Text {
                    visible: row.isHeader
                    anchors.left: parent.left
                    anchors.leftMargin: 2
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 4
                    text: row.isHeader ? row.modelData.name.toUpperCase() : ""
                    color: Theme.secondaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.6
                }
                Rectangle {
                    visible: !row.isHeader
                    anchors.fill: parent
                    radius: Theme.radius
                    color: row.capturing ? Theme.accentSoft : rowMouse.containsMouse ? Theme.surfaceHover : "transparent"
                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: scPage.beginCapture(row.modelData.id)
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingSmall
                        spacing: Theme.spacingMedium
                        Text {
                            Layout.fillWidth: true
                            text: row.isHeader ? "" : row.modelData.name
                            elide: Text.ElideRight
                            color: Theme.primaryText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeMedium
                        }
                        SettingsButton {
                            objectName: row.isHeader ? "" : "shortcutReset_" + row.modelData.id
                            visible: !row.isHeader && row.modelData.key !== row.modelData.defaultKey
                            implicitHeight: 26
                            text: qsTr("Reset")
                            focusPolicy: Qt.NoFocus
                            onClicked: { scPage.endCapture(); shortcuts.reset(row.modelData.id) }
                        }
                        Rectangle {
                            Layout.preferredWidth: Math.max(80, keyText.implicitWidth + 22)
                            Layout.preferredHeight: 26
                            radius: Theme.radius
                            color: Theme.elevated
                            border.width: 1
                            border.color: row.capturing ? Theme.accent : Theme.border
                            Text {
                                id: keyText
                                anchors.centerIn: parent
                                text: row.isHeader ? ""
                                    : row.capturing ? qsTr("Press keys…")
                                    : row.modelData.key === "" ? qsTr("Unbound") : row.modelData.key
                                color: row.capturing ? Theme.accentHover
                                     : row.modelData.key === "" ? Theme.secondaryText : Theme.primaryText
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeSmall
                            }
                        }
                    }
                }
            }
        }
    }

    // Capture / conflict status line.
    RowLayout {
        Layout.fillWidth: true
        visible: scPage.capId !== ""
        spacing: Theme.spacingMedium
        Text {
            objectName: "shortcutHint"
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: scPage.conflicted ? Theme.error : Theme.secondaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
            text: scPage.conflicted
                ? qsTr("%1 is already used by “%2”.").arg(scPage.pendingSeq).arg(shortcuts.nameOf(scPage.pendingWith))
                : qsTr("Press a key combination. Esc cancels, Backspace clears.")
        }
        SettingsButton {
            objectName: "shortcutSwap"
            visible: scPage.conflicted
            text: qsTr("Swap")
            focusPolicy: Qt.NoFocus
            onClicked: scPage.commit(true)
        }
        SettingsButton {
            objectName: "shortcutConflictCancel"
            visible: scPage.conflicted
            text: qsTr("Cancel")
            focusPolicy: Qt.NoFocus
            onClicked: scPage.endCapture()
        }
    }

    // Invisible key sink while rebinding.
    Item {
        id: captureKeys
        focus: false
        Keys.onShortcutOverride: (event) => event.accepted = scPage.capId !== ""
        Keys.onPressed: (event) => {
            if (scPage.capId === "") { event.accepted = false; return }
            event.accepted = true
            if (event.key === Qt.Key_Escape) { scPage.endCapture(); return }
            if (scPage.conflicted) return
            if (event.key === Qt.Key_Backspace && event.modifiers === Qt.NoModifier) {
                shortcuts.setSequence(scPage.capId, "")
                scPage.endCapture()
                return
            }
            const seq = shortcuts.fromKeyEvent(event.key, event.modifiers)
            if (seq === "") return
            const other = shortcuts.conflict(scPage.capId, seq)
            if (other !== "") { scPage.pendingSeq = seq; scPage.pendingWith = other }
            else { shortcuts.setSequence(scPage.capId, seq); scPage.endCapture() }
        }
    }
}
