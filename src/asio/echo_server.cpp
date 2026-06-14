// Copyright 2026, cpp-server-lab

#include "csl/asio/echo_server.h"

#include <algorithm>
#include <array>
#include <csignal>
#include <memory>
#include <utility>

namespace csl::asio {

using boost::asio::ip::tcp;

class EchoServer::Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void start() {
        doRead();
    }

private:
    void doRead() {
        auto self = shared_from_this();
        socket_.async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](const boost::system::error_code& ec, std::size_t length) {
                if (!ec) {
                    doWrite(length);
                }
            });
    }

    void doWrite(std::size_t length) {
        auto self = shared_from_this();
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(buffer_.data(), length),
            [this, self](const boost::system::error_code& ec, std::size_t) {
                if (!ec) {
                    doRead();
                }
            });
    }

    tcp::socket socket_;
    std::array<char, 8192> buffer_{};
};

EchoServer::EchoServer(unsigned short port, std::size_t threadCount)
    : ioContext_(static_cast<int>(std::max<std::size_t>(1, threadCount)))
    , acceptor_(ioContext_, tcp::endpoint(tcp::v4(), port))
    , signals_(ioContext_, SIGINT, SIGTERM)
    , threadCount_(std::max<std::size_t>(1, threadCount))
{
    signals_.async_wait([this](const boost::system::error_code&, int) {
        stop();
    });
}

EchoServer::~EchoServer() {
    stop();
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void EchoServer::run() {
    doAccept();

    for (std::size_t i = 1; i < threadCount_; ++i) {
        threads_.emplace_back([this]() {
            ioContext_.run();
        });
    }

    ioContext_.run();
}

void EchoServer::stop() {
    boost::system::error_code ignored;
    acceptor_.close(ignored);
    ioContext_.stop();
}

void EchoServer::doAccept() {
    acceptor_.async_accept(
        [this](const boost::system::error_code& ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket))->start();
            }

            if (acceptor_.is_open()) {
                doAccept();
            }
        });
}

}  // namespace csl::asio
