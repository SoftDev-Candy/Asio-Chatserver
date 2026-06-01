cmake_minimum_required(VERSION 3.16)

project(SOUL LANGUAGES C CXX)

# Use a modern enough standard for optional, chrono, and the rest of this codebase.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SOUL_BUILD_UI "Build the Qt user interface target" ON)

# Let CMake run Qt's code generators automatically for the UI target.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(WIN32)
    # Boost.Asio checks the Windows target version through these macros.
    # Setting them here removes warnings and makes the target SDK explicit.
    add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)

    set(_qt_prefix_hints)

    # If Qt6_DIR is already exported, convert it to the install root and
    # add that root to CMAKE_PREFIX_PATH so find_package(Qt6) can use it.
    if(DEFINED ENV{Qt6_DIR} AND NOT "$ENV{Qt6_DIR}" STREQUAL "")
        get_filename_component(_qt_prefix_from_env "$ENV{Qt6_DIR}/../../.." ABSOLUTE)
        list(APPEND _qt_prefix_hints "${_qt_prefix_from_env}")
    endif()

    # Some setups use QTDIR instead of Qt6_DIR.
    if(DEFINED ENV{QTDIR} AND NOT "$ENV{QTDIR}" STREQUAL "")
        list(APPEND _qt_prefix_hints "$ENV{QTDIR}")
    endif()

    # Search common Qt install folders on Windows.
    # We try to match the active compiler so CLion uses a compatible Qt build.
    if(MSVC)
        file(GLOB _qt_windows_hints LIST_DIRECTORIES true "C:/Qt/*/msvc*_64")
    elseif(MINGW)
        file(GLOB _qt_windows_hints LIST_DIRECTORIES true "C:/Qt/*/mingw_64" "C:/Qt/*/llvm-mingw_64")
    else()
        file(GLOB _qt_windows_hints LIST_DIRECTORIES true "C:/Qt/*/*_64")
    endif()

    list(APPEND _qt_prefix_hints ${_qt_windows_hints})
    list(REMOVE_DUPLICATES _qt_prefix_hints)
    list(PREPEND CMAKE_PREFIX_PATH ${_qt_prefix_hints})
endif()

set(_boost_root_hints)

# Respect user-provided Boost install hints first.
foreach(_boost_env BOOST_ROOT Boost_ROOT)
    if(DEFINED ENV{${_boost_env}} AND NOT "$ENV{${_boost_env}}" STREQUAL "")
        list(APPEND _boost_root_hints "$ENV{${_boost_env}}")
    endif()
endforeach()

if(WIN32)
    # Search common unpacked Boost locations on Windows.
    # This project only needs Boost headers plus Boost.JSON's source file.
    file(GLOB _boost_windows_hints LIST_DIRECTORIES true
            "C:/Program Files/boost_*"
            "C:/Program Files/boost_*/*"
            "C:/boost_*"
            "C:/boost_*/*"
            "C:/local/boost_*"
            "C:/local/boost_*/*")
    list(APPEND _boost_root_hints ${_boost_windows_hints})
endif()

list(REMOVE_DUPLICATES _boost_root_hints)

find_path(BOOST_INCLUDE_DIR
        NAMES boost/json.hpp boost/asio.hpp
        HINTS ${_boost_root_hints}
        PATH_SUFFIXES include
        DOC "Path to the Boost headers")

if(NOT BOOST_INCLUDE_DIR)
    message(FATAL_ERROR
            "Boost headers were not found. Set BOOST_ROOT/Boost_ROOT to your Boost install, "
            "or install Boost in a standard Windows location such as C:/Program Files/boost_<version>.")
endif()

message(STATUS "Using Boost headers from: ${BOOST_INCLUDE_DIR}")

# Interface library = "carry these include paths/defines to whoever links me".
add_library(soul_boost_headers INTERFACE)
target_include_directories(soul_boost_headers INTERFACE ${BOOST_INCLUDE_DIR})

# Disable Boost's auto-link behavior on MSVC so CMake owns the linkage rules.
target_compile_definitions(soul_boost_headers INTERFACE BOOST_ALL_NO_LIB)

# Boost.JSON is not fully header-only.
# We compile one small .cpp that includes boost/json/src.hpp so we get the
# implementation without needing a separately built Boost JSON package.
add_library(soul_boost_json STATIC Common/BoostJson.cpp)
target_link_libraries(soul_boost_json PUBLIC soul_boost_headers)

add_library(sqlite3 STATIC external/sqlite/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/sqlite)

set(SOUL_PLATFORM_LIBS)
if(WIN32)
    # Winsock is the Windows socket library used underneath Boost.Asio.
    list(APPEND SOUL_PLATFORM_LIBS ws2_32)
endif()

# Main server/receiver executable.

add_executable(SOUL
        ReceiverServer/main.cpp
        ReceiverServer/SatelliteSim.cpp
        ReceiverServer/SatelliteSim.hpp
        Common/TelemetryFrame.hpp
        Common/TelemetryFrame.cpp
        Common/FrameCodec.cpp
        Common/FrameCodec.hpp
        Database/Database.cpp
        Database/Database.hpp)

add_executable(Client
        SatelliteSender/main.cpp
        SatelliteSender/TelemetryHub.cpp
        SatelliteSender/TelemetryHub.hpp
        Common/TelemetryFrame.cpp
        Common/TelemetryFrame.hpp
        Common/FrameCodec.cpp
        Common/FrameCodec.hpp
        simulation/SimulationState.cpp
        simulation/SimulationState.hpp

)

add_executable(Test
        scratchTests/TestFrameCodec.cpp
        Common/TelemetryFrame.cpp
        Common/TelemetryFrame.hpp
        Common/FrameCodec.cpp
        Common/FrameCodec.hpp)

if(SOUL_BUILD_UI)
    # Build the Qt UI when Qt is available, but do not fail the whole project
    # on machines that only have the server/client dependencies installed.
    find_package(Qt6 QUIET COMPONENTS Core Gui Widgets)
    find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)

    if(Qt6_FOUND)

        add_executable(UserInterface
            ui/main.cpp
            ui/mainwindow.h
            ui/mainwindow.cpp
            ui/DashboardUiHelper.hpp
            ui/DashboardUiHelper.cpp
            ui/OperatorActionBox.hpp
            ui/OperatorActionBox.cpp
            ui/mainwindow.ui
            OrbitalRendering/main/OrbitViewSceneHelper.hpp
            OrbitalRendering/main/OrbitViewSceneHelper.cpp
                OrbitalRendering/main/orbitview.cpp
                OrbitalRendering/main/orbitview.hpp
            OrbitalRendering/Shader/ShaderSource.hpp
                 OrbitalRendering/render/ObjLoader.cpp
                 OrbitalRendering/render/ObjLoader.hpp
                 OrbitalRendering/render/MeshData.hpp
                 OrbitalRendering/Camera.hpp
                 OrbitalRendering/Camera.cpp
                OrbitalRendering/main/SatelliteVisual.cpp
                OrbitalRendering/main/SatelliteVisual.hpp
                OrbitalRendering/render/TexturedMesh.cpp
                OrbitalRendering/render/TexturedMesh.hpp




        )
        target_link_libraries(UserInterface
             PRIVATE
                sqlite3
                Qt6::Core
                Qt6::Gui
                Qt6::Widgets
                Qt6::OpenGLWidgets

        )

        target_include_directories(UserInterface
                PRIVATE
                ${CMAKE_SOURCE_DIR}/ui
                ${CMAKE_SOURCE_DIR}/OrbitalRendering
                external/glm
        )
    else()
        message(WARNING
                "Qt6 was not found, so the UserInterface target will not be built. "
                "Set CMAKE_PREFIX_PATH or Qt6_DIR if you want the Qt target in CLion.")
    endif()
endif()

# SOUL needs sqlite3 because the server writes telemetry to the database.
# Client/Test only need the shared codec/JSON/networking support.
target_link_libraries(SOUL PRIVATE sqlite3 soul_boost_json ${SOUL_PLATFORM_LIBS})
target_link_libraries(Client PRIVATE sqlite3 soul_boost_json ${SOUL_PLATFORM_LIBS})
target_link_libraries(Test PRIVATE soul_boost_json ${SOUL_PLATFORM_LIBS})
