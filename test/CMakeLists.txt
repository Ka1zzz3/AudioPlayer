find_package(Qt6 QUIET COMPONENTS Test)
if(NOT Qt6Test_FOUND)
    message(WARNING "Qt6 Test component was not found; skipping skeleton tests.")
    return()
endif()

qt_add_executable(AudioPlayerSkeletonTest
    skeleton_test.cpp
)

target_link_libraries(AudioPlayerSkeletonTest PRIVATE
    Qt6::Test
)

add_test(NAME AudioPlayerSkeletonTest COMMAND AudioPlayerSkeletonTest)
