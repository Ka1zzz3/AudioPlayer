cmake_minimum_required(VERSION 3.16)

project(AudioPlayer
    VERSION 0.1.0
    DESCRIPTION "Qt6 QML MVVM local audio player"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.4 QUIET COMPONENTS Core Gui Qml Quick)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "Qt6 6.4+ with Core, Gui, Qml, and Quick components is required. "
        "Install the Qt6 development packages for your platform or configure CMAKE_PREFIX_PATH "
        "to the Qt6 installation prefix."
    )
endif()

find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    message(FATAL_ERROR
        "nlohmann-json is required for the storage layer. Install the nlohmann_json CMake package "
        "through your system package manager or dependency manager, then configure CMAKE_PREFIX_PATH "
        "if CMake cannot find nlohmann_jsonConfig.cmake."
    )
endif()

qt_standard_project_setup(REQUIRES 6.4)

qt_add_executable(AudioPlayer
    src/main.cpp
    src/Model/AudioFile.cpp
    src/Model/AudioFile.h
    src/Model/FileScanner.cpp
    src/Model/FileScanner.h
    src/Model/JsonSongRepository.cpp
    src/Model/JsonSongRepository.h
    src/Model/PlayList.cpp
    src/Model/PlayList.h
)

target_include_directories(AudioPlayer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(AudioPlayer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    nlohmann_json::nlohmann_json
)

qt_add_qml_module(AudioPlayer
    URI AudioPlayer
    VERSION 1.0
    SOURCES
        src/ViewModel/LibraryViewModel.cpp
        src/ViewModel/LibraryViewModel.h
        src/ViewModel/SongListModel.cpp
        src/ViewModel/SongListModel.h
    QML_FILES
        src/View/Main.qml
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

install(TARGETS AudioPlayer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
