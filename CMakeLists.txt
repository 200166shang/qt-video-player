cmake_minimum_required(VERSION 3.21)
project(PlayerLab VERSION 0.1.0 LANGUAGES CXX)

option(PLAYERLAB_USE_CONAN "Enable Conan-provided dependencies" ON)

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

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Multimedia)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(stb REQUIRED)
find_package(ffmpeg REQUIRED COMPONENTS avcodec avformat avutil swscale swresample)
find_package(yaml-cpp REQUIRED)

add_library(playerlab_utils STATIC
    src/utils/Logger.h
    src/utils/Logger.cpp
    src/utils/StbVersion.h
    src/utils/StbVersion.cpp
)
target_link_libraries(playerlab_utils PUBLIC spdlog::spdlog_header_only fmt::fmt stb::stb)
target_include_directories(playerlab_utils PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_library(playerlab_config STATIC
    src/config/AppConfig.h
    src/config/AppConfig.cpp
)
target_link_libraries(playerlab_config PUBLIC
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    spdlog::spdlog_header_only
    playerlab_utils
)
target_include_directories(playerlab_config PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_library(playerlab_ffmpeg STATIC
    src/ffmpeg/FFmpegGlobal.h
    src/ffmpeg/FFmpegGlobal.cpp
    src/ffmpeg/FFmpegDemuxer.h
    src/ffmpeg/FFmpegDemuxer.cpp
    src/ffmpeg/FFmpegProbe.h
    src/ffmpeg/FFmpegProbe.cpp
    src/ffmpeg/PacketQueue.h
    src/ffmpeg/FrameQueue.h
    src/ffmpeg/QueuedPacket.h
    src/ffmpeg/FFmpegReadWorker.h
    src/ffmpeg/FFmpegReadWorker.cpp
    src/ffmpeg/FFmpegVideoDecoder.h
    src/ffmpeg/FFmpegVideoDecoder.cpp
    src/ffmpeg/FFmpegResampler.h
    src/ffmpeg/FFmpegResampler.cpp
    src/ffmpeg/FFmpegAudioDecoder.h
    src/ffmpeg/FFmpegAudioDecoder.cpp
)
target_link_libraries(playerlab_ffmpeg PUBLIC
    ffmpeg::avcodec
    ffmpeg::avformat
    ffmpeg::avutil
    ffmpeg::swscale
    ffmpeg::swresample
    fmt::fmt
    playerlab_utils
)
target_include_directories(playerlab_ffmpeg PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_executable(playerlab
    src/main.cpp
    src/core/MediaInfo.h
    src/core/MediaSource.h
    src/core/VideoFrame.h
    src/core/AudioFrame.h
    src/core/PlaybackState.h
    src/core/PlaybackClock.h
    src/core/PlaybackClock.cpp
    src/core/AVSynchronizer.h
    src/core/AVSynchronizer.cpp
    src/core/PlayerCore.h
    src/core/PlayerCore.cpp
    src/core/PlayerController.h
    src/core/PlayerController.cpp
    src/audio/IAudioOutput.h
    src/audio/QtAudioOutput.h
    src/audio/QtAudioOutput.cpp
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

target_link_libraries(playerlab
    PRIVATE
        Qt6::Widgets
        Qt6::OpenGLWidgets
        Qt6::Multimedia
        playerlab_utils
        playerlab_config
        playerlab_ffmpeg
)
target_include_directories(playerlab PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(APPLE)
    set_target_properties(playerlab PROPERTIES MACOSX_BUNDLE FALSE)
endif()

add_executable(playerlab_decode_probe
    src/tools/DecodeProbe.cpp
    src/core/MediaSource.h
    src/core/VideoFrame.h
    src/core/MediaInfo.h
    src/ffmpeg/FFmpegGlobal.h
    src/ffmpeg/FFmpegGlobal.cpp
    src/ffmpeg/FFmpegReadWorker.h
    src/ffmpeg/FFmpegReadWorker.cpp
    src/ffmpeg/FFmpegVideoDecoder.h
    src/ffmpeg/FFmpegVideoDecoder.cpp
)
target_link_libraries(playerlab_decode_probe
    PRIVATE
        Qt6::OpenGLWidgets
        playerlab_ffmpeg
        playerlab_utils
)
target_include_directories(playerlab_decode_probe PRIVATE ${CMAKE_SOURCE_DIR}/src)
