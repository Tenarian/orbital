import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#1a1a1a"

    property string gameId: ""
    property var game: null

    // Load game data
    Component.onCompleted: {
        game = libraryModel.gameById(gameId)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        // Hero section
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 300
            color: "#2d2d2d"
            radius: 4

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                // Game title and play button
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    Text {
                        text: game ? game.name : "Loading..."
                        color: "#fff"
                        font.pixelSize: 32
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "▶ Play"
                        font.pixelSize: 16
                        font.bold: true
                        padding: 12
                        background: Rectangle {
                            color: "#2d8c2d"
                            radius: 4
                        }
                        onClicked: {
                            var config = {
                                "executable": game.executable,
                                "protonPath": "proton",
                                "prefixPath": ""
                            }
                            // Launch game
                        }
                    }
                }

                // Info bar
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    // Proton info
                    ColumnLayout {
                        spacing: 4

                        Text {
                            text: "Proton"
                            color: "#888"
                            font.pixelSize: 12
                        }
                        Text {
                            text: game ? game.protonVersion : "N/A"
                            color: "#fff"
                            font.pixelSize: 14
                        }
                    }

                    // Prefix info
                    ColumnLayout {
                        spacing: 4

                        Text {
                            text: "Prefix"
                            color: "#888"
                            font.pixelSize: 12
                        }
                        Text {
                            text: game ? (game.prefixMode === "shared" ? "Shared: " + game.sharedPrefixId : "Per-game") : "N/A"
                            color: "#fff"
                            font.pixelSize: 14
                        }
                    }

                    // Last played
                    ColumnLayout {
                        spacing: 4

                        Text {
                            text: "Last played"
                            color: "#888"
                            font.pixelSize: 12
                        }
                        Text {
                            text: game && game.lastPlayed > 0 ? formatLastPlayed(game.lastPlayed) : "Never"
                            color: "#fff"
                            font.pixelSize: 14
                        }
                    }
                }
            }
        }

        // Tab bar
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            fixedHeight: 40

            Tab { text: "Game Settings" }
            Tab { text: "Env Vars" }
            Tab { text: "Prefix" }
            Tab { text: "ProtonDB" }
        }

        // Tab content
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            currentIndex: tabBar.currentIndex

            // Game Settings tab
            ScrollView {
                TextArea {
                    anchors.fill: parent
                    text: "Game settings will appear here"
                    readOnly: true
                    wrapMode: TextEdit.WordWrap
                }
            }

            // Env Vars tab
            ScrollView {
                TextArea {
                    anchors.fill: parent
                    text: "Environment variables will appear here"
                    readOnly: true
                    wrapMode: TextEdit.WordWrap
                }
            }

            // Prefix tab
            ScrollView {
                TextArea {
                    anchors.fill: parent
                    text: "Prefix management will appear here"
                    readOnly: true
                    wrapMode: TextEdit.WordWrap
                }
            }

            // ProtonDB tab
            ScrollView {
                TextArea {
                    anchors.fill: parent
                    text: "ProtonDB information will appear here"
                    readOnly: true
                    wrapMode: TextEdit.WordWrap
                }
            }
        }
    }

    function formatLastPlayed(timestamp) {
        var now = Date.now() / 1000
        var diff = now - timestamp
        if (diff < 3600) return Math.floor(diff / 60) + "m ago"
        if (diff < 86400) return Math.floor(diff / 3600) + "h ago"
        if (diff < 604800) return Math.floor(diff / 86400) + "d ago"
        return "Recently"
    }
}
