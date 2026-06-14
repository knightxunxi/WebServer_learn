// Copyright 2026, cpp-server-lab
// Boost.Asio HttpServer - 跨平台异步 HTTP 服务

#pragma once

#include <boost/asio.hpp>

#include <cstddef>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace csl::asio {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    bool keepAlive = false;
};

struct HttpResponse {
    int statusCode = 200;
    std::string statusMessage = "OK";
    std::string contentType = "text/html; charset=utf-8";
    std::string body;
    bool close = false;
};

class HttpServer {
public:
    using RequestHandler = std::function<void(const HttpRequest&, HttpResponse*)>;

    explicit HttpServer(unsigned short port, std::size_t threadCount = 1);
    ~HttpServer();

    void setRequestHandler(RequestHandler handler);
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
    RequestHandler handler_;
};

std::string serializeResponse(const HttpResponse& response);

}  // namespace csl::asio

