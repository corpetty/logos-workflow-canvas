import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QuickQanava 2.0 as Qan

/**
 * Visual delegate for control flow nodes (If/Else, Switch, ForEach, etc.)
 * Features a FLOW badge and distinctive purple styling.
 * Error-catching nodes (Try/Catch, Retry, Fallback) show a warning icon.
 */
Qan.NodeItem {
    id: flowNodeItem
    width: 200
    height: 70

    Qan.RectNodeTemplate {
        nodeItem: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 4

            RowLayout {
                spacing: 6

                // FLOW badge
                Rectangle {
                    width: flowLabel.width + 8
                    height: 16; radius: 3
                    color: "#7e57c2"

                    Text {
                        id: flowLabel
                        anchors.centerIn: parent
                        text: "FLOW"
                        color: "white"
                        font.pixelSize: 9
                        font.bold: true
                    }
                }

                // Warning icon for error-catching nodes
                Text {
                    visible: node && node.isErrorCatch
                    text: "⚠"
                    font.pixelSize: 14
                    color: "#ffa726"
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

    // Execution status border
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: 8
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
