cmake_minimum_required(VERSION 3.16)
project(WorkflowCanvasPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add local cmake modules (stub FindOpenGL for headless builds)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Quick Qml QuickWidgets QuickControls2)

# QuickQanava — build from vendored subdirectory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/QuickQanava/CMakeLists.txt")
    set(QUICK_QANAVA_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(vendor/QuickQanava)
else()
    message(FATAL_ERROR "QuickQanava not found. Run: git clone https://github.com/cneben/QuickQanava.git vendor/QuickQanava")
endif()

# Component interfaces (IComponent)
find_package(component-interfaces QUIET)
if(NOT component-interfaces_FOUND)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/interfaces)
    add_library(component-interfaces INTERFACE)
    target_include_directories(component-interfaces INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)
endif()

# Logos SDK
if(NOT DEFINED LOGOS_CPP_SDK_ROOT)
    if(DEFINED ENV{LOGOS_CPP_SDK_ROOT})
        set(LOGOS_CPP_SDK_ROOT "$ENV{LOGOS_CPP_SDK_ROOT}")
    else()
        set(LOGOS_CPP_SDK_ROOT "${CMAKE_SOURCE_DIR}/../logos-cpp-sdk")
    endif()
endif()
if(NOT DEFINED LOGOS_LIBLOGOS_ROOT)
    if(DEFINED ENV{LOGOS_LIBLOGOS_ROOT})
        set(LOGOS_LIBLOGOS_ROOT "$ENV{LOGOS_LIBLOGOS_ROOT}")
    else()
        set(LOGOS_LIBLOGOS_ROOT "${CMAKE_SOURCE_DIR}/../logos-liblogos")
    endif()
endif()

set(SOURCES
    src/CanvasComponent.cpp
    src/CanvasWidget.cpp
    src/WorkflowGraph.cpp
    src/nodes/ModuleMethodNode.cpp
    src/nodes/UtilityNode.cpp
    src/nodes/ControlFlowNode.cpp
    src/nodes/TransformNode.cpp
    src/nodes/TriggerNode.cpp
    src/WorkflowSerializer.cpp
    resources/canvas_resources.qrc
)

add_library(workflow_canvas SHARED ${SOURCES})

target_include_directories(workflow_canvas PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
    # QuickQanava headers
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/QuickQanava/src
)

# SDK includes
if(EXISTS "${LOGOS_CPP_SDK_ROOT}/cpp/logos_api.h")
    target_include_directories(workflow_canvas PRIVATE ${LOGOS_CPP_SDK_ROOT}/cpp)
elseif(EXISTS "${LOGOS_CPP_SDK_ROOT}/include/cpp/logos_api.h")
    target_include_directories(workflow_canvas PRIVATE
        ${LOGOS_CPP_SDK_ROOT}/include
        ${LOGOS_CPP_SDK_ROOT}/include/cpp
        ${LOGOS_CPP_SDK_ROOT}/include/core
    )
endif()

if(EXISTS "${LOGOS_LIBLOGOS_ROOT}/interface.h")
    target_include_directories(workflow_canvas PRIVATE ${LOGOS_LIBLOGOS_ROOT})
elseif(EXISTS "${LOGOS_LIBLOGOS_ROOT}/include/interface.h")
    target_include_directories(workflow_canvas PRIVATE ${LOGOS_LIBLOGOS_ROOT}/include)
endif()

target_link_libraries(workflow_canvas PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickWidgets
    Qt6::QuickControls2
    component-interfaces
    QuickQanava
)

# Output properties
if(APPLE)
    set_target_properties(workflow_canvas PROPERTIES PREFIX "" SUFFIX ".dylib")
else()
    set_target_properties(workflow_canvas PROPERTIES PREFIX "" SUFFIX ".so")
endif()
