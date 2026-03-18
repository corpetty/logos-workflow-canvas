# logos-workflow-canvas

A native [Logos](https://logos.co) UI module providing a visual workflow editor. Built with Qt/QML and [QuickQanava](https://github.com/cneben/QuickQanava) for graph rendering, it lets you drag, drop, and wire together Logos module operations into executable pipelines.

This is **Module 3** of the [Logos Legos](https://github.com/corpetty/logos-legos) v2 native architecture — the authoring layer. Workflows created here are serialized to JSON and executed by the engine independently; the canvas is not required at runtime.

---

## What It Does

The canvas is loaded by `logos-app` as a UI plugin. It queries the registry for the current node palette, renders a graph editor where users can place and connect nodes, and calls the engine to execute workflows interactively. The graph state is serialized to JSON for save/load and for handing off to the scheduler for deployment.

---

## Architecture

```
logos-workflow-registry   (palette source — queries getNodeTypeDefinitions())
        │
        ▼
logos-workflow-canvas     ← you are here
        │
        ├── renders node graph via QuickQanava
        ├── serializes graph → JSON (WorkflowSerializer)
        └──▶ logos-workflow-engine  (calls executeWorkflow for interactive runs)
```

The canvas has no runtime dependency — once a workflow is serialized to JSON it can be handed to the engine or scheduler without the canvas being loaded.

---

## Component Overview

### C++ layer (`src/`)

| File | Purpose |
|---|---|
| `CanvasComponent.h/.cpp` | `IComponent` factory — entry point loaded by `logos-app`, creates the `CanvasWidget` |
| `CanvasWidget.h/.cpp` | Top-level Qt widget hosting the QML canvas, toolbar, and sidebar |
| `WorkflowGraph.h/.cpp` | Extends `qan::Graph` — inserts typed nodes, manages port connections, owns serialize/load |
| `WorkflowSerializer.h/.cpp` | Pure-static: converts `WorkflowGraph ↔ workflow JSON` |
| `nodes/ModuleMethodNode` | Node type for Logos module method calls |
| `nodes/UtilityNode` | String, Number, Boolean, JSON, Display, Template nodes |
| `nodes/ControlFlowNode` | IfElse, Switch, ForEach, Merge, TryCatch, Retry, Fallback nodes |
| `nodes/TransformNode` | ArrayMap, Filter, ObjectPick, ObjectMerge, CodeExpression, HttpRequest nodes |
| `nodes/TriggerNode` | Webhook, Timer, ManualTrigger nodes |

### QML layer (`qml/`)

| File | Purpose |
|---|---|
| `WorkflowCanvas.qml` | Root QML item — hosts the `qan::GraphView`, node palette sidebar, and execution status bar |
| `delegates/ModuleNodeDelegate.qml` | Visual template for module method nodes (color-coded by module, LIVE/MOCK badge, status border) |
| `delegates/ControlFlowDelegate.qml` | Visual template for control flow nodes |
| `delegates/TransformNodeDelegate.qml` | Visual template for transform nodes |
| `delegates/TriggerNodeDelegate.qml` | Visual template for trigger nodes |
| `delegates/UtilityNodeDelegate.qml` | Visual template for utility/constant nodes |

### Vendor

QuickQanava is included as a git submodule at `vendor/QuickQanava`.

---

## Project Structure

```
logos-workflow-canvas/
├── src/
│   ├── CanvasComponent.h/.cpp      # IComponent entry point
│   ├── CanvasWidget.h/.cpp         # Top-level widget
│   ├── WorkflowGraph.h/.cpp        # qan::Graph extension
│   ├── WorkflowSerializer.h/.cpp   # JSON ↔ graph serialization
│   └── nodes/
│       ├── ModuleMethodNode.h/.cpp
│       ├── UtilityNode.h/.cpp
│       ├── ControlFlowNode.h/.cpp
│       ├── TransformNode.h/.cpp
│       └── TriggerNode.h/.cpp
├── qml/
│   ├── WorkflowCanvas.qml
│   └── delegates/
│       ├── ModuleNodeDelegate.qml
│       ├── ControlFlowDelegate.qml
│       ├── TransformNodeDelegate.qml
│       ├── TriggerNodeDelegate.qml
│       └── UtilityNodeDelegate.qml
├── interfaces/
│   └── IComponent.h                # logos-app UI plugin interface
├── resources/
│   └── canvas_resources.qrc
├── vendor/
│   └── QuickQanava/                # git submodule
├── cmake/
│   └── FindOpenGL.cmake
├── CMakeLists.txt
├── flake.nix
├── module.yaml                     # type: ui, depends on: registry + engine
└── metadata.json
```

---

## Building

### With Nix (recommended)

```bash
# Initialize the QuickQanava submodule first
git submodule update --init --recursive

nix build
```

### With CMake

Requires Qt6 with Quick, QuickWidgets, and QuickControls2.

```bash
git submodule update --init --recursive

mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/logos-core
make -j$(nproc)
```

Output: `build/workflow_canvas.so`

> **Note**: The canvas compiles to ~4.7MB due to QuickQanava. It is currently awaiting integration testing in `logos-app`.

---

## Workflow JSON Format

`WorkflowSerializer` produces a JSON object compatible with both the engine and the v1 bridge format:

```json
{
  "name": "My Workflow",
  "nodes": [
    {
      "id": "node-1",
      "type": "module_method",
      "nodeTypeId": "Chat/sendMessage",
      "module": "logos_chat_module",
      "method": "sendMessage",
      "properties": {},
      "inputs": [{"id": "message", "type": "string"}],
      "outputs": [{"id": "result", "type": "object"}],
      "x": 100, "y": 200
    }
  ],
  "edges": [
    {
      "id": "edge-1",
      "sourceNode": "node-0",
      "sourcePort": "value",
      "targetNode": "node-1",
      "targetPort": "message"
    }
  ]
}
```

---

## Related Modules

| Module | Role |
|---|---|
| [logos-workflow-registry](https://github.com/corpetty/logos-workflow-registry) | Supplies the node palette via `getNodeTypeDefinitions()` |
| [logos-workflow-engine](https://github.com/corpetty/logos-workflow-engine) | Executes the serialized graph from this canvas |
| [logos-workflow-scheduler](https://github.com/corpetty/logos-workflow-scheduler) | Deploys and automates workflows authored here |
| [logos-legos](https://github.com/corpetty/logos-legos) | Parent repo with v1 prototype and full architecture docs |

See [logos-legos/docs/NATIVE-ARCHITECTURE.md](https://github.com/corpetty/logos-legos/blob/main/docs/NATIVE-ARCHITECTURE.md) for the full v2 design.

---

## License

MIT
