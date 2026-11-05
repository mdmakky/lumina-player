import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Shared chrome for every popup panel: rounded card, hairline border, soft layered
// shadow, fade/scale entrance. Loose children become scrollable, keyboard-navigable
// rows (Up/Down move focus, Esc / click outside close, never leaves the window).
// Popups with their own `contentItem` (equalizer, bookmarks...) just inherit the chrome.
Popup {
    id: popup

    property int panelWidth: 280
    default property alias content: col.data
    readonly property alias column: col

    // Opens above `item` (right-aligned to it), clamped into the window by `margins`.
    function openAbove(item) {
        const p = item.mapToItem(popup.parent, 0, 0)
        x = Math.round(p.x + item.width - width)
        y = Math.round(p.y - height - 8)
        open()
    }

    focus: true
    margins: 8
    padding: 8
    width: panelWidth > 0 ? panelWidth : implicitWidth
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    transformOrigin: Popup.Bottom

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.animationFast; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: Theme.animationFast; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: Theme.animationFast }
    }

    background: Item {
        // Cheap soft shadow: a few translucent rounded rects growing outward.
        Repeater {
            model: 4
            Rectangle {
                required property int index
                anchors.fill: parent
                anchors.margins: -(index + 1) * 3
                anchors.topMargin: -(index + 1) * 3 + 3
                anchors.bottomMargin: -(index + 1) * 3 - 3
                radius: Theme.radiusLarge + (index + 1) * 3
                color: Theme.shadow
                opacity: 0.2 - index * 0.045
            }
        }
        Rectangle {
            anchors.fill: parent
            radius: Theme.radiusLarge
            color: Theme.surface
            border.color: Theme.border
            border.width: 1
        }
    }

    // Keyboard-focusable rows/buttons inside the panel, in visual order.
    function focusables(item, out) {
        for (let i = 0; i < item.children.length; ++i) {
            const c = item.children[i]
            if (!c.visible || !c.enabled) continue
            if (c.activeFocusOnTab) out.push(c)
            else focusables(c, out)
        }
        return out
    }
    function moveFocus(forward) {
        const list = focusables(col, [])
        if (list.length === 0) return
        const i = list.indexOf(flick.Window.activeFocusItem)
        const n = i < 0 ? (forward ? 0 : list.length - 1) : (i + (forward ? 1 : list.length - 1)) % list.length
        list[n].forceActiveFocus(Qt.TabFocusReason)
    }
    onOpened: {
        const list = focusables(col, [])
        if (list.length > 0) list[0].forceActiveFocus(Qt.OtherFocusReason)
    }

    contentItem: Flickable {
        id: flick
        clip: true
        implicitWidth: popup.panelWidth - popup.leftPadding - popup.rightPadding
        implicitHeight: Math.min(col.implicitHeight,
                                 (Overlay.overlay ? Overlay.overlay.height : 600) - 2 * popup.margins
                                 - popup.topPadding - popup.bottomPadding)
        contentWidth: width
        contentHeight: col.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {
            policy: flick.contentHeight > flick.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
            contentItem: Rectangle { implicitWidth: 4; radius: 2; color: Theme.secondaryText; opacity: 0.6 }
        }
        Keys.onUpPressed: popup.moveFocus(false)
        Keys.onDownPressed: popup.moveFocus(true)

        ColumnLayout {
            id: col
            property var panelRef: popup // lets rows close their panel
            width: flick.width
            spacing: 2
        }
    }
}
