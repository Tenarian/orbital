import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#1a1a1a"

    property int viewMode: 0 // 0 = grid, 1 = list

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "Library"
                color: "#fff"
                font.pixelSize: 24
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            // View mode toggle
            Button {
                text: viewMode === 0 ? "Grid" : "List"
                onClicked: root.viewMode = viewMode === 0 ? 1 : 0
            }
        }

        // Game grid
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: libraryModel
            delegate: GameCard {
                width: 200
                onClicked: stackView.push(Qt.resolvedUrl('../views/GameDetailView.qml'), { gameId: model.id })
            }
            horizontalScrollBarPolicy: Qt.ScrollAsNeeded
            verticalScrollBarPolicy: Qt.ScrollAsNeeded
            wrapView: true
            flow: ListView.TopToBottom
        }
    }
}
