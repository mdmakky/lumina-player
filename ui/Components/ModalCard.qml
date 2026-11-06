import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Centered dialog: title block with a close affordance, free body, optional footer row.
// Put content in as children; put buttons inside a ModalFooter.
Popup {
    id: card

    property string title: ""
    property string subtitle: ""
    property int cardWidth: 480
    property bool closable: true
    default property alias body: bodyCol.data

    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(cardWidth, (parent ? parent.width : cardWidth) - 48)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle { color: Theme.scrimModal }
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.animationMedium; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.97; to: 1; duration: Theme.animationMedium; easing.type: Easing.OutCubic }
    }
    exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.animationFast } }
    background: Surface { elevation: 2 }

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 24
            Layout.bottomMargin: card.subtitle !== "" ? 6 : 14
            spacing: Theme.spacingMedium
            Text {
                Layout.fillWidth: true
                text: card.title
                color: Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeHeading + 1
                font.weight: Font.DemiBold
                wrapMode: Text.Wrap
                Accessible.role: Accessible.Heading
            }
            AbstractButton {
                visible: card.closable
                Layout.alignment: Qt.AlignTop
                implicitWidth: 30; implicitHeight: 30
                hoverEnabled: true
                focusPolicy: Qt.StrongFocus
                Accessible.name: qsTr("Close")
                onClicked: card.close()
                background: Rectangle {
                    radius: 15
                    color: parent.down ? Theme.surfacePressed : parent.hovered ? Theme.surfaceHover : "transparent"
                    border.width: parent.visualFocus ? 2 : 0
                    border.color: Theme.focusRing
                }
                contentItem: Item { Glyph { anchors.centerIn: parent; name: "close"; size: 14; tint: Theme.secondaryText } }
            }
        }
        Text {
            visible: card.subtitle !== ""
            Layout.fillWidth: true
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.bottomMargin: 14
            text: card.subtitle
            wrapMode: Text.Wrap
            color: Theme.secondaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            lineHeight: 1.15
        }
        ColumnLayout {
            id: bodyCol
            Layout.fillWidth: true
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.bottomMargin: 20
            spacing: Theme.spacingMedium
        }
    }
}
