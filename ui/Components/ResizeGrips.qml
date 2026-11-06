import QtQuick
import Lumina.Player

// Invisible edge/corner grips for a frameless window: hover shows the resize cursor, press
// hands the drag to the window system (QQuickWindow::startSystemResize).
Item {
    id: grips

    required property var window
    property int thickness: 6
    // Corners are larger so they are easy to hit.
    readonly property int corner: thickness * 2

    component Grip: MouseArea {
        required property int edges
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onPressed: grips.window.startSystemResize(edges)
    }

    Grip { edges: Qt.TopEdge;    cursorShape: Qt.SizeVerCursor; anchors { top: parent.top; left: parent.left; right: parent.right; leftMargin: grips.corner; rightMargin: grips.corner } height: grips.thickness }
    Grip { edges: Qt.BottomEdge; cursorShape: Qt.SizeVerCursor; anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: grips.corner; rightMargin: grips.corner } height: grips.thickness }
    Grip { edges: Qt.LeftEdge;   cursorShape: Qt.SizeHorCursor; anchors { left: parent.left; top: parent.top; bottom: parent.bottom; topMargin: grips.corner; bottomMargin: grips.corner } width: grips.thickness }
    Grip { edges: Qt.RightEdge;  cursorShape: Qt.SizeHorCursor; anchors { right: parent.right; top: parent.top; bottom: parent.bottom; topMargin: grips.corner; bottomMargin: grips.corner } width: grips.thickness }
    Grip { edges: Qt.TopEdge | Qt.LeftEdge;     cursorShape: Qt.SizeFDiagCursor; anchors { top: parent.top; left: parent.left } width: grips.corner; height: grips.corner }
    Grip { edges: Qt.BottomEdge | Qt.RightEdge; cursorShape: Qt.SizeFDiagCursor; anchors { bottom: parent.bottom; right: parent.right } width: grips.corner; height: grips.corner }
    Grip { edges: Qt.TopEdge | Qt.RightEdge;    cursorShape: Qt.SizeBDiagCursor; anchors { top: parent.top; right: parent.right } width: grips.corner; height: grips.corner }
    Grip { edges: Qt.BottomEdge | Qt.LeftEdge;  cursorShape: Qt.SizeBDiagCursor; anchors { bottom: parent.bottom; left: parent.left } width: grips.corner; height: grips.corner }
}
