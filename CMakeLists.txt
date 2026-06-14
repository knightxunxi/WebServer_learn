cmake_minimum_required(VERSION 3.20)

project(cpp_server_lab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CSL_BUILD_TESTS "构建 cpp-server-lab 测试" ON)
option(CSL_BUILD_LEGACY_EPOLL "构建旧 Linux epoll 实现测试" OFF)

find_package(Threads REQUIRED)

find_package(Boost 1.70 CONFIG QUIET)
if(NOT Boost_FOUND)
    if(POLICY CMP0167)
        cmake_policy(SET CMP0167 OLD)
    endif()
    find_package(Boost 1.70 REQUIRED)
endif()

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
    if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(csl_project_options INTERFACE
            -finput-charset=UTF-8
            -fexec-charset=UTF-8
        )
    endif()
elseif(MSVC)
    target_compile_options(csl_project_options INTERFACE
        /utf-8           # 源文件使用 UTF-8 编码（中文注释等）
        /Zc:__cplusplus  # 正确报告 __cplusplus 宏值
        /W4              # 高警告级别
    )
endif()

add_executable(csl_smoke apps/smoke/main.cpp)
target_link_libraries(csl_smoke PRIVATE csl_project_options)

# ===== Boost.Asio 服务端公共库 =====
add_library(csl_asio_server
    src/asio/echo_server.cpp
    src/asio/http_server.cpp
)
target_link_libraries(csl_asio_server
    PUBLIC
        csl_project_options
        Threads::Threads
)

if(TARGET Boost::headers)
    target_link_libraries(csl_asio_server PUBLIC Boost::headers)
elseif(TARGET Boost::boost)
    target_link_libraries(csl_asio_server PUBLIC Boost::boost)
else()
    target_include_directories(csl_asio_server PUBLIC ${Boost_INCLUDE_DIRS})
endif()

if(WIN32)
    target_link_libraries(csl_asio_server PUBLIC ws2_32 mswsock)
endif()

# ===== Echo Server 应用 =====
add_executable(csl_echo_server apps/echo_server/main.cpp)
target_link_libraries(csl_echo_server PRIVATE csl_asio_server)

# ===== HTTP WebServer 应用 =====
add_executable(csl_web_server
    apps/web_server/main.cpp
)
target_link_libraries(csl_web_server PRIVATE csl_asio_server)

include(CTest)
if(CSL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
