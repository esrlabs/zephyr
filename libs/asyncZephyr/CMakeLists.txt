add_library(asyncZephyr INTERFACE)

target_include_directories(asyncZephyr
        INTERFACE
        include)

target_link_libraries(asyncZephyr
        INTERFACE
        asyncImpl
        asyncZephyrImpl
        timer)

add_library(asyncZephyrImpl
        src/async/Async.cpp
        src/async/FutureSupport.cpp
        src/async/Hook.cpp
        src/async/Types.cpp)

target_include_directories(asyncZephyrImpl
        PRIVATE
        include)

target_link_libraries(asyncZephyrImpl
        PUBLIC
        bsp
        zephyr
        PRIVATE
        asyncBinding
        asyncCoreConfiguration
        etl)

if (BUILD_UNIT_TESTS)
    # These compile definitions apply to all unit test
    target_compile_definitions(asyncZephyr
            INTERFACE
            ASYNC_LOOPER_DISABLE
            ASYNC_TIMEOUTMANAGER2_DISABLE)

    add_library(asyncBinding INTERFACE)

    target_include_directories(asyncBinding
            INTERFACE
            test/include)

    add_library(asyncCoreConfiguration INTERFACE)

    target_include_directories(asyncCoreConfiguration
            INTERFACE
            test/include)
endif ()
