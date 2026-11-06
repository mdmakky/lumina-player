import QtQuick
import Lumina.Player

// Shift + drag over the video pans the picture (engine picture panx/pany).
Item {
    id: layer

    DragHandler {
        id: drag
        target: null
        acceptedModifiers: Qt.ShiftModifier
        property point last
        onActiveChanged: last = Qt.point(0, 0)
        onTranslationChanged: {
            const dx = translation.x - last.x, dy = translation.y - last.y
            last = Qt.point(translation.x, translation.y)
            if (!active || layer.width <= 0) return
            const p = player.engine.picture
            player.engine.setPicture("panx", p.panx + dx / layer.width)
            player.engine.setPicture("pany", p.pany + dy / layer.height)
        }
    }
}
