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
            text: "Proton Manager"
            color: "#fff"
            font.pixelSize: 24
            font.bold: true
            Layout.fillWidth: true
        }

        // Latest version info
        Text {
            text: "Latest: " + protonModel.latestVersion
            color: "#888"
            font.pixelSize: 14
        }

        // Version list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: protonModel
            delegate: Rectangle {
                width: ListView.view.width
                height: 40
                color: index % 2 === 0 ? "#2d2d2d" : "#1e1e1e"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Text {
                        text: modelData
                        color: "#fff"
                        font.pixelSize: 14
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Download"
                        onClicked: protonModel.downloadVersion(modelData)
                    }

                    Button {
                        text: "Delete"
                        onClicked: protonModel.deleteVersion(modelData)
                    }
                }
            }
        }

        // Download progress
        ProgressBar {
            Layout.fillWidth: true
            value: 0
            visible: false
        }
    }
}
