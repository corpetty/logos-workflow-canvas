import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QuickQanava as Qan
import WorkflowCanvas 1.0

/**
 * Main workflow canvas — the root QML component loaded by CanvasWidget.
 * Contains the graph view, toolbar, node palette sidebar, and execution panel.
 */
Rectangle {
    id: root
    color: "#0d1117"

    // The canvasWidget C++ object is injected via setContextProperty
    // canvasWidget.nodeTypeDefinitions — array of node type defs
    // canvasWidget.executeWorkflow(json) — run the workflow
    // canvasWidget.saveWorkflow(name, json) — persist to disk
    // canvasWidget.loadWorkflow(name) — load from disk

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Toolbar ──────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "#161b22"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                // Brand
                RowLayout {
                    spacing: 4
                    Rectangle {
                        width: 28; height: 28; radius: 6
                        color: "#7e57c2"
                        Text {
                            anchors.centerIn: parent
                            text: "L"; color: "white"
                            font.bold: true; font.pixelSize: 16
                        }
                    }
                    Text {
                        text: "LOGOS LEGOS"
                        color: "#c9d1d9"
                        font.bold: true; font.pixelSize: 13
                        font.letterSpacing: 1.5
                    }
                }

                Rectangle { width: 1; Layout.fillHeight: true; color: "#30363d" }

                // Sidebar toggle
                ToolButton {
                    text: "☰"
                    font.pixelSize: 18
                    onClicked: sidebar.visible = !sidebar.visible
                    palette.buttonText: "#c9d1d9"
                }

                Rectangle { width: 1; Layout.fillHeight: true; color: "#30363d" }

                ToolButton { text: "✕ Clear"; palette.buttonText: "#c9d1d9"
                    onClicked: graph.clearGraph() }
                ToolButton { text: "⌖ Fit"; palette.buttonText: "#c9d1d9"
                    onClicked: graphView.fitContentInView() }

                Rectangle { width: 1; Layout.fillHeight: true; color: "#30363d" }

                // Run button
                Button {
                    text: "▶ Run"
                    palette.button: "#238636"
                    palette.buttonText: "white"
                    font.bold: true
                    z: 10
                    onClicked: {
                        console.log("[canvas] Run clicked, serializing...")
                        var json = graph.serializeToJson()
                        console.log("[canvas] Workflow JSON length:", json.length)
                        console.log("[canvas] Workflow JSON:", json)
                        canvasWidget.executeWorkflow(json)
                    }
                }

                Item { Layout.fillWidth: true }

                // Save/Load
                RowLayout {
                    spacing: 4
                    TextField {
                        id: saveNameField
                        placeholderText: "Workflow name"
                        Layout.preferredWidth: 150
                        color: "#c9d1d9"
                        background: Rectangle { color: "#0d1117"; border.color: "#30363d"; radius: 4 }
                    }
                    ToolButton { text: "💾 Save"; palette.buttonText: "#c9d1d9"
                        z: 10
                        onClicked: {
                            if (saveNameField.text.length > 0) {
                                canvasWidget.saveWorkflow(saveNameField.text, graph.serializeToJson())
                                saveConfirm.text = "Saved: " + saveNameField.text
                                saveConfirm.visible = true
                                saveConfirmTimer.restart()
                            } else {
                                saveConfirm.text = "Enter a name first"
                                saveConfirm.visible = true
                                saveConfirmTimer.restart()
                            }
                        }
                    }
                    Text {
                        id: saveConfirm
                        visible: false
                        color: "#4caf50"
                        font.pixelSize: 11
                        Timer {
                            id: saveConfirmTimer
                            interval: 3000
                            onTriggered: saveConfirm.visible = false
                        }
                    }
                }

                // Connection status
                Rectangle {
                    width: statusLabel.width + 16; height: 24; radius: 12
                    color: canvasWidget.connectionStatus === "connected" ? "#238636" : "#6e7681"
                    Text {
                        id: statusLabel
                        anchors.centerIn: parent
                        text: canvasWidget.connectionStatus === "connected" ? "CONNECTED" : "OFFLINE"
                        color: "white"; font.pixelSize: 10; font.bold: true
                    }
                }
            }
        }

        // ── Main area: sidebar + graph canvas ────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ── Node Palette Sidebar ──────────────────────────────────
            Rectangle {
                id: sidebar
                Layout.preferredWidth: 240
                Layout.fillHeight: true
                color: "#161b22"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Text {
                        text: "Node Palette"
                        color: "#c9d1d9"
                        font.bold: true; font.pixelSize: 14
                    }

                    TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: "Search nodes..."
                        color: "#c9d1d9"
                        background: Rectangle { color: "#0d1117"; border.color: "#30363d"; radius: 4 }
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 2

                        model: {
                            var defs = canvasWidget.nodeTypeDefinitions
                            var query = searchField.text.toLowerCase()
                            if (query.length === 0) return defs
                            return defs.filter(function(d) {
                                return d.nodeTypeId.toLowerCase().indexOf(query) >= 0 ||
                                       d.methodDisplayName.toLowerCase().indexOf(query) >= 0
                            })
                        }

                        delegate: Rectangle {
                            width: parent ? parent.width : 200
                            height: 36
                            radius: 4
                            color: paletteMouseArea.containsMouse ? "#21262d" : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 4
                                spacing: 6

                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: modelData.color || "#607d8b"
                                }

                                Text {
                                    text: modelData.methodDisplayName || modelData.nodeTypeId
                                    color: "#c9d1d9"
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: modelData.displayName || ""
                                    color: "#6e7681"
                                    font.pixelSize: 10
                                }
                            }

                            MouseArea {
                                id: paletteMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onDoubleClicked: {
                                    // Insert node at center of canvas
                                    graph.insertWorkflowNode(modelData)
                                }
                            }
                        }
                    }
                }
            }

            Rectangle { width: 1; Layout.fillHeight: true; color: "#30363d" }

            // ── Graph Canvas ──────────────────────────────────────────
            Qan.GraphView {
                id: graphView
                Layout.fillWidth: true
                Layout.fillHeight: true

                navigable: true
                zoomMin: 0.25
                zoomMax: 4.0
                zoomIncrement: 0.05

                graph: WorkflowGraph {
                    id: graph
                    objectName: "WorkflowGraph"
                    connectorEnabled: true
                    connectorEdgeColor: "#58a6ff"
                    connectorColor: "#58a6ff"
                    selectionPolicy: Qan.AbstractGraph.SelectOnClick
                }

                // Delete key removes selected nodes/edges
                Keys.onDeletePressed: {
                    var sel = graph.selectedNodes
                    for (var i = sel.length - 1; i >= 0; --i)
                        graph.removeNode(sel[i])
                    var edgeSel = graph.selectedEdges
                    for (var j = edgeSel.length - 1; j >= 0; --j)
                        graph.removeEdge(edgeSel[j])
                }
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Backspace) {
                        var sel2 = graph.selectedNodes
                        for (var i = sel2.length - 1; i >= 0; --i)
                            graph.removeNode(sel2[i])
                        var edgeSel2 = graph.selectedEdges
                        for (var j = edgeSel2.length - 1; j >= 0; --j)
                            graph.removeEdge(edgeSel2[j])
                    }
                }
                focus: true

                // Right-click on edge to delete
                onEdgeRightClicked: function(edge, pos) {
                    if (edge)
                        graph.removeEdge(edge)
                }

                // Dark background
                Rectangle {
                    z: -1
                    anchors.fill: parent
                    color: "#0d1117"

                    // Grid pattern
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.strokeStyle = "#161b22"
                            ctx.lineWidth = 1
                            var gridSize = 20
                            for (var x = 0; x < width; x += gridSize) {
                                ctx.beginPath()
                                ctx.moveTo(x, 0)
                                ctx.lineTo(x, height)
                                ctx.stroke()
                            }
                            for (var y = 0; y < height; y += gridSize) {
                                ctx.beginPath()
                                ctx.moveTo(0, y)
                                ctx.lineTo(width, y)
                                ctx.stroke()
                            }
                        }
                    }
                }
            }
        }

        // ── Execution Result Panel ──────────────────────────────────────
        Rectangle {
            id: resultPanel
            Layout.fillWidth: true
            Layout.preferredHeight: canvasWidget.lastExecutionResult ? Math.min(150, resultOutput.implicitHeight + 32) : 0
            color: "#161b22"
            visible: canvasWidget.lastExecutionResult !== ""
            clip: true

            Behavior on Layout.preferredHeight { NumberAnimation { duration: 200 } }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Execution Result"
                        color: "#c9d1d9"
                        font.bold: true
                        font.pixelSize: 11
                    }
                    Item { Layout.fillWidth: true }
                    ToolButton {
                        text: "✕"
                        palette.buttonText: "#6e7681"
                        font.pixelSize: 12
                        z: 10
                        onClicked: canvasWidget.clearLastExecutionResult()
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    Text {
                        id: resultOutput
                        width: resultPanel.width - 12
                        text: {
                            try {
                                var obj = JSON.parse(canvasWidget.lastExecutionResult || "{}")
                                return JSON.stringify(obj, null, 2)
                            } catch(e) {
                                return canvasWidget.lastExecutionResult || ""
                            }
                        }
                        color: "#88ccff"
                        font.family: "monospace"
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                    }
                }
            }
        }

        // ── Status Bar ───────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: "#161b22"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 12

                Text {
                    text: "Nodes: " + (graph && graph.nodes ? graph.nodes.count : 0)
                    color: "#6e7681"; font.pixelSize: 11
                }
                Text {
                    text: "Status: " + canvasWidget.connectionStatus
                    color: "#6e7681"; font.pixelSize: 11
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "Logos Legos v2.0"
                    color: "#6e7681"; font.pixelSize: 11
                }
            }
        }
    }
}
