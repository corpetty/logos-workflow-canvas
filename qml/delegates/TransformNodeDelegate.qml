import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QuickQanava as Qan

/**
 * Visual delegate for data transform nodes (ArrayMap, ArrayFilter, etc.)
 * Teal-themed with a TRANSFORM badge.
 */
Qan.NodeItem {
    id: transformNodeItem
    width: 200
    height: 70

    Rectangle {
        anchors.fill: parent
        z: -1
        radius: 6
        color: "#1a2e2e"
        border.color: "#26a69a"
        border.width: 2
        antialiasing: true
        opacity: 0.95

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 4

            RowLayout {
                spacing: 6

                Rectangle {
                    width: transformLabel.width + 8
                    height: 16; radius: 3
                    color: "#26a69a"

                    Text {
                        id: transformLabel
                        anchors.centerIn: parent
                        text: "TRANSFORM"
                        color: "white"
                        font.pixelSize: 9
                        font.bold: true
                    }
                }

                Text {
                    text: node ? node.label : ""
                    color: "#c9d1d9"
                    font.bold: true
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            Text {
                text: node ? node.subtype : ""
                color: "#6e7681"
                font.pixelSize: 10
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        z: -1
        color: "transparent"; radius: 6
        border.width: node && node.executionStatus !== "idle" ? 3 : 0
        border.color: {
            if (!node) return "transparent"
            switch (node.executionStatus) {
                case "running": return "#ff8c00"
                case "success": return "#4caf50"
                case "error":   return "#f44336"
                default:        return "transparent"
            }
        }
    }
}
