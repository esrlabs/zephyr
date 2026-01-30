add_library(
    zephyrEthAdapter
    src/zephyrEthAdapter/tcp/ZephyrTcpWrapper.c
    src/zephyrEthAdapter/tcp/ZephyrSocket.cpp
    src/zephyrEthAdapter/tcp/ZephyrServerSocket.cpp
    src/zephyrEthAdapter/udp/ZephyrDatagramSocket.cpp
    src/zephyrEthAdapter/udp/UdpEchoServer.cpp)


target_include_directories(zephyrEthAdapter PUBLIC include)

target_link_libraries(
    zephyrEthAdapter
    PUBLIC common etl util async cpp2ethernet
    PRIVATE)