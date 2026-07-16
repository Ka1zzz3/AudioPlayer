find_package(Qt6 QUIET COMPONENTS Test Multimedia)
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

qt_add_executable(AudioPlayerLibraryDocumentModelTest
    library_document_model_test.cpp
    ../src/Model/AudioFile.cpp
    ../src/Model/LibraryDocument.cpp
    ../src/Model/LibraryPlaylist.cpp
)

target_include_directories(AudioPlayerLibraryDocumentModelTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerLibraryDocumentModelTest PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME AudioPlayerLibraryDocumentModelTest COMMAND AudioPlayerLibraryDocumentModelTest)



qt_add_executable(AudioPlayerProcessingTaskModelTest
    processing_task_model_test.cpp
    ../src/Model/ProcessingTask.cpp
    ../src/Model/ProcessingTask.h
    ../src/Model/ProcessingTypes.cpp
    ../src/Model/ProcessingTypes.h
)

target_include_directories(AudioPlayerProcessingTaskModelTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerProcessingTaskModelTest PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME AudioPlayerProcessingTaskModelTest COMMAND AudioPlayerProcessingTaskModelTest)

qt_add_executable(AudioPlayerJsonLibraryDocumentRepositoryTest
    json_library_document_repository_test.cpp
    ../src/Model/AudioFile.cpp
    ../src/Model/JsonLibraryDocumentRepository.cpp
    ../src/Model/LibraryDocument.cpp
    ../src/Model/LibraryPlaylist.cpp
)

target_include_directories(AudioPlayerJsonLibraryDocumentRepositoryTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerJsonLibraryDocumentRepositoryTest PRIVATE
    Qt6::Core
    Qt6::Test
    nlohmann_json::nlohmann_json
)

add_test(NAME AudioPlayerJsonLibraryDocumentRepositoryTest COMMAND AudioPlayerJsonLibraryDocumentRepositoryTest)


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

qt_add_executable(AudioPlayerPlaylistCollectionUseCaseTest
    playlist_collection_usecase_test.cpp
    ../src/Model/AudioFile.cpp
    ../src/Model/JsonLibraryDocumentRepository.cpp
    ../src/Model/LibraryDocument.cpp
    ../src/Model/LibraryPlaylist.cpp
    ../src/Model/Service/PlaylistCollectionUseCase.cpp
)

target_include_directories(AudioPlayerPlaylistCollectionUseCaseTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerPlaylistCollectionUseCaseTest PRIVATE
    Qt6::Core
    Qt6::Test
    nlohmann_json::nlohmann_json
)

add_test(NAME AudioPlayerPlaylistCollectionUseCaseTest COMMAND AudioPlayerPlaylistCollectionUseCaseTest)

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


qt_add_executable(AudioPlayerTranscodingBackendContractTest
    transcoding_backend_contract_test.cpp
    ../src/Model/ProcessingTypes.cpp
    ../src/Model/ProcessingTypes.h
    ../src/Model/Service/ITranscodingBackend.h
)

target_include_directories(AudioPlayerTranscodingBackendContractTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerTranscodingBackendContractTest PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME AudioPlayerTranscodingBackendContractTest COMMAND AudioPlayerTranscodingBackendContractTest)


qt_add_executable(AudioPlayerProcessingUseCaseTest
    processing_usecase_test.cpp
    ../src/Model/ProcessingTask.cpp
    ../src/Model/ProcessingTask.h
    ../src/Model/ProcessingTypes.cpp
    ../src/Model/ProcessingTypes.h
    ../src/Model/Service/ITranscodingBackend.h
    ../src/Model/Service/ProcessingUseCase.cpp
    ../src/Model/Service/ProcessingUseCase.h
)

target_include_directories(AudioPlayerProcessingUseCaseTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerProcessingUseCaseTest PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME AudioPlayerProcessingUseCaseTest COMMAND AudioPlayerProcessingUseCaseTest)

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
    ../src/Model/JsonLibraryDocumentRepository.cpp
    ../src/Model/JsonSongRepository.cpp
    ../src/Model/LibraryDocument.cpp
    ../src/Model/LibraryPlaylist.cpp
    ../src/Model/PlayList.cpp
    ../src/Model/Service/LibraryUseCase.cpp
    ../src/Model/Service/PlaybackUseCase.cpp
    ../src/Model/Service/PlaybackUseCase.h
    ../src/Model/Service/PlaylistCollectionUseCase.cpp
    ../src/Model/Service/PlaylistCollectionUseCase.h
    ../src/Model/Service/IPlaybackService.h
    ../src/Model/Service/PlaybackTypes.h
    ../src/ViewModel/LibraryViewModel.cpp
    ../src/ViewModel/LibraryViewModel.h
    ../src/ViewModel/LibraryViewModelProtocol.h
    ../src/ViewModel/PlaybackState.h
    ../src/ViewModel/PlaybackViewModel.cpp
    ../src/ViewModel/PlaybackViewModel.h
    ../src/ViewModel/PlaybackViewModelProtocol.h
    ../src/ViewModel/PlaylistCollectionViewModel.cpp
    ../src/ViewModel/PlaylistCollectionViewModel.h
    ../src/ViewModel/PlaylistCollectionViewModelProtocol.h
    ../src/ViewModel/PlaylistListModel.cpp
    ../src/ViewModel/PlaylistListModel.h
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

if(Qt6Multimedia_FOUND)
    qt_add_executable(AudioPlayerQtMultimediaPlaybackServiceTest
        qt_multimedia_playback_service_test.cpp
        ../src/Model/Service/QtMultimediaPlaybackService.cpp
        ../src/Model/Service/QtMultimediaPlaybackService.h
        ../src/Model/Service/IPlaybackService.h
        ../src/Model/Service/PlaybackTypes.h
    )

    target_include_directories(AudioPlayerQtMultimediaPlaybackServiceTest PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../src"
    )

    target_link_libraries(AudioPlayerQtMultimediaPlaybackServiceTest PRIVATE
        Qt6::Core
        Qt6::Multimedia
        Qt6::Test
    )

    add_test(NAME AudioPlayerQtMultimediaPlaybackServiceTest COMMAND AudioPlayerQtMultimediaPlaybackServiceTest)
    set_tests_properties(AudioPlayerQtMultimediaPlaybackServiceTest PROPERTIES
        DISABLED TRUE
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

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

qt_add_executable(AudioPlayerPlaylistCollectionViewModelTest
    playlist_collection_viewmodel_test.cpp
    ../src/Common/ViewCommand.cpp
    ../src/Model/AudioFile.cpp
    ../src/Model/JsonLibraryDocumentRepository.cpp
    ../src/Model/LibraryDocument.cpp
    ../src/Model/LibraryPlaylist.cpp
    ../src/Model/Service/PlaylistCollectionUseCase.cpp
    ../src/ViewModel/PlaylistCollectionViewModel.cpp
    ../src/ViewModel/PlaylistCollectionViewModel.h
    ../src/ViewModel/PlaylistCollectionViewModelProtocol.h
    ../src/ViewModel/PlaylistListModel.cpp
    ../src/ViewModel/PlaylistListModel.h
)

target_include_directories(AudioPlayerPlaylistCollectionViewModelTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(AudioPlayerPlaylistCollectionViewModelTest PRIVATE
    Qt6::Core
    Qt6::Test
    nlohmann_json::nlohmann_json
)

add_test(NAME AudioPlayerPlaylistCollectionViewModelTest COMMAND AudioPlayerPlaylistCollectionViewModelTest)

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
