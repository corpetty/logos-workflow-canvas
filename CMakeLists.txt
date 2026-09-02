cmake_minimum_required(VERSION 3.16)
project(WorkflowCanvasPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

# Module name from metadata.json — single source of truth.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/metadata.json" METADATA_JSON)
string(JSON MODULE_NAME GET ${METADATA_JSON} name)

# ── The ui-host backend ──────────────────────────────────────────────────
#
# Universal UI module: we write the .rep view contract and the *Backend class.
# REP_FILE runs repc; the generated *Plugin glue compiles automatically. This
# half runs in the ui-host process and does nothing but call the three
# workflow modules.
logos_module(
    NAME ${MODULE_NAME}
    REP_FILE src/workflow_canvas.rep
    SOURCES
        src/workflow_canvas_backend.h
        src/workflow_canvas_backend.cpp
    INCLUDE_DIRS
        src
)

# ── The view-side QML plugin ─────────────────────────────────────────────
#
# The backend above cannot register QML types for the view: it lives in a
# different process. The view runs in the host application's QML engine, and
# basecamp's QML sandbox prepends this module's install directory to that
# engine's import path — so a QML module installed at <installDir>/WorkflowCanvas
# is importable as `import WorkflowCanvas 1.0`.
#
# That is what carries WorkflowGraph, the five typed node classes and the
# serializer across the move to ui_qml unchanged, instead of rewriting the
# graph model in JavaScript.
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

# QUICKQANAVA_ROOT comes from the flake's preConfigure (an env var), or may be
# passed on the command line for a plain CMake build.
if(NOT QUICKQANAVA_ROOT AND DEFINED ENV{QUICKQANAVA_ROOT})
    set(QUICKQANAVA_ROOT "$ENV{QUICKQANAVA_ROOT}")
endif()
if(NOT QUICKQANAVA_ROOT OR NOT EXISTS "${QUICKQANAVA_ROOT}/lib")
    message(FATAL_ERROR "QUICKQANAVA_ROOT must point at a built QuickQanava")
endif()

add_library(QuickQanava SHARED IMPORTED)
find_library(QUICKQANAVA_LIB NAMES QuickQanava
             PATHS "${QUICKQANAVA_ROOT}/lib" NO_DEFAULT_PATH REQUIRED)
set_target_properties(QuickQanava PROPERTIES
    IMPORTED_LOCATION "${QUICKQANAVA_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${QUICKQANAVA_ROOT}/include/QuickQanava"
)

qt_add_library(workflowcanvasqml SHARED)

qt_add_qml_module(workflowcanvasqml
    URI WorkflowCanvas
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml-modules/WorkflowCanvas"
    SOURCES
        src/WorkflowGraph.h
        src/WorkflowGraph.cpp
        src/WorkflowSerializer.h
        src/WorkflowSerializer.cpp
        src/nodes/ModuleMethodNode.h
        src/nodes/ModuleMethodNode.cpp
        src/nodes/UtilityNode.h
        src/nodes/UtilityNode.cpp
        src/nodes/ControlFlowNode.h
        src/nodes/ControlFlowNode.cpp
        src/nodes/TransformNode.h
        src/nodes/TransformNode.cpp
        src/nodes/TriggerNode.h
        src/nodes/TriggerNode.cpp
)

# The node delegates travel inside this library as Qt resources; the node
# classes load them by qrc: URL.
qt_add_resources(workflowcanvasqml "canvas_delegates"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        qml/delegates/ModuleNodeDelegate.qml
        qml/delegates/UtilityNodeDelegate.qml
        qml/delegates/ControlFlowDelegate.qml
        qml/delegates/TransformNodeDelegate.qml
        qml/delegates/TriggerNodeDelegate.qml
        src/icons/workflow.png
)

target_include_directories(workflowcanvasqml PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes
)

target_link_libraries(workflowcanvasqml PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
    QuickQanava
)

# Both this plugin and QuickQanava's own QML plugin must resolve the SAME
# libQuickQanava at runtime, and it ships beside them in the module's lib/.
set_target_properties(workflowcanvasqml PROPERTIES
    INSTALL_RPATH "$ORIGIN/.."
    BUILD_WITH_INSTALL_RPATH ON
)

# NOTE: no install() rule for this target. The module build supplies its own
# installPhase and never runs `ninja install`, so CMake install rules are
# ignored — the flake's postInstall stages qml-modules/WorkflowCanvas and the
# backing library out of the build tree instead.
