import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#1a1a1a"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Text {
            text: "Settings"
            color: "#fff"
            font.pixelSize: 24
            font.bold: true
            Layout.fillWidth: true
        }

        GroupBox {
            Layout.fillWidth: true
            title: "General"

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                CheckBox {
                    text: "Check for Proton updates on startup"
                    checked: true
                }

                CheckBox {
                    text: "Suppress environment variable warnings"
                    checked: false
                }

                TextField {
                    Layout.fillWidth: true
                    placeholderText: "SteamGridDB API Key"
                    text: ""
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            title: "Paths"

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Text {
                    text: "Config: ~/.config/orbital/"
                    color: "#888"
                    font.pixelSize: 12
                }
                Text {
                    text: "Data: ~/.local/share/orbital/"
                    color: "#888"
                    font.pixelSize: 12
                }
                Text {
                    text: "Cache: ~/.cache/orbital/"
                    color: "#888"
                    font.pixelSize: 12
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
