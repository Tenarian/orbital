import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Orbital 1.0

ApplicationWindow {
    id: root
    width: 1280
    height: 800
    visible: true
    title: "Orbital Game Launcher"

    // Models
    LibraryModel { id: libraryModel }
    ProtonModel { id: protonModel }
    EnvVarModel { id: envVarModel }

    // Config
    property bool sidebarCollapsed: false

    // Main layout
    SplitView {
        anchors.fill: parent

        // Sidebar
        Rectangle {
            id: sidebar
            width: 280
            color: "#1e1e1e"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8

                // Search bar
                TextField {
                    Layout.fillWidth: true
                    placeholderText: "Search games..."
                    icon.name: "system-search"

                    onTextChanged: libraryModel.refresh()
                }

                // Game list
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: libraryModel
                    delegate: GameCard {
                        width: ListView.view.width
                        onClicked: stackView.push(Qt.resolvedUrl('views/GameDetailView.qml'), { gameId: model.id })
                    }
                    highlight: Rectangle { color: "#2d2d2d" }
                    focus: true
                }

                // Footer
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Button {
                        Layout.fillWidth: true
                        text: "Add Game"
                        onClicked: { /* Add game dialog */ }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Settings"
                        onClicked: stackView.push(Qt.resolvedUrl('views/SettingsView.qml'))
                    }
                }
            }
        }

        // Main content area
        StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: LibraryView { id: libraryView }
        }
    }

    // On startup
    Component.onCompleted: {
        libraryModel.refresh()
        protonModel.refresh()
        envVarModel.load()
    }
}
