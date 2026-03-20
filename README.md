# logos-workflow-canvas

A native [Logos](https://logos.co) UI module providing a visual workflow editor. Built with Qt/QML and [QuickQanava](https://github.com/cneben/QuickQanava) for graph rendering, it lets you drag, drop, and wire together Logos module operations into executable pipelines.

This is **Module 3** of the [Logos Legos](https://github.com/corpetty/logos-legos) v2 native architecture -- the authoring layer. Workflows created here are serialized to JSON and executed by the engine independently; the canvas is not required at runtime.

---

## What It Does

The canvas is loaded by `logos-app` as a UI plugin (implementing `IComponent`). It queries the registry for the current node palette, renders a graph editor where users can place and connect nodes, and calls the engine to execute workflows interactively. The graph state is serialized to JSON for save/load and for handing off to the scheduler for deployment.

---

## Architecture

```
logos-workflow-registry   (palette source -- queries getNodeTypeDefinitions())
        |
        v
logos-workflow-canvas     <-- you are here
        |
        |-- renders node graph via QuickQanava (shared library)
        |-- serializes graph -> JSON (WorkflowGraph::serializeToJson)
        |-> logos-workflow-engine  (calls executeWorkflow for interactive runs)
```

The canvas has no runtime dependency -- once a workflow is serialized to JSON it can be handed to the engine or scheduler without the canvas being loaded.

---

## C++ Public API

### CanvasWidget (QML-exposed properties)

| Property | Type | Description |
|---|---|---|
| `nodeTypeDefinitions` | `QJsonArray` | Node palette from the registry (read-only) |
| `connectionStatus` | `QString` | `"connected"`, `"no_api"`, or `"disconnected"` |
| `executionHistory` | `QJsonArray` | Recent execution results |
| `lastExecutionResult` | `QString` | JSON string of the most recent execution result |

### CanvasWidget (public slots, callable from QML)

| Slot | Signature | Description |
|---|---|---|
| `executeWorkflow` | `void executeWorkflow(const QString& workflowJson)` | Send workflow JSON to the engine, update result panel |
| `refreshNodeTypes` | `void refreshNodeTypes()` | Re-query the registry for node type definitions |
| `saveWorkflow` | `void saveWorkflow(const QString& name, const QString& workflowJson)` | Save to `~/.local/share/Logos/LogosApp/workflows/<name>.json` |
| `loadWorkflow` | `QString loadWorkflow(const QString& name)` | Load a saved workflow JSON by name |
| `listSavedWorkflows` | `QStringList listSavedWorkflows()` | List saved workflow names |
| `deleteWorkflow` | `void deleteWorkflow(const QString& name)` | Delete a saved workflow |
| `deployWorkflow` | `void deployWorkflow(const QString& id, const QString& json)` | Deploy to the scheduler |
| `clearLastExecutionResult` | `void clearLastExecutionResult()` | Clear the result panel |

### CanvasWidget (signals)

| Signal | Description |
|---|---|
| `executionStarted(executionId)` | Workflow execution began |
| `executionCompleted(executionId, result)` | Workflow execution finished |
| `nodeExecuting(executionId, nodeId)` | A specific node started executing |
| `nodeCompleted(executionId, nodeId, result)` | A specific node finished |
| `nodeFailed(executionId, nodeId, error)` | A specific node errored |

### WorkflowGraph (Q_INVOKABLE, callable from QML)

| Method | Description |
|---|---|
| `insertWorkflowNode(nodeTypeDef)` | Create a typed node from a registry node definition |
| `serializeToJson()` | Serialize the entire graph to workflow JSON |
| `loadFromJson(json)` | Load a workflow from JSON, replacing the current graph |
| `clearGraph()` | Remove all nodes and edges |

---

## Node Types

Each node type is a C++ class extending `qan::Node` with a QML delegate for visual rendering.

| Class | Category | Properties | Delegate |
|---|---|---|---|
| `ModuleMethodNode` | module_method | `moduleName`, `methodName`, `nodeTypeId`, `nodeColor`, `isLive`, `executionStatus`, `executionResult` | Blue header with LIVE/MOCK badge, module name subtitle, result preview |
| `UtilityNode` | utility | `nodeTypeId`, `subtype`, `propertyValue` | Inline editors: TextField (String/Template), SpinBox (Number), Switch (Boolean), Display viewer |
| `ControlFlowNode` | control_flow | `nodeTypeId`, `subtype`, `isErrorCatch`, `executionStatus` | Purple FLOW badge, warning icon for error-catching nodes |
| `TransformNode` | transform | `nodeTypeId`, `subtype`, `executionStatus` | Teal TRANSFORM badge |
| `TriggerNode` | trigger | `nodeTypeId`, `subtype` | Orange TRIGGER badge with lightning bolt |

---

## UI Features

- **Node palette sidebar** with search filtering -- double-click to add nodes
- **Drag-and-drop** node positioning on the canvas
- **Port-based edge connections** -- drag from output port to input port
- **Visual connector** enabled (`connectorEnabled: true`) with blue edge color
- **Keyboard shortcuts**: Delete/Backspace to remove selected nodes/edges
- **Right-click** on edges to delete them
- **Run button** executes the workflow and shows results in a bottom panel
- **Save/Load** with confirmation feedback (saves to user data directory)
- **Execution result panel** with JSON output, dismissible with X button
- **Connection status** indicator (CONNECTED/OFFLINE badge)
- **Dark theme** with grid background (`#0d1117`)

---

## QuickQanava Packaging

QuickQanava is built as a **shared Nix derivation** (not vendored statically). The `flake.nix` defines a `quickqanavaPackage` that:

1. Fetches QuickQanava from `github:cneben/QuickQanava`
2. Patches the CMake to build as a **shared library** (removes `STATIC` from `qt_add_qml_module`)
3. Provides a stub `FindOpenGL.cmake` for headless builds
4. Installs: `lib/libQuickQanava.so`, `lib/qt-6/qml/QuickQanava/` (QML module with qmldir), `include/QuickQanava/` (headers)
5. Fixes RPATH so the QML plugin loads the correct `libQuickQanava.so`

The canvas derivation links against this shared library and bundles both the `.so` and QML module in its output. At runtime, `CanvasWidget::setupUI()` adds the QML module path to the engine's import paths so `import QuickQanava` resolves.

For local CMake builds, the vendored submodule at `vendor/QuickQanava/` is still supported as a fallback via `add_subdirectory()`.

---

## IComponent Integration

The canvas is loaded by `logos-app` via `QPluginLoader`. The entry point is `CanvasComponent`:

```cpp
class CanvasComponent : public QObject, public IComponent {
    Q_PLUGIN_METADATA(IID IComponent_iid FILE "metadata.json")
    QWidget* createWidget(LogosAPI* logosAPI) override;
    void destroyWidget(QWidget* widget) override;
};
```

When `createWidget` is called, it:
1. Initializes `QuickQanava::initialize(engine)` to register default styles and edge path components
2. Registers custom QML types (`WorkflowGraph`, `ModuleMethodNode`, etc.) via `qmlRegisterType`
3. Sets up QML import paths for the QuickQanava shared module
4. Loads `qrc:/qml/WorkflowCanvas.qml`
5. Connects to the registry and engine modules via LogosAPI

---

## Component Overview

### C++ layer (`src/`)

| File | Purpose |
|---|---|
| `CanvasComponent.h/.cpp` | `IComponent` factory -- entry point loaded by `logos-app` |
| `CanvasWidget.h/.cpp` | Top-level widget: QML engine setup, QuickQanava init, type registration, module communication |
| `WorkflowGraph.h/.cpp` | Extends `qan::Graph` -- typed node insertion via `insertNode<T>()`, port management, serialization |
| `nodes/ModuleMethodNode` | Node for Logos module method calls, with custom delegate and style |
| `nodes/UtilityNode` | Constant/utility nodes with inline editing, `setPropertyValue`/`getProperty` API |
| `nodes/ControlFlowNode` | Branching/looping nodes with error-catch support |
| `nodes/TransformNode` | Data transformation nodes |
| `nodes/TriggerNode` | Workflow trigger nodes (webhook, timer, manual) |

### QML layer (`qml/`)

| File | Purpose |
|---|---|
| `WorkflowCanvas.qml` | Root item: GraphView, toolbar, sidebar, result panel, status bar |
| `delegates/ModuleNodeDelegate.qml` | Module method node visual (dark bg `#1a1a2e`, colored border/header) |
| `delegates/UtilityNodeDelegate.qml` | Utility node visual with inline editors (z:5 for interactivity) |
| `delegates/ControlFlowDelegate.qml` | Control flow node visual (purple `#7e57c2` border) |
| `delegates/TransformNodeDelegate.qml` | Transform node visual (teal `#26a69a` border) |
| `delegates/TriggerNodeDelegate.qml` | Trigger node visual (orange `#ffa726` border) |

---

## Workflow JSON Format

`WorkflowGraph::serializeToJson()` produces:

```json
{
  "name": "Untitled Workflow",
  "version": "2.0",
  "nodes": [
    {
      "id": "392364336",
      "type": "utility",
      "subtype": "String",
      "nodeTypeId": "Utility/String",
      "label": "String",
      "properties": { "value": "hello world" },
      "ports": {
        "inputs": [],
        "outputs": [{ "id": "output", "label": "Value" }]
      },
      "position": { "x": 100.0, "y": 200.0 }
    }
  ],
  "edges": [
    {
      "id": "edge_1",
      "sourceNode": "392364336",
      "sourcePort": "output",
      "targetNode": "394680416",
      "targetPort": "value"
    }
  ]
}
```

Node IDs are generated from memory addresses (unique per session). Port IDs match the registry's port definitions.

---

## Saved Workflows

Workflows are saved to: `~/.local/share/Logos/LogosApp/workflows/<name>.json`

---

## Building

### With Nix (recommended)

```bash
nix build
```

Output: `result/lib/workflow_canvas.so`, `result/lib/libQuickQanava.so`, `result/lib/qt-6/qml/QuickQanava/`

### With CMake (local development)

Requires Qt6 with Quick, QuickWidgets, and QuickControls2. Initialize the QuickQanava submodule first.

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake .. -GNinja
ninja
```

---

## Related Modules

| Module | Role |
|---|---|
| [logos-workflow-registry](https://github.com/corpetty/logos-workflow-registry) | Supplies the node palette via `getNodeTypeDefinitions()` |
| [logos-workflow-engine](https://github.com/corpetty/logos-workflow-engine) | Executes the serialized graph |
| [logos-workflow-scheduler](https://github.com/corpetty/logos-workflow-scheduler) | Deploys and automates workflows |

---

## License

MIT
