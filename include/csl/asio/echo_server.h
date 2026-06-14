// Copyright 2026, cpp-server-lab
// Boost.Asio EchoServer - 跨平台异步 TCP 回显服务

#pragma once

#include <boost/asio.hpp>

#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

namespace csl::asio {

// 基于 Boost.Asio 的异步 TCP 回显服务器。
//
// EchoServer 负责监听指定端口、接收 TCP 连接，并将客户端发送的数据原样写回。
// 当前实现使用一个 io_context 和多个工作线程，适合作为后续协议服务的最小网络骨架。
class EchoServer {
public:
    // 创建 EchoServer。
    //
    // port：监听端口。
    // threadCount：运行 io_context 的线程数，传入 0 时会修正为 1。
    explicit EchoServer(unsigned short port, std::size_t threadCount = 1);

    // 停止服务并等待内部工作线程退出。
    ~EchoServer();

    // 启动异步 accept，并运行 io_context。
    //
    // 该函数会阻塞当前线程，直到 stop() 被调用或收到退出信号。
    void run();

    // 停止接收新连接并终止 io_context。
    //
    // 该函数可以被信号回调或外部控制逻辑调用，多次调用应保持安全。
    void stop();

private:
    class Session;

    // 投递下一次异步 accept。
    //
    // 每次接收连接成功后都会继续调用自身，以保持监听循环。
    void doAccept();

    boost::asio::io_context ioContext_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::signal_set signals_;
    std::size_t threadCount_;
    std::vector<std::thread> threads_;
};

}  // namespace csl::asio
