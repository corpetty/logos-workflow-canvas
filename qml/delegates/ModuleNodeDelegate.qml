import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QuickQanava as Qan

/**
 * Visual delegate for module method nodes.
 * Shows: colored header with method name, LIVE/MOCK badge,
 * execution status border, and result preview.
 */
Qan.NodeItem {
    id: moduleNodeItem
    width: 220
    height: Math.max(80, contentColumn.implicitHeight + 20)

    Qan.RectNodeTemplate {
        nodeItem: parent

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 2
            spacing: 0

            // Header bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                radius: 4
                color: node ? node.nodeColor : "#4a9eff"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 6

                    Text {
                        text: node ? node.label : ""
                        color: "white"
                        font.bold: true
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    // LIVE / MOCK badge
                    Rectangle {
                        width: badgeText.width + 10
                        height: 16
                        radius: 3
                        color: (node && node.isLive) ? "#1a7f37" : "#6e4d00"
                        visible: node !== null

                        Text {
                            id: badgeText
                            anchors.centerIn: parent
                            text: (node && node.isLive) ? "LIVE" : "MOCK"
                            color: "white"
                            font.pixelSize: 9
                            font.bold: true
                        }
                    }
                }
            }

            // Module name subtitle
            Text {
                Layout.leftMargin: 8
                Layout.topMargin: 4
                text: node ? node.moduleName : ""
                color: "#6e7681"
                font.pixelSize: 10
            }

            // Result preview (after execution)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: resultText.implicitHeight + 8
                Layout.margins: 4
                radius: 4
                color: "#0d1117"
                visible: node && node.executionResult !== undefined
                         && node.executionStatus !== "idle"

                Text {
                    id: resultText
                    anchors.fill: parent
                    anchors.margins: 4
                    text: {
                        if (!node || !node.executionResult) return ""
                        var r = node.executionResult
                        return typeof r === "object" ? JSON.stringify(r, null, 2) : String(r)
                    }
                    color: "#88ccff"
                    font.family: "monospace"
                    font.pixelSize: 10
                    wrapMode: Text.Wrap
                    maximumLineCount: 4
                    elide: Text.ElideRight
                }
            }
        }
    }

    // Execution status border overlay
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: 6
        border.width: {
            if (!node) return 0
            return node.executionStatus === "idle" ? 0 : 3
        }
        border.color: {
            if (!node) return "transparent"
            switch (node.executionStatus) {
                case "running": return "#ff8c00"
                case "success": return "#4caf50"
                case "error":   return "#f44336"
                default:        return "transparent"
            }
        }

        // Running animation
        SequentialAnimation on border.color {
            running: node && node.executionStatus === "running"
            loops: Animation.Infinite
            ColorAnimation { to: "#ffb74d"; duration: 500 }
            ColorAnimation { to: "#ff8c00"; duration: 500 }
        }
    }
}
