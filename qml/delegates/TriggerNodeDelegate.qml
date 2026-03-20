import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QuickQanava as Qan

/**
 * Visual delegate for trigger nodes (Webhook, Timer, Manual).
 * Orange-themed with a TRIGGER badge and lightning bolt icon.
 */
Qan.NodeItem {
    id: triggerNodeItem
    width: 200
    height: 70

    Rectangle {
        anchors.fill: parent
        z: -1
        radius: 6
        color: "#2e2214"
        border.color: "#ffa726"
        border.width: 2
        antialiasing: true
        opacity: 0.95

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 4

            RowLayout {
                spacing: 6

                // Lightning bolt
                Text {
                    text: "⚡"
                    font.pixelSize: 14
                }

                Rectangle {
                    width: triggerLabel.width + 8
                    height: 16; radius: 3
                    color: "#ffa726"

                    Text {
                        id: triggerLabel
                        anchors.centerIn: parent
                        text: "TRIGGER"
                        color: "#1a1a2e"
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
}
