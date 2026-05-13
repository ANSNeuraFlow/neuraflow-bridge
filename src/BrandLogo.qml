import QtQuick
import QtQuick.Controls
import QtQuick.Layouts



RowLayout {
    /** When false (default), stretch and leave trailing space like a toolbar title. When true, shrink-wrap so parent can AlignHCenter. */
    property bool compact: false

    Layout.fillWidth: !compact
    spacing: 16

    FontLoader {
        id: spaceGroteskFont
        source: "fonts/space_grotesk_bold.ttf"
    }

    Item {
        id: logoContainer
        width: 40
        height: 40

        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 20
            height: parent.height + 20
            radius: 22
            color: "white"
            opacity: 0.02
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 16
            height: parent.height + 16
            radius: 20
            color: "white"
            opacity: 0.025
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 12
            height: parent.height + 12
            radius: 19
            color: "white"
            opacity: 0.03
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 8
            height: parent.height + 8
            radius: 18
            color: "white"
            opacity: 0.04
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 4
            height: parent.height + 4
            radius: 17
            color: "white"
            opacity: 0.05
        }

        Rectangle {
            id: logoBg
            anchors.fill: parent
            radius: 16
            color: "#f8fafc";

            Image {
                anchors.centerIn: parent;
                fillMode: Image.PreserveAspectFit
                source: Qt.resolvedUrl("icons/" + "brand_logo.svg")
                smooth: true
            }
        }
    }

    RowLayout {
        spacing: 0

        Label {
            text: "Neura"
            color: "#f8fafc"
            font.family: spaceGroteskFont.name
            font.pixelSize: 24
            font.bold: true
        }

        Text {
            id: gradientLabel
            text: "Flow"
            font.family: spaceGroteskFont.name
            font.pixelSize: 24
            font.bold: true
            color: "white"

            layer.enabled: true
            layer.effect: ShaderEffect {
                property color color1: "#60a5fa"
                property color color2: "#a78bfa"

                fragmentShader: "qrc:/shaders/animated-text.frag.qsb"
            }
        }
    }

    Item {
        Layout.fillWidth: true
        visible: !compact
    }
}

