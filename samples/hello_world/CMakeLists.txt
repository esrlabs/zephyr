cmake_minimum_required(VERSION 3.20.0)

set(OPENBSW_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../openbsw
    CACHE PATH "Path to Eclipse OpenBSW")

# make sure zephyr compiler options are also set for cmake modules not depending on zephyr
add_compile_options($<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(hello_world)

include(${OPENBSW_DIR}/Filelists.cmake)

target_sources(app PRIVATE src/main.cpp)

target_link_libraries(app PUBLIC util)
