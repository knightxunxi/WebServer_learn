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
endif()

add_executable(csl_smoke apps/smoke/main.cpp)
target_link_libraries(csl_smoke PRIVATE csl_project_options)

include(CTest)
if(CSL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

