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

qt_add_executable(AudioPlayerViewModelBehaviorTest
    viewmodel_behavior_test.cpp
    ../src/Model/AudioFile.cpp
    ../src/Model/FileScanner.cpp
    ../src/Model/JsonSongRepository.cpp
    ../src/Model/PlayList.cpp
    ../src/Model/Service/LibraryUseCase.cpp
    ../src/ViewModel/LibraryViewModel.cpp
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
