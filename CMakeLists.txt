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

option(AUDIOPLAYER_ENABLE_GSTREAMER_EFFECTS "Enable the optional GStreamer effects-capable playback backend build gate" ON)

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


set(AUDIOPLAYER_GSTREAMER_EFFECTS_AVAILABLE OFF)
set(AUDIOPLAYER_GSTREAMER_EFFECTS_REASON "GStreamer effects backend was disabled.")
if(AUDIOPLAYER_ENABLE_GSTREAMER_EFFECTS)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(AUDIOPLAYER_GSTREAMER_EFFECTS QUIET IMPORTED_TARGET
            gstreamer-1.0>=1.20
            gstreamer-audio-1.0>=1.20
            gstreamer-pbutils-1.0>=1.20
        )
    endif()

    find_program(AUDIOPLAYER_GST_INSPECT_EXECUTABLE NAMES gst-inspect-1.0)
    find_program(AUDIOPLAYER_GST_LAUNCH_EXECUTABLE NAMES gst-launch-1.0)

    set(AUDIOPLAYER_GSTREAMER_EFFECTS_PLUGINS_AVAILABLE OFF)
    if(AUDIOPLAYER_GST_INSPECT_EXECUTABLE)
        set(AUDIOPLAYER_GSTREAMER_EFFECTS_PLUGINS_AVAILABLE ON)
        foreach(plugin_name IN ITEMS scaletempo equalizer-nbands fakesink uridecodebin audioconvert audioresample)
            execute_process(
                COMMAND "${AUDIOPLAYER_GST_INSPECT_EXECUTABLE}" "${plugin_name}"
                RESULT_VARIABLE AUDIOPLAYER_GST_PLUGIN_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
            )
            if(NOT AUDIOPLAYER_GST_PLUGIN_RESULT EQUAL 0)
                set(AUDIOPLAYER_GSTREAMER_EFFECTS_PLUGINS_AVAILABLE OFF)
                list(APPEND AUDIOPLAYER_GSTREAMER_EFFECTS_MISSING_PLUGINS "${plugin_name}")
            endif()
        endforeach()
    endif()

    if(AUDIOPLAYER_GSTREAMER_EFFECTS_FOUND AND AUDIOPLAYER_GSTREAMER_EFFECTS_PLUGINS_AVAILABLE)
        set(AUDIOPLAYER_GSTREAMER_EFFECTS_AVAILABLE ON)
        set(AUDIOPLAYER_GSTREAMER_EFFECTS_REASON "GStreamer dev packages and effects runtime plugins were found.")
        message(STATUS "AudioPlayer GStreamer effects backend gate: available (${AUDIOPLAYER_GSTREAMER_EFFECTS_VERSION})")
    else()
        set(AUDIOPLAYER_GSTREAMER_EFFECTS_REASON "GStreamer effects backend prerequisites are missing.")
        message(WARNING
            "AUDIOPLAYER_ENABLE_GSTREAMER_EFFECTS=ON, but the effects backend cannot be built. "
            "Install GStreamer 1.20+ development packages for gstreamer-1.0, gstreamer-audio-1.0, "
            "gstreamer-pbutils-1.0 and runtime plugins: scaletempo, equalizer-nbands, fakesink, "
            "uridecodebin, audioconvert, audioresample. "
            "PkgConfig found=${PkgConfig_FOUND}; dev packages found=${AUDIOPLAYER_GSTREAMER_EFFECTS_FOUND}; "
            "gst-inspect=${AUDIOPLAYER_GST_INSPECT_EXECUTABLE}; missing plugins=${AUDIOPLAYER_GSTREAMER_EFFECTS_MISSING_PLUGINS}. "
            "Default fake-only tests and the Qt Multimedia playback fallback can still build, but real effects are unavailable."
        )
    endif()
else()
    message(STATUS "AudioPlayer GStreamer effects backend gate: disabled by AUDIOPLAYER_ENABLE_GSTREAMER_EFFECTS=OFF")
endif()

find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    message(FATAL_ERROR
        "nlohmann-json is required for the storage layer. Install the nlohmann_json CMake package "
        "through your system package manager or dependency manager, then configure CMAKE_PREFIX_PATH "
        "if CMake cannot find nlohmann_jsonConfig.cmake."
    )
endif()

find_program(AUDIOPLAYER_FFMPEG_EXECUTABLE NAMES ffmpeg)
find_program(AUDIOPLAYER_FFPROBE_EXECUTABLE NAMES ffprobe)
if(NOT AUDIOPLAYER_FFMPEG_EXECUTABLE)
    set(AUDIOPLAYER_FFMPEG_EXECUTABLE "ffmpeg")
    message(STATUS "ffmpeg executable was not found at configure time; transcoding will report a runtime error if unavailable.")
endif()
if(NOT AUDIOPLAYER_FFPROBE_EXECUTABLE)
    set(AUDIOPLAYER_FFPROBE_EXECUTABLE "ffprobe")
    message(STATUS "ffprobe executable was not found at configure time; transcoding progress may be unavailable if the tool is missing.")
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
        src/Model/Service/AudioEffectsTypes.cpp
        src/Model/Service/AudioEffectsTypes.h
        src/Model/Service/AudioEffectsUseCase.cpp
        src/Model/Service/AudioEffectsUseCase.h
        src/Model/Service/IAudioEffectsService.h
        src/ViewModel/AudioEffectsViewModel.cpp
        src/ViewModel/AudioEffectsViewModel.h
        src/ViewModel/AudioEffectsViewModelProtocol.h
        src/Model/Service/IPlaybackService.h
        src/Model/Service/ITranscodingBackend.h
        src/Model/Service/FfmpegTranscodingBackend.cpp
        src/Model/Service/FfmpegTranscodingBackend.h
        src/Model/Service/LibraryUseCase.cpp
        src/Model/Service/LibraryUseCase.h
        src/Model/Service/NullPlaybackService.cpp
        src/Model/Service/NullPlaybackService.h
        src/Model/Service/PlaybackTypes.h
        src/Model/Service/PlaylistCollectionUseCase.cpp
        src/Model/Service/PlaylistCollectionUseCase.h
        src/Model/Service/PlaybackUseCase.cpp
        src/Model/Service/PlaybackUseCase.h
        src/Model/Service/ProcessingUseCase.cpp
        src/Model/Service/ProcessingUseCase.h
        src/Model/Service/TranscodedPlaylistService.cpp
        src/Model/Service/TranscodedPlaylistService.h
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
        src/ViewModel/ProcessingTaskListModel.cpp
        src/ViewModel/ProcessingTaskListModel.h
        src/ViewModel/ProcessingViewModel.cpp
        src/ViewModel/ProcessingViewModel.h
        src/ViewModel/ProcessingViewModelProtocol.h
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

    target_compile_definitions(AudioPlayer PRIVATE
        AUDIOPLAYER_FFMPEG_EXECUTABLE="${AUDIOPLAYER_FFMPEG_EXECUTABLE}"
        AUDIOPLAYER_FFPROBE_EXECUTABLE="${AUDIOPLAYER_FFPROBE_EXECUTABLE}"
    )

    if(AUDIOPLAYER_GSTREAMER_EFFECTS_AVAILABLE)
        target_sources(AudioPlayer PRIVATE
            src/Model/Service/GStreamerEffectsPlaybackBackend.cpp
            src/Model/Service/GStreamerEffectsPlaybackBackend.h
        )
        target_link_libraries(AudioPlayer PRIVATE
            PkgConfig::AUDIOPLAYER_GSTREAMER_EFFECTS
        )
    endif()
endif()

if(AUDIOPLAYER_GSTREAMER_EFFECTS_AVAILABLE)
    add_executable(AudioPlayerGStreamerEffectsProbe
        tools/gstreamer_effects_probe.cpp
    )
    target_link_libraries(AudioPlayerGStreamerEffectsProbe PRIVATE
        PkgConfig::AUDIOPLAYER_GSTREAMER_EFFECTS
    )
elseif(AUDIOPLAYER_ENABLE_GSTREAMER_EFFECTS)
    add_custom_target(AudioPlayerGStreamerEffectsProbe
        COMMAND "${CMAKE_COMMAND}" -E echo "AudioPlayerGStreamerEffectsProbe is unavailable: ${AUDIOPLAYER_GSTREAMER_EFFECTS_REASON}"
        COMMAND "${CMAKE_COMMAND}" -E echo "Install GStreamer 1.20+ dev packages and runtime plugins, then reconfigure."
        COMMAND "${CMAKE_COMMAND}" -E false
        VERBATIM
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
