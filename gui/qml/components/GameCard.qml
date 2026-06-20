import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#2d2d2d"
    radius: 4

    property string gameId: ""
    property string name: ""
    property string executable: ""
    property string protonVersion: ""
    property int lastPlayed: 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        // Cover art placeholder
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            color: "#1e1e1e"
            radius: 4

            Image {
                anchors.fill: parent
                anchors.margins: 2
                source: "" // artwork path
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            Text {
                anchors.centerIn: parent
                text: "No artwork"
                color: "#666"
                visible: !image.source
            }
        }

        // Game name
        Text {
            Layout.fillWidth: true
            text: name
            color: "#fff"
            font.bold: true
            font.pixelSize: 14
            elide: Text.ElideRight
        }

        // Proton version
        Text {
            Layout.fillWidth: true
            text: protonVersion
            color: "#888"
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        // Last played
        Text {
            Layout.fillWidth: true
            text: lastPlayed > 0 ? formatLastPlayed(lastPlayed) : "Never played"
            color: "#666"
            font.pixelSize: 11
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

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    signal clicked()
}
