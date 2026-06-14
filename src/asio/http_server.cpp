// Copyright 2026, cpp-server-lab

#include "csl/asio/http_server.h"

#include "csl/asio/http_parser.h"
#include "csl/asio/logger.h"

#include <boost/asio/buffers_iterator.hpp>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <exception>
#include <memory>
#include <sstream>
#include <utility>

namespace csl::asio {

using boost::asio::ip::tcp;

namespace {

HttpServerConfig makeConfig(unsigned short port, std::size_t threadCount) {
    HttpServerConfig config;
    config.port = port;
    config.threadCount = threadCount;
    return config;
}

std::string makeErrorBody(int statusCode, const std::string& message) {
    return "<!DOCTYPE html>\n"
           "<html lang=\"zh-CN\">\n"
           "<head><meta charset=\"UTF-8\"><title>" + std::to_string(statusCode) + "</title></head>\n"
           "<body><h1>" + std::to_string(statusCode) + " " + reasonPhrase(statusCode)
           + "</h1><p>" + message + "</p></body>\n"
           "</html>\n";
}

HttpResponse makeParseErrorResponse(HttpParseStatus status, const std::string& message) {
    int statusCode = statusCodeForParseError(status);
    HttpResponse response;
    response.setStatus(statusCode, reasonPhrase(statusCode));
    response.setBody(makeErrorBody(statusCode, message), "text/html; charset=utf-8");
    response.close = true;
    return response;
}

std::string toString(std::size_t value) {
    return std::to_string(static_cast<unsigned long long>(value));
}

}  // namespace

void HttpResponse::setStatus(int code, std::string message) {
    statusCode = code;
    statusMessage = std::move(message);
}

void HttpResponse::setHeader(std::string name, std::string value) {
    headers[std::move(name)] = std::move(value);
}

void HttpResponse::setBody(std::string data, std::string type) {
    body = std::move(data);
    contentType = std::move(type);
}

// 单个 HTTP 连接的会话对象。
//
// Session 负责请求读取、超时控制、响应写回和 Keep-Alive 循环。
class HttpServer::Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, RequestHandler handler, HttpServerConfig config)
        : socket_(std::move(socket))
        , timer_(socket_.get_executor())
        , buffer_(config.maxHeaderBytes + config.maxBodyBytes + 4)
        , handler_(std::move(handler))
        , config_(std::move(config)) {}

    // 启动当前连接的 HTTP 请求读取流程。
    void start() {
        doReadHead();
    }

private:
    // 刷新连接超时计时器。
    //
    // 每次读写前都会重新设置超时时间，超时后关闭 socket。
    void armTimer() {
        auto self = shared_from_this();
        timer_.expires_after(std::chrono::milliseconds(config_.keepAliveTimeoutMs));
        timer_.async_wait([this, self](const boost::system::error_code& ec) {
            if (!ec) {
                logMessage(LogLevel::Warn, "连接超时，主动关闭");
                close();
            }
        });
    }

    // 读取一个完整 HTTP 头部。
    void doReadHead() {
        requestStartedAt_ = std::chrono::steady_clock::now();
        armTimer();

        auto self = shared_from_this();
        boost::asio::async_read_until(
            socket_,
            buffer_,
            "\r\n\r\n",
            [this, self](const boost::system::error_code& ec, std::size_t length) {
                if (ec) {
                    if (ec != boost::asio::error::operation_aborted) {
                        logMessage(LogLevel::Debug, "读取 HTTP 头部失败: " + ec.message());
                    }
                    close();
                    return;
                }
                handleHead(length);
            });
    }

    // 解析头部，并根据 Content-Length 决定是否继续读取 body。
    void handleHead(std::size_t length) {
        if (length > config_.maxHeaderBytes) {
            writeResponse(HttpRequest{}, makeParseErrorResponse(
                HttpParseStatus::HeaderTooLarge, "请求头超过限制"));
            return;
        }

        std::string rawHeaders(
            boost::asio::buffers_begin(buffer_.data()),
            boost::asio::buffers_begin(buffer_.data()) + static_cast<std::ptrdiff_t>(length));
        buffer_.consume(length);

        HttpParseResult parsed = parseHttpRequestHead(rawHeaders, config_);
        if (parsed.status != HttpParseStatus::Ok) {
            writeResponse(parsed.request, makeParseErrorResponse(parsed.status, parsed.message));
            return;
        }

        HttpRequest request = std::move(parsed.request);
        std::string body;
        std::size_t alreadyBuffered = std::min<std::size_t>(buffer_.size(), request.contentLength);
        if (alreadyBuffered > 0) {
            body.assign(
                boost::asio::buffers_begin(buffer_.data()),
                boost::asio::buffers_begin(buffer_.data()) + static_cast<std::ptrdiff_t>(alreadyBuffered));
            buffer_.consume(alreadyBuffered);
        }

        if (body.size() < request.contentLength) {
            doReadBody(std::move(request), std::move(body));
            return;
        }

        handleRequest(std::move(request), std::move(body));
    }

    // 按 Content-Length 补齐请求体。
    void doReadBody(HttpRequest request, std::string body) {
        armTimer();

        std::size_t remaining = request.contentLength - body.size();
        auto self = shared_from_this();
        boost::asio::async_read(
            socket_,
            buffer_,
            boost::asio::transfer_exactly(remaining),
            [this, self, request = std::move(request), body = std::move(body)](
                const boost::system::error_code& ec, std::size_t length) mutable {
                if (ec) {
                    if (ec != boost::asio::error::operation_aborted) {
                        logMessage(LogLevel::Debug, "读取 HTTP body 失败: " + ec.message());
                    }
                    close();
                    return;
                }

                body.append(
                    boost::asio::buffers_begin(buffer_.data()),
                    boost::asio::buffers_begin(buffer_.data()) + static_cast<std::ptrdiff_t>(length));
                buffer_.consume(length);
                handleRequest(std::move(request), std::move(body));
            });
    }

    // 执行业务处理函数并写回响应。
    void handleRequest(HttpRequest request, std::string body) {
        HttpParseStatus bodyStatus = assignRequestBody(&request, std::move(body), config_);
        if (bodyStatus != HttpParseStatus::Ok) {
            writeResponse(request, makeParseErrorResponse(bodyStatus, "请求体不合法"));
            return;
        }

        HttpResponse response;
        response.close = !request.keepAlive;

        try {
            if (handler_) {
                handler_(request, &response);
            } else {
                response.setStatus(404, reasonPhrase(404));
                response.setBody(makeErrorBody(404, "未注册请求处理函数"), "text/html; charset=utf-8");
            }
        } catch (const std::exception& ex) {
            logMessage(LogLevel::Error, std::string("请求处理异常: ") + ex.what());
            response.setStatus(500, reasonPhrase(500));
            response.setBody(makeErrorBody(500, "服务器内部错误"), "text/html; charset=utf-8");
            response.close = true;
        }

        if (response.statusMessage.empty()) {
            response.statusMessage = reasonPhrase(response.statusCode);
        }
        writeResponse(request, std::move(response));
    }

    // 异步写回响应，并根据 close/Keep-Alive 决定下一步。
    void writeResponse(const HttpRequest& request, HttpResponse response) {
        armTimer();

        bool skipBody = request.method == "HEAD";
        auto data = std::make_shared<std::string>(serializeResponse(response, skipBody));
        bool closeAfterWrite = response.close;
        logAccess(request, response, data->size());

        auto self = shared_from_this();
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(*data),
            [this, self, data, closeAfterWrite](
                const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    if (ec != boost::asio::error::operation_aborted) {
                        logMessage(LogLevel::Debug, "写 HTTP 响应失败: " + ec.message());
                    }
                    close();
                    return;
                }

                if (!closeAfterWrite) {
                    doReadHead();
                    return;
                }
                close();
            });
    }

    void logAccess(const HttpRequest& request,
                   const HttpResponse& response,
                   std::size_t bytes) const {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - requestStartedAt_);

        std::string target = request.target.empty() ? "-" : request.target;
        std::string method = request.method.empty() ? "-" : request.method;
        logMessage(LogLevel::Info,
                   method + " " + target + " -> "
                   + std::to_string(response.statusCode)
                   + " bytes=" + toString(bytes)
                   + " cost_ms=" + toString(static_cast<std::size_t>(elapsed.count())));
    }

    void close() {
        boost::system::error_code ignored;
        timer_.cancel();
        socket_.shutdown(tcp::socket::shutdown_both, ignored);
        socket_.close(ignored);
    }

    tcp::socket socket_;
    boost::asio::steady_timer timer_;
    boost::asio::streambuf buffer_;
    RequestHandler handler_;
    HttpServerConfig config_;
    std::chrono::steady_clock::time_point requestStartedAt_;
};

HttpServer::HttpServer(HttpServerConfig config)
    : ioContext_(static_cast<int>(std::max<std::size_t>(1, config.threadCount)))
    , acceptor_(ioContext_, tcp::endpoint(tcp::v4(), config.port))
    , signals_(ioContext_, SIGINT, SIGTERM)
    , config_(std::move(config))
{
    config_.threadCount = std::max<std::size_t>(1, config_.threadCount);
    prepareRuntimeDirectories(config_);
    initializeLogger(config_.log);

    signals_.async_wait([this](const boost::system::error_code&, int) {
        stop();
    });

    logMessage(LogLevel::Info,
               "WebServer 启动配置: port=" + std::to_string(config_.port)
               + " threads=" + std::to_string(config_.threadCount)
               + " document_root=" + config_.documentRoot);
}

HttpServer::HttpServer(unsigned short port, std::size_t threadCount)
    : HttpServer(makeConfig(port, threadCount)) {}

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

    for (std::size_t i = 1; i < config_.threadCount; ++i) {
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
                std::make_shared<Session>(std::move(socket), handler_, config_)->start();
            } else if (acceptor_.is_open()) {
                logMessage(LogLevel::Warn, "accept 失败: " + ec.message());
            }

            if (acceptor_.is_open()) {
                doAccept();
            }
        });
}

std::string serializeResponse(const HttpResponse& response, bool skipBody) {
    std::ostringstream out;
    out << "HTTP/1.1 " << response.statusCode << ' '
        << response.statusMessage << "\r\n";
    out << "Connection: " << (response.close ? "close" : "keep-alive") << "\r\n";
    out << "Content-Type: " << response.contentType << "\r\n";
    out << "Content-Length: " << response.body.size() << "\r\n";
    for (const auto& [key, value] : response.headers) {
        out << key << ": " << value << "\r\n";
    }
    out << "\r\n";
    if (!skipBody) {
        out << response.body;
    }
    return out.str();
}

}  // namespace csl::asio
