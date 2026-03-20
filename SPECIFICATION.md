# logos-workflow-canvas -- Technical Specification

## Overview

Visual workflow editor for the Logos platform. Loaded as a UI plugin by `logos-app`, it provides a node graph canvas powered by QuickQanava where users author workflows by placing nodes and connecting ports. Workflows are serialized to JSON for execution by the engine or deployment via the scheduler.

---

## Functionality

### Core Capabilities

- **Node graph editing**: Drag-and-drop node placement on an infinite, zoomable canvas
- **Port-based connections**: Typed input/output ports with visual edge connectors
- **5 node categories**: module_method, utility, control_flow, transform, trigger
- **Workflow serialization**: Bidirectional JSON conversion (serialize/load)
- **Interactive execution**: Run workflows via the engine and display per-node results
- **Persistence**: Save/load workflows to user data directory
- **Deployment**: Send workflows to the scheduler for automated execution

### Node Insertion

Nodes are created via `WorkflowGraph::insertWorkflowNode(nodeTypeDef)` which dispatches to typed `insertNode<T>()` template calls. QuickQanava manages the visual representation using QML delegate components specific to each node type. Each node type provides a static `delegate()` factory that loads its QML delegate from Qt resources.

### Serialization Format

The `WorkflowGraph::serializeToJson()` method walks all nodes via `get_nodes()`, serializes by dynamic type (`dynamic_cast<T*>`), captures port definitions from `NodeItem::getPorts()`, and collects edges with port bindings. Node IDs are generated from memory addresses (unique per session, not persistent).

### Execution Feedback

After `executeWorkflow()` returns, the canvas:
1. Stores the raw result JSON in `lastExecutionResult` (displayed in result panel)
2. Iterates graph nodes and updates Display nodes' `propertyValue` from `nodeResults`
3. Updates ModuleMethodNode `executionResult` and `executionStatus` properties
4. Emits `executionCompleted` signal

### QML Type Registration

Since the canvas is a plain shared library (not built with `qt_add_qml_module`), all custom QML types are registered explicitly via `qmlRegisterType<T>()` in `CanvasWidget::setupUI()`:
- `WorkflowGraph` (as `WorkflowCanvas.WorkflowGraph`)
- `ModuleMethodNode`, `UtilityNode`, `ControlFlowNode`, `TransformNode`, `TriggerNode`

### QuickQanava Integration

QuickQanava is initialized via `QuickQanava::initialize(engine)` which registers:
- Default node/edge/group styles as QML context properties
- Edge path components (straight, ortho, curved, arrow endpoints)

The QML import path for the QuickQanava shared module is discovered at runtime by checking:
1. `~/.local/share/Logos/LogosAppNix/plugins/workflow_canvas/qt-6/qml/` (non-portable)
2. `~/.local/share/Logos/LogosApp/plugins/workflow_canvas/qt-6/qml/` (portable)
3. `<app-dir>/../lib/qt-6/qml/` (Nix output layout)

---

## Usability

### User Interface

- **Dark theme**: Background `#0d1117`, sidebar `#161b22`, consistent with Logos design system
- **Node palette**: Left sidebar with categorized node list, text search filter, double-click to add
- **Toolbar**: Clear, Fit, Run (green), Save with name field, connection status badge
- **Graph interaction**: Pan (drag background), zoom (scroll wheel, 0.25x--4x), select (click), multi-select
- **Edge creation**: Drag from output port circle to input port circle (blue `#58a6ff` connector)
- **Edge deletion**: Right-click to delete, or select + Delete/Backspace key
- **Node deletion**: Select + Delete/Backspace key
- **Execution feedback**: Bottom result panel with JSON output, green save confirmation (3s auto-hide)

### Node Visuals

Each node category has distinct styling:
- **Module method**: Dark blue bg (`#1a1a2e`), colored border matching `nodeColor`, LIVE/MOCK badge
- **Utility**: Dark blue-gray bg (`#1c2333`), gray border, inline editors for constants
- **Control flow**: Dark bg, purple border (`#7e57c2`), FLOW badge
- **Transform**: Dark teal bg (`#1a2e2e`), teal border (`#26a69a`), TRANSFORM badge
- **Trigger**: Dark orange bg (`#2e2214`), orange border (`#ffa726`), lightning bolt icon

### Inline Editing

Utility nodes provide type-appropriate editors:
- **String**: TextField with placeholder
- **Number**: SpinBox (-999999 to 999999)
- **Boolean**: Switch toggle
- **Template**: Monospace TextField for `{key}` interpolation
- **Display**: Read-only monospace viewer showing received value after execution

---

## Reliability

### Graceful Degradation

- If `workflow_registry` is unavailable: node palette is empty, connection status shows "OFFLINE"
- If `workflow_engine` is unavailable: Run button logs warning, no crash
- If `workflow_scheduler` is unavailable: Deploy logs warning, no crash
- If no `LogosAPI` provided: standalone mode with empty palette

### Error Handling

- QML load errors are logged via `qWarning` with specific error messages
- QPluginLoader failures are caught and reported
- Execution errors from the engine are displayed in the result panel

### Edge Cases

- Empty graph serialization returns valid JSON with empty arrays
- Duplicate node insertion creates separate instances
- Self-loops are prevented by QuickQanava's connector validation

---

## Performance

### Rendering

- QuickQanava uses Qt Quick's hardware-accelerated scene graph
- Node delegates are lightweight QML items (no heavy computation)
- Background grid rendered via Canvas 2D (redrawn on resize only)
- Zoom range 0.25x to 4.0x with 0.05 increments

### Execution

- `serializeToJson()` is O(N+E) where N=nodes, E=edges
- `executeWorkflow()` is synchronous -- blocks the UI thread during engine IPC call
- Node result feedback is O(N) after execution returns
- Execution history capped at 100 entries in memory

### Resource Usage

- `workflow_canvas.so`: ~250KB (canvas code only)
- `libQuickQanava.so`: ~3MB (graph rendering library)
- QML module: ~40 QML files for QuickQanava components

---

## Supportability

### Dependencies

| Dependency | Type | Purpose |
|---|---|---|
| Qt 6 (Core, Quick, Qml, QuickWidgets, QuickControls2, Widgets) | Build + Runtime | UI framework |
| QuickQanava | Build + Runtime | Node graph rendering (shared Nix package) |
| logos-cpp-sdk | Build | LogosAPI headers |
| logos-liblogos | Build | Plugin interface headers |
| logos-workflow-registry | Runtime | Node palette source |
| logos-workflow-engine | Runtime | Workflow execution |

### Build System

- **Nix flake** with custom `stdenv.mkDerivation` (not `mkLogosModule` -- requires QuickQanava packaging)
- QuickQanava built as a separate Nix derivation within the canvas flake
- CMakeLists.txt supports both pre-built (`QUICKQANAVA_ROOT`) and vendored (`add_subdirectory`) QuickQanava
- QML files embedded via Qt resource system (`canvas_resources.qrc`)

### Plugin Architecture

- Implements `IComponent` interface (`createWidget`/`destroyWidget`)
- Loaded by `logos-app` via `QPluginLoader`
- Plugin metadata embedded via `Q_PLUGIN_METADATA(FILE "metadata.json")`
- All inter-module communication via `LogosAPI` (process-isolated, Qt Remote Objects)

### File Locations

| Path | Content |
|---|---|
| `~/.local/share/Logos/LogosApp/workflows/` | Saved workflow JSON files |
| `~/.local/share/Logos/LogosAppNix/plugins/workflow_canvas/` | Installed plugin (`.so`, `libQuickQanava.so`, `qt-6/qml/`) |

### Known Limitations

- Execution is synchronous -- large workflows block the UI
- No undo/redo
- Node IDs are memory addresses (not stable across sessions)
- No copy/paste for nodes
- Webhook port not configurable from the canvas
- No visual indication of data flow during execution (only post-execution result update)

---

## Version

1.0.0
