import QtQuick
import Lumina.Player

// A row whose control is one or two action buttons (objectNames `name` and `name2`).
SettingRow {
    id: row

    property string name
    property string buttonText
    property string button2Text
    property bool primary: false
    property bool danger: false
    property bool buttonEnabled: true
    signal clicked()
    signal clicked2()

    SettingsButton {
        visible: row.button2Text !== ""
        objectName: row.name + "2"
        text: row.button2Text
        onClicked: row.clicked2()
    }
    SettingsButton {
        objectName: row.name
        text: row.buttonText
        primary: row.primary
        danger: row.danger
        enabled: row.buttonEnabled
        onClicked: row.clicked()
    }
}
