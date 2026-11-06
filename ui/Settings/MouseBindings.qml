pragma Singleton
import QtQuick
import Lumina.Player

// Mouse/gesture bindings (spec 7.7): which action each click, wheel and drag runs.
// Persisted as mouse/<key>; MouseActions.qml dispatches them, MouseSettingsPage edits them.
QtObject {
    id: mb

    // Defaults reproduce the pre-editor behaviour (click pause, double-click fullscreen, wheel volume).
    readonly property var defaults: ({
        click: "playpause", dblclick: "fullscreen", middle: "none", right: "none",
        wheel: "volume", ctrlWheel: "seek", shiftWheel: "volume",
        swipeH: "none", edgeV: "none"
    })
    readonly property var buttonActions: ["playpause", "fullscreen", "mute", "seekfwd", "seekback",
                                          "next", "prev", "screenshot", "menu", "none"]
    readonly property var wheelActions: ["volume", "seek", "zoom", "none"]
    readonly property var swipeActions: ["seek", "none"]
    readonly property var edgeActions: ["edge", "none"] // left half brightness, right half volume
    property var values: ({})

    function optionsFor(key) {
        return key.indexOf("heel") >= 0 ? wheelActions : key === "swipeH" ? swipeActions
             : key === "edgeV" ? edgeActions : buttonActions
    }
    function get(key) {
        const v = values[key]
        return v !== undefined && optionsFor(key).indexOf(v) >= 0 ? v : defaults[key]
    }
    function set(key, value) {
        if (defaults[key] === undefined || optionsFor(key).indexOf(value) < 0) return false
        const o = {}
        for (const k in values) o[k] = values[k]
        o[key] = value
        values = o
        player.setSetting("mouse/" + key, value)
        return true
    }
    function resetAll() {
        for (const k in defaults) set(k, defaults[k])
    }
    Component.onCompleted: {
        const o = {}
        for (const k in defaults) o[k] = player.setting("mouse/" + k, defaults[k])
        values = o
    }
}
