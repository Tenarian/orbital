import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "transparent"

    property string gameName: ""
    property string coverArt: ""
    property string heroImage: ""
    property string logo: ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        // Hero image background
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 400
            color: "#1a1a1a"

            Image {
                anchors.fill: parent
                source: heroImage
                fillMode: Image.PreserveAspectFit
                smooth: true
                opacity: 0.3
            }

            // Gradient overlay
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 1.0; color: "#1a1a1a" }
                }
            }

            // Game title overlay
            Text {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 16
                text: gameName
                color: "#fff"
                font.pixelSize: 48
                font.bold: true
                elide: Text.ElideRight
                maximumWidth: parent.width - 32
            }
        }

        // Info bar below hero
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            spacing: 16

            // HLTB block
            ColumnLayout {
                spacing: 2

                Text {
                    text: "HLTB"
                    color: "#888"
                    font.pixelSize: 11
                }
                Text {
                    text: "Main: 45h"
                    color: "#fff"
                    font.pixelSize: 13
                }
                Text {
                    text: "Main+: 80h"
                    color: "#888"
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            // Proton info
            ColumnLayout {
                spacing: 2

                Text {
                    text: "Proton"
                    color: "#888"
                    font.pixelSize: 11
                }
                Text {
                    text: "GE-Proton9-7"
                    color: "#fff"
                    font.pixelSize: 13
                }
            }

            // Prefix info
            ColumnLayout {
                spacing: 2

                Text {
                    text: "Prefix"
                    color: "#888"
                    font.pixelSize: 11
                }
                Text {
                    text: "Per-game"
                    color: "#fff"
                    font.pixelSize: 13
                }
            }

            // Last played
            ColumnLayout {
                spacing: 2

                Text {
                    text: "Last played"
                    color: "#888"
                    font.pixelSize: 11
                }
                Text {
                    text: "2 days ago"
                    color: "#fff"
                    font.pixelSize: 13
                }
            }
        }
    }
}
