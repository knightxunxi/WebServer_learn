# cpp-server-lab 测试构建配置
#
# 默认测试只覆盖当前主线 Boost.Asio 实现。
# 旧 epoll 实现已移动到 src_epoll，可通过 CSL_BUILD_LEGACY_EPOLL=ON 在 Linux 下启用。

# ===== 通用冒烟测试 =====

add_executable(csl_smoke_test smoke_test.cpp)
target_link_libraries(csl_smoke_test PRIVATE csl_project_options)

add_test(NAME csl_smoke_test COMMAND csl_smoke_test)

# ===== Boost.Asio 主线测试 =====

add_executable(csl_asio_http_response_test asio/http_response_test.cpp)
target_link_libraries(csl_asio_http_response_test PRIVATE csl_asio_server)

add_test(NAME csl_asio_http_response_test COMMAND csl_asio_http_response_test)

if(NOT CSL_BUILD_LEGACY_EPOLL)
    return()
endif()

if(WIN32)
    message(WARNING "CSL_BUILD_LEGACY_EPOLL=ON is ignored on Windows because the legacy implementation depends on epoll/timerfd/eventfd.")
    return()
endif()

# ===== 旧 epoll 实现测试 =====

# 测试 T01：noncopyable（仅头文件，无源文件依赖）
add_executable(csl_noncopyable_test base/noncopyable_test.cpp)
target_link_libraries(csl_noncopyable_test PRIVATE csl_project_options)
add_test(NAME csl_noncopyable_test COMMAND csl_noncopyable_test)

# 测试 T02：Timestamp
add_executable(csl_timestamp_test
    base/timestamp_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/base/timestamp.cpp
)
target_link_libraries(csl_timestamp_test PRIVATE csl_project_options)
add_test(NAME csl_timestamp_test COMMAND csl_timestamp_test)

# 测试 T03：Logger（依赖 timestamp.cpp）
add_executable(csl_logger_test
    base/logger_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/base/timestamp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/base/logger.cpp
)
target_link_libraries(csl_logger_test PRIVATE csl_project_options)
add_test(NAME csl_logger_test COMMAND csl_logger_test)

# ===== 里程碑 #3：事件循环基线测试 =====

# 测试 T04：EventLoop 集成测试（依赖 Channel + Poller + EpollPoller + EventLoop）
add_executable(csl_eventloop_test
    net/eventloop_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/Channel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/Poller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/EpollPoller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/EventLoop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/timer/TimerQueue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/base/timestamp.cpp
)
target_link_libraries(csl_eventloop_test PRIVATE csl_project_options Threads::Threads)
add_test(NAME csl_eventloop_test COMMAND csl_eventloop_test)

# ===== 里程碑 #4：TCP 基线测试 =====

# 测试 T05：InetAddress / Socket / Acceptor / TcpConnection 集成测试
add_executable(csl_tcp_test
    net/tcp_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/InetAddress.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/Socket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/Acceptor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/TcpConnection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/Channel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/Poller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/EpollPoller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/EventLoop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/timer/TimerQueue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/Buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/base/timestamp.cpp
)
target_link_libraries(csl_tcp_test PRIVATE csl_project_options Threads::Threads)
add_test(NAME csl_tcp_test COMMAND csl_tcp_test)

# ===== 里程碑 #6：Buffer 单元测试 =====

add_executable(csl_buffer_test
    net/buffer_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src_epoll/net/Buffer.cpp
)
target_link_libraries(csl_buffer_test PRIVATE csl_project_options)
add_test(NAME csl_buffer_test COMMAND csl_buffer_test)
