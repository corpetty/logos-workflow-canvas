cmake_minimum_required(VERSION 3.16)
project(WorkflowCanvasPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add local cmake modules (stub FindOpenGL for headless builds)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Quick Qml QuickWidgets QuickControls2)

# Header-only; pulled in by liblogos's logos_api_client.h since the SDK split.
find_package(nlohmann_json REQUIRED)

# QuickQanava — prefer pre-built shared library (Nix), fall back to vendored subdirectory
if(DEFINED QUICKQANAVA_ROOT AND EXISTS "${QUICKQANAVA_ROOT}/lib")
    message(STATUS "Using pre-built QuickQanava from: ${QUICKQANAVA_ROOT}")
    add_library(QuickQanava SHARED IMPORTED)
    find_library(QUICKQANAVA_LIB NAMES QuickQanava
                 PATHS "${QUICKQANAVA_ROOT}/lib" NO_DEFAULT_PATH)
    set_target_properties(QuickQanava PROPERTIES
        IMPORTED_LOCATION "${QUICKQANAVA_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${QUICKQANAVA_ROOT}/include/QuickQanava"
    )
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/QuickQanava/CMakeLists.txt")
    message(STATUS "Building QuickQanava from vendored subdirectory")
    set(QUICK_QANAVA_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(vendor/QuickQanava)
else()
    message(FATAL_ERROR "QuickQanava not found. Either set QUICKQANAVA_ROOT or run: git clone https://github.com/cneben/QuickQanava.git vendor/QuickQanava")
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
)
# QuickQanava headers — use pre-built path or vendored source
if(DEFINED QUICKQANAVA_ROOT AND EXISTS "${QUICKQANAVA_ROOT}/include/QuickQanava")
    target_include_directories(workflow_canvas PRIVATE ${QUICKQANAVA_ROOT}/include/QuickQanava)
else()
    target_include_directories(workflow_canvas PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/QuickQanava/src)
endif()

# SDK / liblogos includes.
#
# LogosAPI moved out of logos-cpp-sdk during the SDK split: liblogos is what
# ships logos_api.h now, and its include tree flattened (logos_api.h at the
# root of include/, interface.h under include/core/). The older layouts are
# still probed so this builds against an older pin too.
foreach(_root "${LOGOS_CPP_SDK_ROOT}" "${LOGOS_LIBLOGOS_ROOT}")
    if(EXISTS "${_root}/cpp/logos_api.h")
        target_include_directories(workflow_canvas PRIVATE "${_root}" "${_root}/cpp")
    endif()
    if(EXISTS "${_root}/include/logos_api.h")
        # Current layout.
        target_include_directories(workflow_canvas PRIVATE "${_root}/include")
    endif()
    if(EXISTS "${_root}/include/cpp/logos_api.h")
        target_include_directories(workflow_canvas PRIVATE
            "${_root}/include" "${_root}/include/cpp")
    endif()
    if(EXISTS "${_root}/include/core/interface.h")
        target_include_directories(workflow_canvas PRIVATE "${_root}/include/core")
    elseif(EXISTS "${_root}/include/interface.h")
        target_include_directories(workflow_canvas PRIVATE "${_root}/include")
    elseif(EXISTS "${_root}/interface.h")
        target_include_directories(workflow_canvas PRIVATE "${_root}")
    endif()
endforeach()

# LogosAPI::getClient lives in liblogos_qt_host, LogosAPIClient::invokeRemoteMethod
# in liblogos_protocol. Both must be LINKED, not left undefined:
#
# The host binary exports NOTHING (it is not linked --export-dynamic; `nm -D`
# on LogosBasecamp lists zero defined symbols), so a plugin that leaves these
# undefined fails to dlopen and the widget is never created — which is exactly
# how this presented: the canvas's declared module dependencies loaded, then
# nothing, and no CanvasWidget object existed in the running app.
#
# Linking the SHARED libraries, not static copies, is also what keeps the token
# ring intact: LogosAPI caches &TokenManager::instance(), so an image carrying
# its own copy of that code would cache a DIFFERENT singleton than the host
# wrote the token into (see the note in logos_api.h). The host has already
# loaded both sonames by the time it dlopens this plugin, so the plugin binds
# to the host's copies.
foreach(_lib logos_qt_host logos_protocol)
    find_library(LOGOS_LIB_${_lib} NAMES ${_lib}
                 PATHS "${LOGOS_LIBLOGOS_ROOT}/lib" NO_DEFAULT_PATH)
    if(LOGOS_LIB_${_lib})
        target_link_libraries(workflow_canvas PRIVATE "${LOGOS_LIB_${_lib}}")
        message(STATUS "Linking host runtime: ${LOGOS_LIB_${_lib}}")
    else()
        message(FATAL_ERROR
            "lib${_lib} not found under ${LOGOS_LIBLOGOS_ROOT}/lib — the host "
            "exports no symbols, so leaving LogosAPI undefined would fail at "
            "dlopen with the widget never created.")
    endif()
endforeach()

target_link_libraries(workflow_canvas PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickWidgets
    Qt6::QuickControls2
    component-interfaces
    QuickQanava
    nlohmann_json::nlohmann_json
)

# Output properties
if(APPLE)
    # LogosAPI / LogosAPIClient are resolved by the HOST at load time — this
    # plugin deliberately links liblogos's headers but not its library. ELF
    # allows a shared object to carry undefined symbols, so Linux links clean;
    # macOS ld does not, and fails with "symbol(s) not found for architecture
    # arm64" on LogosAPI::getClient and LogosAPIClient::invokeRemoteMethod.
    # Defer them, the same way logos-module-builder's LogosModule.cmake does
    # for the module plugins it builds.
    target_link_options(workflow_canvas PRIVATE -undefined dynamic_lookup)
    set_target_properties(workflow_canvas PROPERTIES PREFIX "" SUFFIX ".dylib")
else()
    set_target_properties(workflow_canvas PROPERTIES PREFIX "" SUFFIX ".so")
endif()
