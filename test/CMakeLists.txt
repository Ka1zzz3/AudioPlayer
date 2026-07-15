find_package(Qt6 QUIET COMPONENTS Test)
if(NOT Qt6Test_FOUND)
    message(WARNING "Qt6 Test component was not found; skipping tests.")
    return()
endif()

qt_add_executable(AudioPlayerSkeletonTest
    skeleton_test.cpp
)

target_link_libraries(AudioPlayerSkeletonTest PRIVATE
    Qt6::Test
)

add_test(NAME AudioPlayerSkeletonTest COMMAND AudioPlayerSkeletonTest)

qt_add_executable(AudioPlayerModelBehaviorTest
    model_behavior_test.cpp
    ../src/Model/AudioFile.cpp
    ../src/Model/FileScanner.cpp
    ../src/Model/JsonSongRepository.cpp
    ../src/Model/PlayList.cpp
)

target_include_directories(AudioPlayerModelBehaviorTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerModelBehaviorTest PRIVATE
    Qt6::Core
    Qt6::Test
    nlohmann_json::nlohmann_json
)

add_test(NAME AudioPlayerModelBehaviorTest COMMAND AudioPlayerModelBehaviorTest)

qt_add_executable(AudioPlayerLibraryUseCaseTest
    library_usecase_test.cpp
    ../src/Model/AudioFile.cpp
    ../src/Model/FileScanner.cpp
    ../src/Model/JsonSongRepository.cpp
    ../src/Model/PlayList.cpp
    ../src/Model/Service/LibraryUseCase.cpp
)

target_include_directories(AudioPlayerLibraryUseCaseTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerLibraryUseCaseTest PRIVATE
    Qt6::Core
    Qt6::Test
    nlohmann_json::nlohmann_json
)

add_test(NAME AudioPlayerLibraryUseCaseTest COMMAND AudioPlayerLibraryUseCaseTest)

qt_add_executable(AudioPlayerPlaybackServiceContractTest
    playback_service_contract_test.cpp
    ../src/Model/Service/IPlaybackService.h
    ../src/Model/Service/PlaybackTypes.h
)

target_include_directories(AudioPlayerPlaybackServiceContractTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerPlaybackServiceContractTest PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME AudioPlayerPlaybackServiceContractTest COMMAND AudioPlayerPlaybackServiceContractTest)

qt_add_executable(AudioPlayerPlaybackUseCaseTest
    playback_usecase_test.cpp
    ../src/Model/AudioFile.cpp
    ../src/Model/Service/PlaybackUseCase.cpp
    ../src/Model/Service/PlaybackUseCase.h
    ../src/Model/Service/IPlaybackService.h
    ../src/Model/Service/PlaybackTypes.h
)

target_include_directories(AudioPlayerPlaybackUseCaseTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerPlaybackUseCaseTest PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME AudioPlayerPlaybackUseCaseTest COMMAND AudioPlayerPlaybackUseCaseTest)

qt_add_executable(AudioPlayerPlaybackViewModelTest
    playback_viewmodel_test.cpp
    ../src/Common/ViewCommand.cpp
    ../src/Model/AudioFile.cpp
    ../src/Model/Service/PlaybackUseCase.cpp
    ../src/Model/Service/PlaybackUseCase.h
    ../src/Model/Service/IPlaybackService.h
    ../src/Model/Service/PlaybackTypes.h
    ../src/ViewModel/PlaybackState.h
    ../src/ViewModel/PlaybackViewModel.cpp
    ../src/ViewModel/PlaybackViewModel.h
    ../src/ViewModel/PlaybackViewModelProtocol.h
)

target_include_directories(AudioPlayerPlaybackViewModelTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerPlaybackViewModelTest PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME AudioPlayerPlaybackViewModelTest COMMAND AudioPlayerPlaybackViewModelTest)

qt_add_executable(AudioPlayerPlaybackCompositionTest
    playback_composition_test.cpp
    ../src/Common/ViewCommand.cpp
    ../src/Model/AudioFile.cpp
    ../src/Model/FileScanner.cpp
    ../src/Model/JsonSongRepository.cpp
    ../src/Model/PlayList.cpp
    ../src/Model/Service/LibraryUseCase.cpp
    ../src/Model/Service/PlaybackUseCase.cpp
    ../src/Model/Service/PlaybackUseCase.h
    ../src/Model/Service/IPlaybackService.h
    ../src/Model/Service/PlaybackTypes.h
    ../src/ViewModel/LibraryViewModel.cpp
    ../src/ViewModel/LibraryViewModel.h
    ../src/ViewModel/LibraryViewModelProtocol.h
    ../src/ViewModel/PlaybackState.h
    ../src/ViewModel/PlaybackViewModel.cpp
    ../src/ViewModel/PlaybackViewModel.h
    ../src/ViewModel/PlaybackViewModelProtocol.h
    ../src/ViewModel/SongListModel.cpp
    ../src/ViewModel/SongListModel.h
)

target_include_directories(AudioPlayerPlaybackCompositionTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerPlaybackCompositionTest PRIVATE
    Qt6::Core
    Qt6::Test
    nlohmann_json::nlohmann_json
)

add_test(NAME AudioPlayerPlaybackCompositionTest COMMAND AudioPlayerPlaybackCompositionTest)

qt_add_executable(AudioPlayerViewModelBehaviorTest
    viewmodel_behavior_test.cpp
    ../src/Model/AudioFile.cpp
    ../src/Model/FileScanner.cpp
    ../src/Model/JsonSongRepository.cpp
    ../src/Model/PlayList.cpp
    ../src/Model/Service/LibraryUseCase.cpp
    ../src/Common/ViewCommand.cpp
    ../src/ViewModel/LibraryViewModel.cpp
    ../src/ViewModel/LibraryViewModelProtocol.h
    ../src/ViewModel/SongListModel.cpp
)

target_include_directories(AudioPlayerViewModelBehaviorTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerViewModelBehaviorTest PRIVATE
    Qt6::Core
    Qt6::Test
    nlohmann_json::nlohmann_json
)

add_test(NAME AudioPlayerViewModelBehaviorTest COMMAND AudioPlayerViewModelBehaviorTest)

qt_add_executable(AudioPlayerArchitectureBoundaryTest
    architecture_boundary_test.cpp
)

target_compile_definitions(AudioPlayerArchitectureBoundaryTest PRIVATE
    AUDIOPLAYER_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(AudioPlayerArchitectureBoundaryTest PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME AudioPlayerArchitectureBoundaryTest COMMAND AudioPlayerArchitectureBoundaryTest)
