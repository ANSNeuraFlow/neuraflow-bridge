import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Config

ApplicationWindow {
    visible: true
    title: Config.appName
    color: "#0f172a"
    width: 900
    height: 620

    property var bridgeApiSettings: ({
        "webUrl": Config.bridgeApiWebUrl,
        "apiUrl": Config.bridgeApiApiUrl,
        "authStartPath": Config.bridgeAuthStartPath,
        "authTokenPath": Config.bridgeAuthTokenPath,
        "devicesPath": Config.bridgeDevicesPath,
        "streamWsUrl": Config.bridgeStreamWsUrl,
        "codeTtlSeconds": Config.bridgeCodeTtlSeconds,
        "allowedClientIds": Config.bridgeAllowedClientIds
    })

    component Card: Rectangle {
        color: "#1e293b"
        radius: 16
        border.color: "#334155"
        border.width: 1
    }

    // Nowoczesny przycisk
    component ModernButton: Button {
        id: control
        property string bgColor: "#3b82f6"
        property string bgHover: "#60a5fa"
        property string bgPressed: "#2563eb"

        background: Rectangle {
            implicitHeight: 40
            implicitWidth: 120
            radius: 8
            color: !control.enabled ? "#334155" : (control.down ? control.bgPressed : (control.hovered ? control.bgHover : control.bgColor))

            Behavior on color { ColorAnimation { duration: 150 } }
        }
        contentItem: Text {
            text: control.text
            font.pixelSize: 14
            font.bold: true
            color: !control.enabled ? "#94a3b8" : "#ffffff"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
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
        anchors.margins: 32
        spacing: 24

        // NAGŁÓWEK
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Rectangle {
                width: 4
                height: 32
                color: "#3b82f6"
                radius: 2
            }

            Label {
                text: "NeuraFlow Bridge Runtime"
                color: "#f8fafc"
                font.pixelSize: 28
                font.bold: true
                font.letterSpacing: 0.5
            }

            Item { Layout.fillWidth: true }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 2
            columnSpacing: 24
            rowSpacing: 24

            Card {
                Layout.fillWidth: true
                Layout.preferredHeight: 160

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 12

                    Label {
                        text: "System Status"
                        color: "#94a3b8"
                        font.pixelSize: 14
                        font.capitalization: Font.AllUppercase
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 12
                        StatusDot { active: SessionManager.authenticated }
                        Label {
                            text: SessionManager.authenticated ? "Authenticated" : "Not authenticated"
                            color: "#f8fafc"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }

                    Label {
                        text: "State: " + SessionManager.state
                        color: "#cbd5e1"
                        font.pixelSize: 14
                    }

                    Label {
                        text: SessionManager.statusMessage
                        color: "#94a3b8"
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        verticalAlignment: Text.AlignBottom
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.rowSpan: 2

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 20

                    Label {
                        text: "Device Control & Streaming"
                        color: "#94a3b8"
                        font.pixelSize: 14
                        font.capitalization: Font.AllUppercase
                        font.bold: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label { text: "Select Port"; color: "#cbd5e1"; font.pixelSize: 13 }

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
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        ModernButton {
                            Layout.fillWidth: true
                            text: "Connect Device"
                            enabled: SessionManager.authenticated && !SessionManager.deviceConnected
                            onClicked: SessionManager.connectDevice()
                        }

                        ModernButton {
                            Layout.fillWidth: true
                            text: "Disconnect"
                            bgColor: "#475569" // Szary
                            bgHover: "#64748b"
                            bgPressed: "#334155"
                            enabled: SessionManager.deviceConnected
                            onClicked: SessionManager.disconnectDevice()
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }

                    RowLayout {
                        spacing: 12
                        StatusDot {
                            active: SessionManager.streamConnected
                            color: active ? "#10b981" : "#f59e0b"
                        }
                        Label {
                            text: SessionManager.streamConnected ? "Uploader Active" : "Uploader Inactive"
                            color: "#f8fafc"
                            font.pixelSize: 16
                            font.bold: true
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        ModernButton {
                            Layout.fillWidth: true
                            text: "Start Streaming"
                            bgColor: "#10b981"
                            bgHover: "#34d399"
                            bgPressed: "#059669"
                            enabled: SessionManager.deviceConnected && !SessionManager.streamConnected
                            onClicked: SessionManager.startStreaming(bridgeApiSettings)
                        }

                        ModernButton {
                            Layout.fillWidth: true
                            text: "Stop Streaming"
                            bgColor: "#ef4444"
                            bgHover: "#f87171"
                            bgPressed: "#dc2626"
                            enabled: SessionManager.streamConnected
                            onClicked: SessionManager.stopStreaming()
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    Label {
                        text: "Authentication"
                        color: "#94a3b8"
                        font.pixelSize: 14
                        font.capitalization: Font.AllUppercase
                        font.bold: true
                    }

                    Item { Layout.fillHeight: true }

                    ModernButton {
                        Layout.fillWidth: true
                        text: "Login with Browser"
                        onClicked: SessionManager.beginLogin(bridgeApiSettings)
                    }

                    ModernButton {
                        Layout.fillWidth: true
                        text: "Refresh Ports"
                        bgColor: "#475569"
                        bgHover: "#64748b"
                        bgPressed: "#334155"
                        onClicked: SessionManager.refreshPorts()
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}