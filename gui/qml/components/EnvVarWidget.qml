import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "transparent"

    property string key: ""
    property string value: ""
    property string widgetType: "text" // toggle, slider, multiselect, dropdown, text
    property string label: ""
    property string description: ""
    property bool locked: false
    property bool enabled: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        // Label row
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: label || key
                color: locked ? "#666" : "#fff"
                font.pixelSize: 13
                elide: Text.ElideRight
            }

            Item { Layout.fillWidth: true }

            // Lock icon
            Text {
                text: locked ? "🔒" : ""
                visible: locked
                font.pixelSize: 12
            }

            // Category badge
            Text {
                text: categoryId
                color: "#4a90d9"
                font.pixelSize: 10
                visible: categoryId && !locked
                background: Rectangle {
                    color: "#4a90d922"
                    radius: 2
                }
                padding: 2
            }
        }

        // Value widget
        Item {
            Layout.fillWidth: true
            height: 30

            // Toggle switch
            Switch {
                anchors.fill: parent
                checked: value === "1" || value === "true"
                enabled: !locked
                visible: widgetType === "toggle"
                onCheckedChanged: root.valueChanged(key, checked ? "1" : "0")
            }

            // Text field
            TextField {
                anchors.fill: parent
                text: value
                enabled: !locked
                visible: widgetType === "text"
                onTextModified: root.valueChanged(key, text)
            }

            // Dropdown
            ComboBox {
                anchors.fill: parent
                model: ["disable", "fps", "frametimes", "devinfo"]
                enabled: !locked
                visible: widgetType === "dropdown"
                onActivated: root.valueChanged(key, currentText)
            }

            // Slider
            Slider {
                anchors.fill: parent
                value: parseFloat(value) || 0
                enabled: !locked
                visible: widgetType === "slider"
                onValueChanged: root.valueChanged(key, String(value))
            }

            // Multiselect (simplified)
            Text {
                anchors.fill: parent
                anchors.margins: 4
                text: value || "Select..."
                color: "#888"
                font.pixelSize: 12
                visible: widgetType === "multiselect"
                elide: Text.ElideRight
            }
        }

        // Description
        Text {
            Layout.fillWidth: true
            text: description
            color: "#666"
            font.pixelSize: 11
            visible: description
            elide: Text.ElideRight
        }
    }

    signal valueChanged(string key, string value)
}
