cmake_minimum_required(VERSION 3.20)

project(cpp_server_lab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CSL_BUILD_TESTS "Build cpp-server-lab tests" ON)

add_library(csl_project_options INTERFACE)
target_compile_features(csl_project_options INTERFACE cxx_std_20)
target_include_directories(csl_project_options INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(csl_project_options INTERFACE
        -Wall
        -Wextra
        -Wpedantic
    )
elseif(MSVC)
    target_compile_options(csl_project_options INTERFACE
        /utf-8           # 源文件使用 UTF-8 编码（中文注释等）
        /Zc:__cplusplus  # 正确报告 __cplusplus 宏值
        /W4              # 高警告级别
    )
endif()

add_executable(csl_smoke apps/smoke/main.cpp)
target_link_libraries(csl_smoke PRIVATE csl_project_options)

# ===== Echo Server =====
add_executable(csl_echo_server
    apps/echo_server/main.cpp
    src/net/InetAddress.cpp
    src/net/Socket.cpp
    src/net/Acceptor.cpp
    src/net/TcpConnection.cpp
    src/net/TcpServer.cpp
    src/net/Channel.cpp
    src/net/Poller.cpp
    src/net/EpollPoller.cpp
    src/net/EventLoop.cpp
    src/net/EventLoopThread.cpp
    src/net/EventLoopThreadPool.cpp
    src/net/Buffer.cpp
    src/timer/TimerQueue.cpp
    src/base/timestamp.cpp
)
target_link_libraries(csl_echo_server PRIVATE csl_project_options)

# ===== Web Server =====
add_executable(csl_web_server
    apps/web_server/main.cpp
    src/http/HttpRequest.cpp
    src/http/HttpServer.cpp
    src/net/InetAddress.cpp
    src/net/Socket.cpp
    src/net/Acceptor.cpp
    src/net/TcpConnection.cpp
    src/net/TcpServer.cpp
    src/net/Channel.cpp
    src/net/Poller.cpp
    src/net/EpollPoller.cpp
    src/net/EventLoop.cpp
    src/net/EventLoopThread.cpp
    src/net/EventLoopThreadPool.cpp
    src/net/Buffer.cpp
    src/timer/TimerQueue.cpp
    src/base/timestamp.cpp
)
target_link_libraries(csl_web_server PRIVATE csl_project_options)

include(CTest)
if(CSL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
