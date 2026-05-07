cmake_minimum_required(VERSION 3.21)
project(PlayerLab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

add_executable(playerlab
    src/main.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/Sidebar.h
    src/ui/Sidebar.cpp
    src/ui/TopBar.h
    src/ui/TopBar.cpp
    src/ui/OpenGLVideoWidget.h
    src/ui/OpenGLVideoWidget.cpp
    src/ui/MediaLibraryWidget.h
    src/ui/MediaLibraryWidget.cpp
    src/ui/MediaInfoPanel.h
    src/ui/MediaInfoPanel.cpp
    src/ui/ControlBar.h
    src/ui/ControlBar.cpp
    src/render/IVideoRenderer.h
    src/render/OpenGLVideoRenderer.h
    src/render/OpenGLVideoRenderer.cpp
    src/render/DebugImageRenderer.h
    src/render/DebugImageRenderer.cpp
    src/render/RendererFactory.h
    src/render/RendererFactory.cpp
)

target_link_libraries(playerlab PRIVATE Qt6::Widgets Qt6::OpenGLWidgets)

if(APPLE)
    set_target_properties(playerlab PROPERTIES MACOSX_BUNDLE FALSE)
endif()
