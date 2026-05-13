import QtQuick
import QtQuick.Controls

BusyIndicator {
    id: control

    property string bgColor: "#3b82f6"
    property string bgHover: "#60a5fa"
    property string bgPressed: "#2563eb"
    /** Muted ring; lighter than slate-700 so it reads on #0f172a */
    property string trackColor: "#64748b"

    implicitWidth: 56
    implicitHeight: 56

    padding: 0

    contentItem: Item {
        implicitWidth: 56
        implicitHeight: 56

        Canvas {
            id: ringCanvas
            anchors.fill: parent
            renderTarget: Canvas.FramebufferObject
            renderStrategy: Canvas.Cooperative

            property real sweepDeg: 0

            NumberAnimation on sweepDeg {
                id: sweepAnim
                from: 0
                to: 360
                duration: 1100
                loops: Animation.Infinite
                running: control.running
                easing.type: Easing.Linear
            }

            onSweepDegChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()

                const w = width
                const h = height
                const lineW = 4
                const cx = w * 0.5
                const cy = h * 0.5
                const r = Math.min(w, h) * 0.5 - lineW * 0.5

                ctx.lineWidth = lineW
                ctx.lineCap = "round"

                ctx.strokeStyle = control.trackColor
                ctx.beginPath()
                ctx.arc(cx, cy, r, 0, Math.PI * 2)
                ctx.stroke()

                ctx.strokeStyle = control.bgHover
                const start = (sweepDeg - 90) * (Math.PI / 180)
                const span = (270 * Math.PI) / 180
                ctx.beginPath()
                ctx.arc(cx, cy, r, start, start + span)
                ctx.stroke()
            }

            Component.onCompleted: requestPaint()
        }
    }
}
