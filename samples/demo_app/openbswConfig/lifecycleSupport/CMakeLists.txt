add_library(lifecycleSupport
        src/lifecycle/console/LifecycleControlCommand.cpp
        src/lifecycle/console/StatisticsCommand.cpp
        )

target_include_directories(lifecycleSupport PUBLIC
        include)

target_link_libraries(lifecycleSupport PUBLIC
        lifecycle
        asyncBinding
        asyncCoreConfiguration
        runtime)