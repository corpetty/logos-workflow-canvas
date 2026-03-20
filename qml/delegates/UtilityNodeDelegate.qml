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

    Qan.RectNodeTemplate {
        nodeItem: parent

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 4
            spacing: 4

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
                        default:
                            return null
                    }
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
            onTextChanged: if (node) node.setPropertyValue("value", text)
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
}
