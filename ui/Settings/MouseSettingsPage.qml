import QtQuick
import Lumina.Player

// Settings > Mouse: what clicks, the wheel and drag gestures do (see MouseActions.qml).
SettingsPage {
    id: page
    title: qsTr("Mouse")
    resettable: false

    readonly property var labels: ({
        playpause: qsTr("Play / pause"), fullscreen: qsTr("Fullscreen"), mute: qsTr("Mute"),
        seekfwd: qsTr("Seek forward"), seekback: qsTr("Seek back"), next: qsTr("Next item"),
        prev: qsTr("Previous item"), screenshot: qsTr("Screenshot"), menu: qsTr("Open menu"),
        none: qsTr("Do nothing"), volume: qsTr("Volume"), seek: qsTr("Seek"), zoom: qsTr("Zoom"),
        edge: qsTr("Brightness / volume")
    })
    function modelFor(key) {
        return MouseBindings.optionsFor(key).map(function (v) { return { label: labels[v], value: v } })
    }

    component BindRow: ComboRow {
        property string key
        name: "mouse_" + key
        model: page.modelFor(key)
        value: MouseBindings.get(key)
        onPicked: (v) => MouseBindings.set(key, v)
    }

    SettingSection {
        title: qsTr("Buttons")
        BindRow { key: "click"; title: qsTr("Single click"); keywords: "left button tap pause" }
        BindRow { key: "dblclick"; title: qsTr("Double click"); keywords: "left button fullscreen"
                  description: qsTr("When set to nothing, a single click acts instantly.") }
        BindRow { key: "middle"; title: qsTr("Middle click"); keywords: "wheel button" }
        BindRow { key: "right"; title: qsTr("Right click"); keywords: "context menu" }
    }
    SettingSection {
        title: qsTr("Wheel")
        BindRow { key: "wheel"; title: qsTr("Scroll wheel"); keywords: "mouse wheel touchpad scroll volume" }
        BindRow { key: "ctrlWheel"; title: qsTr("Ctrl + wheel"); keywords: "control modifier" }
        BindRow { key: "shiftWheel"; title: qsTr("Shift + wheel"); keywords: "modifier" }
    }
    SettingSection {
        title: qsTr("Touch and trackpad gestures")
        BindRow { key: "swipeH"; title: qsTr("Horizontal drag")
                  description: qsTr("Drag left or right over the video; the seek happens on release.")
                  keywords: "swipe touch seek gesture" }
        BindRow { key: "edgeV"; title: qsTr("Vertical drag")
                  description: qsTr("Drag up or down over the video: left half brightness, right half volume.")
                  keywords: "swipe touch brightness volume gesture" }
    }
    SettingSection {
        ButtonRow {
            name: "mouseReset"
            title: qsTr("Restore defaults")
            description: qsTr("Click pauses, double click toggles fullscreen, the wheel changes volume.")
            keywords: "reset mouse"
            buttonText: qsTr("Restore defaults")
            onClicked: MouseBindings.resetAll()
        }
    }
}
