# cpp-server-lab 测试构建配置
#
# 每个测试直接编译为独立可执行文件，通过 CTest 注册。
# 简单做法：将测试依赖的 src 实现文件直接加入对应测试 target。

# ===== 已有的冒烟测试 =====

add_executable(csl_smoke_test smoke_test.cpp)
target_link_libraries(csl_smoke_test PRIVATE csl_project_options)

add_test(NAME csl_smoke_test COMMAND csl_smoke_test)

# ===== 里程碑 #2：基础工具模块测试 =====

# 测试 T01：noncopyable（仅头文件，无源文件依赖）
add_executable(csl_noncopyable_test base/noncopyable_test.cpp)
target_link_libraries(csl_noncopyable_test PRIVATE csl_project_options)
add_test(NAME csl_noncopyable_test COMMAND csl_noncopyable_test)

# 测试 T02：Timestamp
add_executable(csl_timestamp_test
    base/timestamp_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base/timestamp.cpp
)
target_link_libraries(csl_timestamp_test PRIVATE csl_project_options)
add_test(NAME csl_timestamp_test COMMAND csl_timestamp_test)

# 测试 T03：Logger（依赖 timestamp.cpp）
add_executable(csl_logger_test
    base/logger_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base/timestamp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base/logger.cpp
)
target_link_libraries(csl_logger_test PRIVATE csl_project_options)
add_test(NAME csl_logger_test COMMAND csl_logger_test)

# ===== 里程碑 #3：事件循环基线测试 =====

# 测试 T04：EventLoop 集成测试（依赖 Channel + Poller + EpollPoller + EventLoop）
add_executable(csl_eventloop_test
    net/eventloop_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/Channel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/Poller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/EpollPoller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/EventLoop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/timer/TimerQueue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base/timestamp.cpp
)
target_link_libraries(csl_eventloop_test PRIVATE csl_project_options)
add_test(NAME csl_eventloop_test COMMAND csl_eventloop_test)

# ===== 里程碑 #4：TCP 基线测试 =====

# 测试 T05：InetAddress / Socket / Acceptor / TcpConnection 集成测试
add_executable(csl_tcp_test
    net/tcp_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/InetAddress.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/Socket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/Acceptor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/TcpConnection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/Channel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/Poller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/EpollPoller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/EventLoop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base/timestamp.cpp
)
target_link_libraries(csl_tcp_test PRIVATE csl_project_options)
add_test(NAME csl_tcp_test COMMAND csl_tcp_test)

# ===== 里程碑 #6：Buffer 单元测试 =====

add_executable(csl_buffer_test
    net/buffer_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/Buffer.cpp
)
target_link_libraries(csl_buffer_test PRIVATE csl_project_options)
add_test(NAME csl_buffer_test COMMAND csl_buffer_test)
