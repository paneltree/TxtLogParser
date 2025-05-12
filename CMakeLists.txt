cmake_minimum_required(VERSION 3.16.0)

project(TxtLogParser VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure Visual Studio uses the correct compiler
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /utf-8")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Allow users to specify Qt path via -DQT_DIR or environment variable
set(QT_DIR "" CACHE PATH "Path to Qt installation (e.g., /path/to/Qt/6.8.3/platform)")
if(NOT QT_DIR)
    # Fallback to environment variable QT6_DIR if set
    if(DEFINED ENV{QT6_DIR})
        set(QT_DIR "$ENV{QT6_DIR}")
    endif()
endif()

# Set platform-specific hints for Qt if QT_DIR is provided
if(QT_DIR)
    if(WIN32)
        list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}/msvc2022_64/lib/cmake")
    elseif(APPLE)
        list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}/macos/lib/cmake")
    elseif(UNIX AND NOT APPLE)  # Linux
        list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}/gcc_64/lib/cmake")
    endif()
endif()

# Enable multi-configuration build
if(NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Available build configurations" FORCE)
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# Find Qt packages with a helpful error message if not found
find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt6 COMPONENTS LinguistTools REQUIRED)
find_package(Qt6 COMPONENTS Core REQUIRED)
find_package(Qt6 COMPONENTS Gui REQUIRED)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt6 not found. Please set -DQT_DIR to your Qt installation path (e.g., 'cmake -DQT_DIR=/path/to/Qt/6.8.3') or set the QT6_DIR environment variable.")
endif()

# Include nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://gitee.com/code9labs/nlohmann.json.git
    GIT_TAG v3.11.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)

# Translation files
set(TS_FILES
    translations/workspace_zh_CN.ts
    translations/workspace_en.ts
)

# Define directories to exclude from translation scanning
set(LUPDATE_EXCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}/build"
    "${CMAKE_SOURCE_DIR}/build2"
    "${CMAKE_SOURCE_DIR}/build3"
    "${CMAKE_SOURCE_DIR}/cmake-build-debug"
    "${CMAKE_SOURCE_DIR}/_deps"
)

# Create a list of source directories to scan (excluding test directories)
file(GLOB_RECURSE TS_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.ui"
)

# Create translations - Add environment variable to silence "undeclared qualified class" warnings
set(ENV{QT_LUPDATE_DISABLE_WARNING_FOR_UNDECLARED_CLASS} "1")
qt_create_translation(QM_FILES ${TS_SOURCES} ${TS_FILES} OPTIONS -no-obsolete)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set macOS-specific debugger attribute
if(APPLE)
    set(CMAKE_XCODE_ATTRIBUTE_DEBUGGER "lldb")
endif()

# Set macOS bundle properties
if(APPLE)
    set(MACOSX_BUNDLE TRUE)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.paneltree.txtlogparser")
    set(MACOSX_BUNDLE_BUNDLE_NAME "TxtLogParser")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
    set(MACOSX_BUNDLE_INFO_STRING "Txt Log Parser Tool by PanelTree")
    set(MACOSX_BUNDLE_ICON_FILE app_icon.icns)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/icons/app_icon.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
endif()

# Set Windows app icon resource
if(WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/app_icon.rc")
endif()

# Core library (pure C++)
set(CORE_SOURCES
    src/core/FileData.cpp
    src/core/FileData.h
    src/core/FilterData.cpp
    src/core/FilterData.h
    src/core/FilterSearchColorManager.cpp
    src/core/FilterSearchColorManager.h
    src/core/SearchData.cpp
    src/core/SearchData.h
    src/core/WorkspaceData.cpp
    src/core/WorkspaceData.h
    src/core/WorkspaceManager.cpp
    src/core/WorkspaceManager.h
    src/core/AppUtils.cpp
    src/core/AppUtils.h
    src/core/Logger.cpp
    src/core/Logger.h
    src/core/TroubleshootingLogger.cpp
    src/core/TroubleshootingLogger.h
    src/core/LoggerBridge.cpp
    src/core/LoggerBridge.h
    src/core/StringConverter.cpp
    src/core/StringConverter.h
    src/core/OutputData.cpp
    src/core/OutputData.h
    src/core/OutputWindow.cpp
    src/core/OutputWindow.h
    src/core/OutputLine.cpp
    src/core/OutputLine.h
    src/core/ColorData.cpp
    src/core/ColorData.h
)

# Bridge library (connects Qt UI with core)
set(BRIDGE_SOURCES
    src/bridge/QtBridge.cpp
    src/bridge/QtBridge.h
    src/bridge/FilterAdapter.cpp
    src/bridge/FilterAdapter.h
    src/bridge/SearchAdapter.cpp
    src/bridge/SearchAdapter.h
    src/bridge/FileAdapter.cpp
    src/bridge/FileAdapter.h
)

# UI sources (Qt-dependent)
set(UI_SOURCES
    src/ui/main.cpp
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/workspace.cpp
    src/ui/workspace.h
    src/ui/StyleManager.cpp
    src/ui/StyleManager.h
    src/ui/widgets/filelistwidget.cpp
    src/ui/widgets/filelistwidget.h
    src/ui/widgets/filterlistwidget.cpp
    src/ui/widgets/filterlistwidget.h
    src/ui/widgets/outputdisplaywidget.cpp
    src/ui/widgets/outputdisplaywidget.h
    src/ui/widgets/searchlistwidget.cpp
    src/ui/widgets/searchlistwidget.h
    src/ui/models/filterconfig.cpp
    src/ui/models/filterconfig.h
    src/ui/models/fileinfo.cpp
    src/ui/models/fileinfo.h
    src/ui/models/searchconfig.cpp
    src/ui/models/searchconfig.h
)

# Set executable type based on platform
if(APPLE)
    add_executable(TxtLogParser MACOSX_BUNDLE
        ${CORE_SOURCES}
        ${BRIDGE_SOURCES}
        ${UI_SOURCES}
        translations/workspace.qrc
        icons.qrc
        ${QM_FILES}
        ${CMAKE_CURRENT_SOURCE_DIR}/icons/app_icon.icns
    )
elseif(WIN32)
    add_executable(TxtLogParser WIN32
        ${CORE_SOURCES}
        ${BRIDGE_SOURCES}
        ${UI_SOURCES}
        translations/workspace.qrc
        icons.qrc
        ${QM_FILES}
        ${APP_ICON_RESOURCE_WINDOWS}
    )
else()
    add_executable(TxtLogParser
        ${CORE_SOURCES}
        ${BRIDGE_SOURCES}
        ${UI_SOURCES}
        translations/workspace.qrc
        icons.qrc
        ${QM_FILES}
    )
endif()

# Add include directories
target_include_directories(TxtLogParser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bridge
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/models
)

# Link Qt libraries and nlohmann_json
target_link_libraries(TxtLogParser PRIVATE 
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    nlohmann_json::nlohmann_json
)

# Add macdeployqt support
if(APPLE)
    # Find macdeployqt
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")

    if(MACDEPLOYQT_EXECUTABLE)
        # Create a custom target for deployment
        add_custom_target(deploy
            COMMAND ${MACDEPLOYQT_EXECUTABLE}
                "$<TARGET_BUNDLE_CONTENT_DIR:TxtLogParser>/.."
                -verbose=2
                -dmg
                -always-overwrite
            DEPENDS TxtLogParser
            COMMENT "Deploying application bundle..."
        )
    endif()
endif()

if(WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        # Create a clean deploy directory
        set(DEPLOY_DIR "${CMAKE_BINARY_DIR}/deploy")
        
        add_custom_target(deploy
            # Create clean directory
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${DEPLOY_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_DIR}"
            # Copy main executable
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:TxtLogParser>" "${DEPLOY_DIR}/"
            # Run windeployqt on the copied executable
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
                "${DEPLOY_DIR}/$<TARGET_FILE_NAME:TxtLogParser>"
                --no-translations
                --no-system-d3d-compiler
                --no-compiler-runtime
                --no-quick-import
                --verbose 0
            DEPENDS TxtLogParser
            COMMENT "Deploying clean Qt application package..."
        )
        
        
        add_custom_target(create_sfx
            COMMAND "C:/Program Files/7-Zip/7z.exe"
                a -sfx
                "${CMAKE_BINARY_DIR}/TxtLogParserInstaller.exe"
                "${DEPLOY_DIR}/*"
            DEPENDS deploy
            COMMENT "Creating self-extracting archive from clean deployment..."
        )
    endif()
endif()


if(WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        # Create a clean deploy directory
        
        add_custom_target(deploy_local
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
                "$<TARGET_FILE:TxtLogParser>"
                --no-translations
                --no-system-d3d-compiler
                --no-compiler-runtime
                --no-quick-import
                --verbose 0
            DEPENDS TxtLogParser
            COMMENT "Deploying Qt application to $<TARGET_FILE:TxtLogParser>..."
        )
    endif()
endif()
