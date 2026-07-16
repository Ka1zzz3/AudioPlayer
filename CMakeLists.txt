cmake_minimum_required(VERSION 3.16)

project(AudioPlayer
    VERSION 0.1.0
    DESCRIPTION "Qt6 Widgets MVVM local audio player"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AUDIOPLAYER_BUILD_APP "Build the Qt Widgets application target" ON)

find_package(Qt6 6.4 QUIET COMPONENTS Core Gui)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "Qt6 6.4+ with Core and Gui components is required. "
        "Install the Qt6 development packages for your platform or configure CMAKE_PREFIX_PATH "
        "to the Qt6 installation prefix."
    )
endif()

if(AUDIOPLAYER_BUILD_APP)
    find_package(Qt6 6.4 QUIET COMPONENTS Widgets)
    if(NOT Qt6Widgets_FOUND)
        message(FATAL_ERROR
            "AUDIOPLAYER_BUILD_APP=ON requires Qt6 Widgets. "
            "Install the Qt Widgets development module or configure with "
            "-DAUDIOPLAYER_BUILD_APP=OFF for model/viewmodel-only tests."
        )
    endif()
    find_package(Qt6 6.4 QUIET COMPONENTS Multimedia)
    if(NOT Qt6Multimedia_FOUND)
        message(FATAL_ERROR
            "AUDIOPLAYER_BUILD_APP=ON requires Qt6 Multimedia for the playback backend. "
            "Install the Qt Multimedia development module or configure with "
            "-DAUDIOPLAYER_BUILD_APP=OFF for model/viewmodel-only tests."
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

if(AUDIOPLAYER_BUILD_APP)
    qt_add_executable(AudioPlayer
        src/App/AppCompositionRoot.cpp
        src/App/AppCompositionRoot.h
        src/App/widgets_main.cpp
        src/Common/ViewCommand.cpp
        src/Common/ViewCommand.h
        src/Model/AudioFile.cpp
        src/Model/AudioFile.h
        src/Model/FileScanner.cpp
        src/Model/FileScanner.h
        src/Model/JsonSongRepository.cpp
        src/Model/JsonSongRepository.h
        src/Model/JsonLibraryDocumentRepository.cpp
        src/Model/JsonLibraryDocumentRepository.h
        src/Model/LibraryDocument.cpp
        src/Model/LibraryDocument.h
        src/Model/LibraryPlaylist.cpp
        src/Model/LibraryPlaylist.h
        src/Model/PlayList.cpp
        src/Model/PlayList.h
        src/Model/ProcessingTask.cpp
        src/Model/ProcessingTask.h
        src/Model/ProcessingTypes.cpp
        src/Model/ProcessingTypes.h
        src/Model/Service/IPlaybackService.h
        src/Model/Service/LibraryUseCase.cpp
        src/Model/Service/LibraryUseCase.h
        src/Model/Service/NullPlaybackService.cpp
        src/Model/Service/NullPlaybackService.h
        src/Model/Service/PlaybackTypes.h
        src/Model/Service/PlaylistCollectionUseCase.cpp
        src/Model/Service/PlaylistCollectionUseCase.h
        src/Model/Service/PlaybackUseCase.cpp
        src/Model/Service/PlaybackUseCase.h
        src/Model/Service/QtMultimediaPlaybackService.cpp
        src/Model/Service/QtMultimediaPlaybackService.h
        src/View/MainWindow.cpp
        src/View/MainWindow.h
        src/ViewModel/LibraryViewModel.cpp
        src/ViewModel/LibraryViewModel.h
        src/ViewModel/LibraryViewModelProtocol.h
        src/ViewModel/PlaybackState.h
        src/ViewModel/PlaylistCollectionViewModel.cpp
        src/ViewModel/PlaylistCollectionViewModel.h
        src/ViewModel/PlaylistCollectionViewModelProtocol.h
        src/ViewModel/PlaylistListModel.cpp
        src/ViewModel/PlaylistListModel.h
        src/ViewModel/PlaybackViewModel.cpp
        src/ViewModel/PlaybackViewModel.h
        src/ViewModel/PlaybackViewModelProtocol.h
        src/ViewModel/SongListModel.cpp
        src/ViewModel/SongListModel.h
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
        Qt6::Multimedia
        Qt6::Widgets
        nlohmann_json::nlohmann_json
    )
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if(AUDIOPLAYER_BUILD_APP)
    install(TARGETS AudioPlayer
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )
endif()
