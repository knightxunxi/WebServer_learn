// Copyright 2026, cpp-server-lab
// Boost.Asio EchoServer - 跨平台异步 TCP 回显服务

#pragma once

#include <boost/asio.hpp>

#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

namespace csl::asio {

class EchoServer {
public:
    explicit EchoServer(unsigned short port, std::size_t threadCount = 1);
    ~EchoServer();

    void run();
    void stop();

private:
    class Session;

    void doAccept();

    boost::asio::io_context ioContext_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::signal_set signals_;
    std::size_t threadCount_;
    std::vector<std::thread> threads_;
};

}  // namespace csl::asio

