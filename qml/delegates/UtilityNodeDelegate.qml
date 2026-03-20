import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QuickQanava as Qan

/**
 * Visual delegate for utility nodes (String, Number, Boolean, JSON, Display, Template).
 * Provides inline editing widgets for constant values.
 */
Qan.NodeItem {
    id: utilityNodeItem
    width: 180
    height: Math.max(60, contentColumn.implicitHeight + 16)

    // Background (behind ports)
    Rectangle {
        anchors.fill: parent
        z: -1
        radius: 6
        color: "#1c2333"
        border.color: "#607d8b"
        border.width: 1.5
        antialiasing: true
        opacity: 0.95
    }

    // Interactive content (above NodeItem internals so text fields work)
    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4
        z: 5

        // Header
        Text {
            text: node ? node.label : ""
            color: "#8b949e"
            font.bold: true
            font.pixelSize: 11
        }

        // Inline value editor for constant nodes
        Loader {
            Layout.fillWidth: true
            active: node !== null
            sourceComponent: {
                if (!node) return null
                switch (node.subtype) {
                    case "String":
                    case "string_constant":
                        return stringEditor
                    case "Number":
                    case "number_constant":
                        return numberEditor
                    case "Boolean":
                    case "boolean_constant":
                        return boolEditor
                    case "Template":
                        return templateEditor
                    case "Display":
                    case "display":
                        return displayViewer
                    default:
                        return null
                }
            }
        }
    }

    Component {
        id: stringEditor
        TextField {
            text: node ? (node.propertyValue || "") : ""
            placeholderText: "Value..."
            color: "#c9d1d9"
            font.pixelSize: 11
            background: Rectangle { color: "#0d1117"; border.color: "#30363d"; radius: 3 }
            onTextChanged: {
                if (node) {
                    node.setPropertyValue("value", text)
                    console.log("[canvas] String value set to:", text, "readback:", node.getProperty("value"))
                }
            }
        }
    }

    Component {
        id: numberEditor
        SpinBox {
            value: node ? (node.propertyValue || 0) : 0
            from: -999999
            to: 999999
            onValueChanged: if (node) node.setPropertyValue("value", value)
        }
    }

    Component {
        id: boolEditor
        Switch {
            checked: node ? (node.propertyValue || false) : false
            text: checked ? "true" : "false"
            onCheckedChanged: if (node) node.setPropertyValue("value", checked)
        }
    }

    Component {
        id: templateEditor
        TextField {
            text: node ? (node.getProperty("template") || "{key}") : "{key}"
            placeholderText: "{key} template..."
            color: "#c9d1d9"
            font.pixelSize: 11
            font.family: "monospace"
            background: Rectangle { color: "#0d1117"; border.color: "#30363d"; radius: 3 }
            onTextChanged: if (node) node.setPropertyValue("template", text)
        }
    }

    Component {
        id: displayViewer
        Rectangle {
            implicitHeight: displayText.implicitHeight + 12
            radius: 3
            color: "#0d1117"
            border.color: "#30363d"

            Text {
                id: displayText
                anchors.fill: parent
                anchors.margins: 6
                text: {
                    if (!node) return "(waiting for input)"
                    var v = node.propertyValue
                    if (v === undefined || v === null || v === "")
                        return "(waiting for input)"
                    return typeof v === "object" ? JSON.stringify(v, null, 2) : String(v)
                }
                color: node && node.propertyValue ? "#88ccff" : "#6e7681"
                font.family: "monospace"
                font.pixelSize: 11
                font.italic: !node || !node.propertyValue
                wrapMode: Text.Wrap
            }
        }
    }
}
