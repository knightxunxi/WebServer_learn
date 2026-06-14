// Copyright 2026, cpp-server-lab

#include "csl/asio/http_server.h"

#include <boost/asio/buffers_iterator.hpp>

#include <algorithm>
#include <cctype>
#include <csignal>
#include <memory>
#include <sstream>
#include <utility>

namespace csl::asio {

using boost::asio::ip::tcp;

namespace {

bool iequals(std::string lhs, std::string rhs) {
    auto lower = [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    };
    std::transform(lhs.begin(), lhs.end(), lhs.begin(), lower);
    std::transform(rhs.begin(), rhs.end(), rhs.begin(), lower);
    return lhs == rhs;
}

HttpRequest parseRequestLineAndHeaders(const std::string& raw) {
    HttpRequest request;
    bool connectionClose = false;
    bool connectionKeepAlive = false;
    std::istringstream stream(raw);
    stream >> request.method >> request.path >> request.version;

    std::string line;
    std::getline(stream, line); // 消费请求行末尾的换行残留
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            break;
        }

        auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        value.erase(value.begin(),
                    std::find_if(value.begin(), value.end(), [](unsigned char ch) {
                        return !std::isspace(ch);
                    }));

        if (iequals(key, "Connection")) {
            connectionClose = iequals(value, "close");
            connectionKeepAlive = iequals(value, "keep-alive");
        }
    }

    if (request.version == "HTTP/1.1") {
        request.keepAlive = !connectionClose;
    } else if (request.version == "HTTP/1.0") {
        request.keepAlive = connectionKeepAlive;
    }
    return request;
}

}  // namespace

class HttpServer::Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, RequestHandler handler)
        : socket_(std::move(socket))
        , handler_(std::move(handler)) {}

    void start() {
        doRead();
    }

private:
    void doRead() {
        auto self = shared_from_this();
        boost::asio::async_read_until(
            socket_,
            buffer_,
            "\r\n\r\n",
            [this, self](const boost::system::error_code& ec, std::size_t length) {
                if (!ec) {
                    handleRequest(length);
                }
            });
    }

    void handleRequest(std::size_t length) {
        std::string raw(
            boost::asio::buffers_begin(buffer_.data()),
            boost::asio::buffers_begin(buffer_.data()) + static_cast<std::ptrdiff_t>(length));
        buffer_.consume(length);

        HttpRequest request = parseRequestLineAndHeaders(raw);
        HttpResponse response;
        response.close = !request.keepAlive;

        if (request.method.empty() || request.path.empty() || request.version.empty()) {
            response.statusCode = 400;
            response.statusMessage = "Bad Request";
            response.body = "<h1>400 Bad Request</h1>";
            response.close = true;
        } else if (handler_) {
            handler_(request, &response);
        } else {
            response.statusCode = 404;
            response.statusMessage = "Not Found";
            response.body = "<h1>404 Not Found</h1>";
        }

        auto data = std::make_shared<std::string>(serializeResponse(response));
        auto self = shared_from_this();
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(*data),
            [this, self, data, close = response.close](
                const boost::system::error_code& ec, std::size_t) {
                if (!ec && !close) {
                    doRead();
                    return;
                }

                boost::system::error_code ignored;
                socket_.shutdown(tcp::socket::shutdown_both, ignored);
                socket_.close(ignored);
            });
    }

    tcp::socket socket_;
    boost::asio::streambuf buffer_;
    RequestHandler handler_;
};

HttpServer::HttpServer(unsigned short port, std::size_t threadCount)
    : ioContext_(static_cast<int>(std::max<std::size_t>(1, threadCount)))
    , acceptor_(ioContext_, tcp::endpoint(tcp::v4(), port))
    , signals_(ioContext_, SIGINT, SIGTERM)
    , threadCount_(std::max<std::size_t>(1, threadCount))
{
    signals_.async_wait([this](const boost::system::error_code&, int) {
        stop();
    });
}

HttpServer::~HttpServer() {
    stop();
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void HttpServer::setRequestHandler(RequestHandler handler) {
    handler_ = std::move(handler);
}

void HttpServer::run() {
    doAccept();

    for (std::size_t i = 1; i < threadCount_; ++i) {
        threads_.emplace_back([this]() {
            ioContext_.run();
        });
    }

    ioContext_.run();
}

void HttpServer::stop() {
    boost::system::error_code ignored;
    acceptor_.close(ignored);
    ioContext_.stop();
}

void HttpServer::doAccept() {
    acceptor_.async_accept(
        [this](const boost::system::error_code& ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), handler_)->start();
            }

            if (acceptor_.is_open()) {
                doAccept();
            }
        });
}

std::string serializeResponse(const HttpResponse& response) {
    std::ostringstream out;
    out << "HTTP/1.1 " << response.statusCode << ' '
        << response.statusMessage << "\r\n";
    out << "Connection: " << (response.close ? "close" : "Keep-Alive") << "\r\n";
    out << "Content-Type: " << response.contentType << "\r\n";
    out << "Content-Length: " << response.body.size() << "\r\n";
    out << "\r\n";
    out << response.body;
    return out.str();
}

}  // namespace csl::asio
