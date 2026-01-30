add_library(bspZephyr
        src/Stdio.cpp
        src/SystemTimer.cpp)

target_include_directories(bspZephyr 
        PUBLIC
        include)

target_link_libraries(bspZephyr
        etl
        zephyr
        platform)

add_library(bspInterrupts ALIAS bspZephyr)

add_library(canTransceiverZephyr
        src/ZephyrCanTransceiver.cpp)

target_include_directories(canTransceiverZephyr 
        PUBLIC
        include)

target_link_libraries(canTransceiverZephyr
        async
        cpp2can
        platform)

if (BUILD_UNIT_TESTS)

    add_subdirectory(mock)

endif ()
