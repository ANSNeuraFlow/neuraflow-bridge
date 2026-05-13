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

    Component {
        id: splashScreen

        Item {
            anchors.fill: parent

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 24
                width: Math.min(parent.width - 64, 420)

                BrandLogo {
                    compact: true
                    Layout.alignment: Qt.AlignHCenter
                }

                BaseButton {
                    Layout.fillWidth: true
                    text: "Login with Browser"
                    onClicked: SessionManager.beginLogin()
                }

                Label {
                    visible: text.length > 0
                    text: SessionManager.statusMessage
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    color: "#94a3b8"
                    font.pixelSize: 13
                }
            }
        }
    }

    Component {
        id: authenticatingScreen

        Item {
            anchors.fill: parent

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 24
                width: Math.min(parent.width - 64, 420)

                BrandLogo {
                    compact: true
                    Layout.alignment: Qt.AlignHCenter
                }

                BaseBusyIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    running: true
                }

                Label {
                    text: SessionManager.statusMessage.length > 0
                          ? SessionManager.statusMessage
                          : qsTr("Completing sign-in…")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    color: "#cbd5e1"
                    font.pixelSize: 14
                }
            }
        }
    }

    Component {
        id: errorScreen

        Item {
            anchors.fill: parent

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 24
                width: Math.min(parent.width - 64, 420)

                BrandLogo {
                    compact: true
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: SessionManager.statusMessage.length > 0
                          ? SessionManager.statusMessage
                          : qsTr("Something went wrong.")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    color: "#f87171"
                    font.pixelSize: 14
                }

                BaseButton {
                    Layout.fillWidth: true
                    text: "Try again"
                    onClicked: SessionManager.beginLogin()
                }
            }
        }
    }

    Component {
        id: dashboardScreen
        DashboardPage {}
}

    Loader {
        id: screenLoader
        anchors.fill: parent
        sourceComponent: SessionManager.uiState === "ready"
                         ? dashboardScreen
                         : SessionManager.uiState === "authenticating"
                           ? authenticatingScreen
                           : SessionManager.uiState === "error"
                             ? errorScreen
                             : splashScreen
    }
}
