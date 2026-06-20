import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#2d2d2d"
    radius: 4

    property string title: "Conflict Resolver"
    property var conflicts: []
    property var selectedIndices: []

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Text {
            text: root.title
            color: "#fff"
            font.pixelSize: 16
            font.bold: true
            Layout.fillWidth: true
        }

        Text {
            text: "Merging " + root.conflicts.length + " presets — " + root.conflicts.length + " conflicts found"
            color: "#888"
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        // Conflict list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.conflicts
            delegate: Rectangle {
                width: ListView.view.width
                height: 60
                color: "#1e1e1e"
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    Text {
                        text: model.key
                        color: "#4a90d9"
                        font.pixelSize: 14
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: model.values
                        RadioButton {
                            text: model.label + " ← " + model.source
                            checked: model.selected
                            onClicked: {
                                // Handle selection
                            }
                        }
                    }
                }
            }
        }

        // Footer
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                onClicked: root.cancelled()
            }

            Button {
                text: "Apply with selections"
                background: Rectangle {
                    color: "#2d8c2d"
                    radius: 4
                }
                onClicked: root.applied(root.selectedIndices)
            }
        }
    }

    signal cancelled()
    signal applied(var indices)
}
