import QtQuick
import Lumina.Player

// Mouse / trackpad / touch dispatcher over the video surface. What each click, wheel turn and
// drag does is configured in Settings > Mouse (MouseBindings). Popups are modal, so clicks that
// merely dismiss a menu never reach this item.
MouseArea {
    id: ma

    property bool playlistOpen: false
    property bool gesturesEnabled: true
    property int seekStep: 5
    signal closeOverlays()
    signal fullscreenRequested()
    signal menuRequested()
    signal feedback(string text)

    acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton

    function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)) }

    function changeVolume(steps) {
        const sys = player.systemVolume >= 0
        const cur = sys ? player.systemVolume : player.engine.volume
        player.setVolumePercent(clamp(cur + steps, 0, sys ? 100 : 150))
    }
    function changeBrightness(steps) {
        const b = clamp((player.engine.videoAdjust.brightness || 0) + steps, -100, 100)
        player.engine.setVideoAdjust("brightness", b)
        ma.feedback(qsTr("Brightness %1").arg(b))
    }
    property real zoom: 0
    function changeZoom(delta) {
        zoom = clamp(zoom + delta, -1, 3)
        player.engine.setMpvOption("video-zoom", String(zoom))
        ma.feedback(qsTr("Zoom %1%").arg(Math.round(Math.pow(2, zoom) * 100)))
    }

    // Settings / self-test hooks.
    function bindAction(key, action) { return MouseBindings.set(key, action) }
    function binding(key) { return MouseBindings.get(key) }

    // Runs a button action (see MouseBindings.buttonActions).
    function run(action) {
        switch (action) {
        case "playpause": player.engine.togglePlayPause(); break
        case "fullscreen": ma.fullscreenRequested(); break
        case "mute": player.engine.setMuted(!player.engine.muted); break
        case "seekfwd": player.engine.seekRelative(seekStep); break
        case "seekback": player.engine.seekRelative(-seekStep); break
        case "next": player.next(); break
        case "prev": player.previous(); break
        case "screenshot": player.takeScreenshot(); break
        case "menu": ma.menuRequested(); break
        }
    }

    // Wheel action for `units` of angleDelta (120 = one mouse notch; positive = up/louder).
    property real wheelAcc: 0
    function wheelAction(action, units) {
        wheelAcc += units
        if (action === "volume" || action === "zoom") {
            const steps = Math.trunc(wheelAcc / 24)
            if (steps === 0) return
            wheelAcc -= steps * 24
            if (action === "volume") changeVolume(steps)
            else changeZoom(steps * 0.02)
        } else if (action === "seek") {
            const steps = Math.trunc(wheelAcc / 120)
            if (steps === 0) return
            wheelAcc -= steps * 120
            player.engine.seekRelative(steps * seekStep)
        } else {
            wheelAcc = 0
        }
    }

    onWheel: (w) => {
        const mods = w.modifiers
        const key = (mods & (Qt.ControlModifier | Qt.MetaModifier)) ? "ctrlWheel"
                  : (mods & Qt.ShiftModifier) ? "shiftWheel" : "wheel"
        // Shift+wheel arrives as a horizontal scroll on some platforms.
        const d = w.angleDelta.y !== 0 ? w.angleDelta.y : w.angleDelta.x
        // `inverted` = natural scrolling: flip so fingers/wheel moving "up" always means louder.
        wheelAction(MouseBindings.get(key), d * (w.inverted ? -1 : 1))
    }

    // --- clicks ---
    Timer {
        id: clickTimer
        interval: 220
        onTriggered: ma.run(MouseBindings.get("click"))
    }
    onClicked: (m) => {
        if (moved) return
        if (m.button === Qt.MiddleButton) run(MouseBindings.get("middle"))
        else if (m.button === Qt.RightButton) run(MouseBindings.get("right"))
        else if (playlistOpen) closeOverlays()
        // Only wait for a possible second click when double-click is bound to something.
        else if (MouseBindings.get("dblclick") === "none") run(MouseBindings.get("click"))
        else clickTimer.restart()
    }
    onDoubleClicked: (m) => {
        if (m.button === Qt.LeftButton) {
            clickTimer.stop()
            run(MouseBindings.get("dblclick"))
        } else {
            clicked(m)
        }
    }

    // --- drag gestures: horizontal = seek, vertical (left half brightness / right half volume) ---
    property real px: 0
    property real py: 0
    property real lastY: 0
    property real seekDelta: 0
    property bool moved: false
    property string dragKind: "" // "", "h", "v", "x" (not bound)
    property real vAcc: 0
    onPressed: (m) => {
        px = m.x; py = m.y; lastY = m.y
        moved = false; dragKind = ""; seekDelta = 0; vAcc = 0
    }
    onPositionChanged: (m) => {
        if (!pressed || !gesturesEnabled || m.buttons !== Qt.LeftButton) return
        const dx = m.x - px, dy = m.y - py
        if (dragKind === "") {
            if (Math.abs(dx) < 16 && Math.abs(dy) < 16) return
            moved = true
            if (Math.abs(dx) >= Math.abs(dy))
                dragKind = MouseBindings.get("swipeH") === "seek" ? "h" : "x"
            else
                dragKind = MouseBindings.get("edgeV") === "edge" ? "v" : "x"
            lastY = m.y
        }
        if (dragKind === "h") {
            seekDelta = dx * 0.5 // half a second per pixel
            ma.feedback((seekDelta >= 0 ? "+" : "") + Math.round(seekDelta) + " s")
        } else if (dragKind === "v") {
            vAcc += (lastY - m.y) * 100 / Math.max(120, height * 0.6)
            lastY = m.y
            const steps = Math.trunc(vAcc)
            if (steps === 0) return
            vAcc -= steps
            if (px < width / 2) changeBrightness(steps)
            else changeVolume(steps)
        }
    }
    onReleased: {
        if (dragKind === "h" && Math.abs(seekDelta) >= 1)
            player.engine.seekRelative(seekDelta)
        dragKind = ""
    }
    onCanceled: dragKind = ""
}
