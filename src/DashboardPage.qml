import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    anchors.fill: parent

    component Card: Rectangle {
        color: "#1e293b"
        radius: 16
        border.color: "#334155"
        border.width: 1
    }

    component SectionTitle: Label {
        color: "#94a3b8"
        font.pixelSize: 12
        font.capitalization: Font.AllUppercase
        font.bold: true
    }

    component StatusDot: Rectangle {
        property bool active: false
        width: 12
        height: 12
        radius: 6
        color: active ? "#10b981" : "#ef4444"

        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 8
            height: parent.height + 8
            radius: width / 2
            color: parent.color
            opacity: 0.2
            visible: parent.active
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            BrandLogo {
                compact: true
            }

            Item {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }

            BaseButton {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 100
                Layout.preferredHeight: 34
                text: "Logout"
                bgColor: "#ef4444"
                bgHover: "#f87171"
                bgPressed: "#dc2626"
                onClicked: SessionManager.logout()
            }
        }

        Item {
            Layout.fillHeight: true
            visible: !SessionManager.deviceStreaming
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 16
            rowSpacing: 0

            Card {
                Layout.fillWidth: true
                Layout.minimumWidth: 200
                implicitHeight: connectionCol.implicitHeight + 36

                ColumnLayout {
                    id: connectionCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 18
                    spacing: 10

                    SectionTitle {
                        text: "Connection"
                        Layout.bottomMargin: 4
                    }

                    Label {
                        text: "Select Port"
                        color: "#cbd5e1"
                        font.pixelSize: 13
                    }

                    ComboBox {
                        id: portComboBox
                        Layout.fillWidth: true
                        model: SessionManager.availablePorts
                        currentIndex: Math.max(0, model.indexOf(SessionManager.selectedPort))

                        background: Rectangle {
                            color: "#0f172a"
                            border.color: portComboBox.popup.visible ? "#3b82f6" : "#334155"
                            radius: 8
                            implicitHeight: 44

                            Behavior on border.color { ColorAnimation { duration: 150 } }
                        }

                        contentItem: Text {
                            text: portComboBox.displayText
                            color: "#f8fafc"
                            font.pixelSize: 14
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 16
                        }

                        indicator: Canvas {
                            id: portComboIndicator
                            implicitWidth: 18
                            implicitHeight: 10
                            width: implicitWidth
                            height: implicitHeight

                            readonly property int edgeInset: 14

                            x: portComboBox.mirrored
                                   ? portComboBox.padding + edgeInset
                                   : portComboBox.width - width - portComboBox.padding - edgeInset
                            y: portComboBox.topPadding
                               + (portComboBox.availableHeight - height) / 2

                            readonly property color arrowColor: portComboBox.popup.visible
                                                      ? "#e0f2fe"
                                                      : "#f8fafc"

                            onArrowColorChanged: requestPaint()

                            onPaint: {
                                let ctx = getContext("2d")
                                ctx.reset()
                                ctx.strokeStyle = arrowColor
                                ctx.lineWidth = 2.25
                                ctx.lineCap = "round"
                                ctx.lineJoin = "round"
                                ctx.beginPath()
                                ctx.moveTo(2, 3.5)
                                ctx.lineTo(9, 8.5)
                                ctx.moveTo(9, 8.5)
                                ctx.lineTo(16, 3.5)
                                ctx.stroke()
                            }
                        }

                        delegate: ItemDelegate {
                            width: portComboBox.width - 8
                            padding: 12

                            contentItem: Text {
                                text: modelData
                                color: highlighted ? "#ffffff" : "#cbd5e1"
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                color: highlighted ? "#334155" : "transparent"
                                radius: 6
                            }

                            highlighted: portComboBox.highlightedIndex === index
                        }

                        popup: Popup {
                            y: portComboBox.height + 4
                            width: portComboBox.width
                            implicitHeight: contentItem.implicitHeight
                            padding: 4

                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight > 240 ? 240 : contentHeight
                                model: portComboBox.popup.visible ? portComboBox.delegateModel : null
                                currentIndex: portComboBox.highlightedIndex

                                ScrollBar.vertical: ScrollBar {
                                    policy: ScrollBar.AsNeeded
                                    contentItem: Rectangle {
                                        implicitWidth: 4
                                        radius: 2
                                        color: "#475569"
                                    }
                                }
                            }

                            background: Rectangle {
                                color: "#1e293b"
                                border.color: "#334155"
                                radius: 8
                            }
                        }

                        onActivated: function(index) {
                            SessionManager.selectedPort = model[index]
                        }
                    }

                    BaseButton {
                        Layout.fillWidth: true
                        text: "Auto-connect"
                        bgColor: "#10b981"
                        bgHover: "#34d399"
                        bgPressed: "#059669"
                        enabled: SessionManager.authenticated && !SessionManager.deviceConnected
                        onClicked: SessionManager.autoConnectDevice()
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        BaseButton {
                            Layout.fillWidth: true
                            compact: true
                            text: "Refresh"
                            bgColor: "#475569"
                            bgHover: "#64748b"
                            bgPressed: "#334155"
                            onClicked: SessionManager.refreshPorts()
                        }

                        BaseButton {
                            Layout.fillWidth: true
                            compact: true
                            text: "Connect"
                            enabled: SessionManager.authenticated && !SessionManager.deviceConnected
                            onClicked: SessionManager.connectDevice()
                        }

                        BaseButton {
                            Layout.fillWidth: true
                            compact: true
                            text: "Disconnect"
                            bgColor: "#475569"
                            bgHover: "#64748b"
                            bgPressed: "#334155"
                            enabled: SessionManager.deviceConnected
                            onClicked: SessionManager.disconnectDevice()
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.minimumWidth: 200
                implicitHeight: boardCol.implicitHeight + 36

                ColumnLayout {
                    id: boardCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 18
                    spacing: 8

                    SectionTitle {
                        text: "Board"
                    }

                    Label {
                        text: SessionManager.connectionStatus
                        color: "#e2e8f0"
                        font.pixelSize: 14
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: SessionManager.firmwareVersion.length > 0
                        text: "Firmware: " + SessionManager.firmwareVersion
                        color: "#94a3b8"
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: SessionManager.deviceConnected
                        text: SessionManager.boardReady ? "Board ready — you can start streaming"
                              : "Initializing protocol (v / d / c)…"
                        color: SessionManager.boardReady ? "#34d399" : "#fbbf24"
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        spacing: 10

                        StatusDot {
                            active: SessionManager.streamConnected
                            color: active ? "#10b981" : "#64748b"
                        }

                        Label {
                            Layout.fillWidth: true
                            text: SessionManager.streamConnected ? "Cloud uploader connected"
                                                                   : "Cloud uploader idle"
                            color: "#cbd5e1"
                            font.pixelSize: 13
                            font.bold: true
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }

        Card {
            Layout.fillWidth: true
            implicitHeight: streamingCol.implicitHeight + 36

            ColumnLayout {
                id: streamingCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 18
                spacing: 10

                SectionTitle {
                    text: "Streaming"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    BaseButton {
                        Layout.fillWidth: true
                        text: "Start Streaming"
                        bgColor: "#10b981"
                        bgHover: "#34d399"
                        bgPressed: "#059669"
                        enabled: SessionManager.deviceConnected
                                 && SessionManager.boardReady
                                 && !SessionManager.deviceStreaming
                        onClicked: SessionManager.startStreaming()
                    }

                    BaseButton {
                        Layout.fillWidth: true
                        text: "Stop Streaming"
                        bgColor: "#ef4444"
                        bgHover: "#f87171"
                        bgPressed: "#dc2626"
                        enabled: SessionManager.deviceStreaming
                        onClicked: SessionManager.stopStreaming()
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
            visible: !SessionManager.deviceStreaming
        }

        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 280
            visible: SessionManager.deviceStreaming

            TimeSeriesView {
                anchors.fill: parent
                anchors.margins: 16
            }
        }
    }
}
