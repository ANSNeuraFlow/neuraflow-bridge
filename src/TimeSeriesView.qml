import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property var windowLabels: ["1 sec", "3 sec", "5 sec", "10 sec", "20 sec"]
    readonly property var vertLabels: ["Auto", "50 µV", "100 µV", "200 µV", "400 µV", "1000 µV", "10000 µV"]

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                text: qsTr("EEG time series")
                color: "#94a3b8"
                font.pixelSize: 14
                font.capitalization: Font.AllUppercase
                font.bold: true
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Vert scale")
                color: "#cbd5e1"
                font.pixelSize: 13
            }

            ComboBox {
                id: vertCombo
                implicitWidth: 130
                model: vertLabels
                currentIndex: TimeSeriesController.vertScaleIndex

                background: Rectangle {
                    color: "#0f172a"
                    border.color: vertCombo.popup.visible ? "#3b82f6" : "#334155"
                    radius: 8
                    implicitHeight: 36
                    Behavior on border.color { ColorAnimation { duration: 150 } }
                }

                contentItem: Text {
                    text: vertCombo.displayText
                    color: "#f8fafc"
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 12
                }

                onActivated: function (i) {
                    TimeSeriesController.vertScaleIndex = i
                }

                Connections {
                    target: TimeSeriesController
                    function onVertScaleIndexChanged() {
                        vertCombo.currentIndex = TimeSeriesController.vertScaleIndex
                    }
                }
            }

            Label {
                text: qsTr("Window")
                color: "#cbd5e1"
                font.pixelSize: 13
            }

            ComboBox {
                id: winCombo
                implicitWidth: 110
                model: windowLabels
                currentIndex: TimeSeriesController.horizontalWindowIndex

                background: Rectangle {
                    color: "#0f172a"
                    border.color: winCombo.popup.visible ? "#3b82f6" : "#334155"
                    radius: 8
                    implicitHeight: 36
                    Behavior on border.color { ColorAnimation { duration: 150 } }
                }

                contentItem: Text {
                    text: winCombo.displayText
                    color: "#f8fafc"
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 12
                }

                onActivated: function (i) {
                    TimeSeriesController.horizontalWindowIndex = i
                }

                Connections {
                    target: TimeSeriesController
                    function onHorizontalWindowIndexChanged() {
                        winCombo.currentIndex = TimeSeriesController.horizontalWindowIndex
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#334155"
        }

        ListView {
            id: chList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2
            model: TimeSeriesController.numChannels

            delegate: ChannelBar {
                required property int index
                width: chList.width
                height: Math.max(80, (chList.height - 7 * chList.spacing) / 8)
                channelIndex: index
                isBottom: index === TimeSeriesController.numChannels - 1
            }
        }

        Label {
            Layout.alignment: Qt.AlignRight
            font.pixelSize: 11
            color: "#b3b3b3"
            text: Qt.formatTime(new Date(), "HH:mm:ss")

            Timer {
                interval: 1000
                running: true
                repeat: true
                onTriggered: parent.text = Qt.formatTime(new Date(), "HH:mm:ss")
            }
        }
    }
}
