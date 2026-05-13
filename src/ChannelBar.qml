import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NeuraFlowBridge

Item {
    id: root
    required property int channelIndex
    property bool isBottom: false

    readonly property color traceColor: TimeSeriesController.channelColor(channelIndex)
    readonly property bool chVis: TimeSeriesController.channelVisible(channelIndex)

    implicitHeight: 100

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: Qt.rgba(1, 1, 1, 0.06)
        border.width: 1
        radius: 4
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 8

        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            width: 28
            height: 28
            radius: 14
            color: chVis ? root.traceColor : "#475569"

            Text {
                anchors.centerIn: parent
                text: String(channelIndex + 1)
                color: "#f8fafc"
                font.pixelSize: 13
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: TimeSeriesController.toggleChannelVisibility(channelIndex)
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            opacity: chVis ? 1.0 : 0.35

            ColumnLayout {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 52
                spacing: 0

                Label {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    text: "+" + TimeSeriesController.yMaxForChannel(channelIndex).toFixed(0) + " µV"
                    font.pixelSize: 10
                    color: "#b3b3b3"
                }

                Item {
                    Layout.fillHeight: true
                }

                Label {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
                    text: TimeSeriesController.yMinForChannel(channelIndex).toFixed(0) + " µV"
                    font.pixelSize: 10
                    color: "#b3b3b3"
                }
            }

            QCustomPlotQuickItem {
                id: plot
                anchors.fill: parent
                anchors.leftMargin: 54
                lineColor: root.traceColor
                bottomAxisVisible: root.isBottom

                function refresh() {
                    TimeSeriesController.updatePlot(plot, channelIndex)
                }

                Connections {
                    target: DataProcessor
                    function onRenderTick() {
                        plot.refresh()
                    }
                }
                Connections {
                    target: TimeSeriesController
                    function onVertScaleIndexChanged() {
                        plot.refresh()
                    }
                    function onHorizontalWindowIndexChanged() {
                        plot.refresh()
                    }
                    function onActiveChannelsChanged() {
                        plot.refresh()
                    }
                }
                Component.onCompleted: refresh()
            }

            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 4
                width: rmsLabel.width + 12
                height: rmsLabel.height + 8
                color: "#14181d"
                border.color: Qt.rgba(1, 1, 1, 0.08)
                border.width: 1
                radius: 4

                Label {
                    id: rmsLabel
                    anchors.centerIn: parent
                    text: {
                        const list = DataProcessor.rmsMicrovolts
                        if (!root.chVis)
                            return "—"
                        const v = (list.length > channelIndex) ? list[channelIndex] : 0
                        return Number(v).toFixed(4) + " µVrms"
                    }
                    font.pixelSize: 11
                    color: "#ffffff"
                }
            }
        }
    }
}
