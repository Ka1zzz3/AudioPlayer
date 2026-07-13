cmake_minimum_required(VERSION 3.16)

project(AudioPlayer
    VERSION 0.1.0
    DESCRIPTION "Qt6 QML MVVM local audio player"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AUDIOPLAYER_BUILD_QML_APP "Build the Qt Quick/QML application target" ON)

find_package(Qt6 6.4 QUIET COMPONENTS Core Gui)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "Qt6 6.4+ with Core and Gui components is required. "
        "Install the Qt6 development packages for your platform or configure CMAKE_PREFIX_PATH "
        "to the Qt6 installation prefix."
    )
endif()

if(AUDIOPLAYER_BUILD_QML_APP)
    find_package(Qt6 6.4 QUIET COMPONENTS Qml Quick)
    if(NOT Qt6Qml_FOUND OR NOT Qt6Quick_FOUND)
        message(FATAL_ERROR
            "AUDIOPLAYER_BUILD_QML_APP=ON requires Qt6 6.4+ Qml and Quick components. "
            "Install Qt Declarative/QML development packages or configure CMAKE_PREFIX_PATH "
            "to a Qt installation that provides Qt6Qml and Qt6Quick. "
            "For model/viewmodel-only tests with vcpkg qtbase, configure with "
            "-DAUDIOPLAYER_BUILD_QML_APP=OFF."
        )
    endif()
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

if(AUDIOPLAYER_BUILD_QML_APP)
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

    set_target_properties(AudioPlayer PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    )

    target_include_directories(AudioPlayer PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ViewModel"
    )

    target_link_libraries(AudioPlayer PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        nlohmann_json::nlohmann_json
    )

    set_source_files_properties(src/View/Main.qml PROPERTIES
        QT_RESOURCE_ALIAS Main.qml
    )

    qt_add_qml_module(AudioPlayer
        URI AudioPlayer
        VERSION 1.0
        # Keep the first-iteration resource layout stable while making Qt 6.10
        # QML policy defaults explicit: QTP0001 is avoided by RESOURCE_PREFIX /,
        # and QTP0004 by preserving the existing no-extra-qmldirs behavior.
        RESOURCE_PREFIX /
        NO_GENERATE_EXTRA_QMLDIRS
        SOURCES
            src/ViewModel/LibraryViewModel.cpp
            src/ViewModel/LibraryViewModel.h
            src/ViewModel/SongListModel.cpp
            src/ViewModel/SongListModel.h
        QML_FILES
            src/View/Main.qml
    )

    if(TARGET Qt6::QXcbIntegrationPlugin)
        qt_import_plugins(AudioPlayer INCLUDE Qt6::QXcbIntegrationPlugin)
    endif()

    if(TARGET Qt6::QWaylandIntegrationPlugin)
        qt_import_plugins(AudioPlayer INCLUDE Qt6::QWaylandIntegrationPlugin)
    endif()

endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if(AUDIOPLAYER_BUILD_QML_APP)
    install(TARGETS AudioPlayer
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )
endif()
