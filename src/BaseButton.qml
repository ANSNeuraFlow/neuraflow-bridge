import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string bgColor: "#3b82f6"
    property string bgHover: "#60a5fa"
    property string bgPressed: "#2563eb"
    /** Smaller height and type for secondary toolbar actions */
    property bool compact: false

    background: Rectangle {
        implicitHeight: control.compact ? 32 : 40
        implicitWidth: control.compact ? 88 : 120
        radius: 8
        color: !control.enabled ? "#334155" : (control.down ? control.bgPressed : (control.hovered ? control.bgHover : control.bgColor))

        Behavior on color { ColorAnimation { duration: 150 } }
    }
    contentItem: Text {
        text: control.text
        font.pixelSize: control.compact ? 12 : 14
        font.bold: true
        color: !control.enabled ? "#94a3b8" : "#ffffff"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
