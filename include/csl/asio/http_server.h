// Copyright 2026, cpp-server-lab
// Boost.Asio HttpServer - 跨平台异步 HTTP 服务

#pragma once

#include <boost/asio.hpp>

#include "csl/asio/server_config.h"

#include <cstddef>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace csl::asio {

// HTTP 请求解析结果。
//
// 字段保持公开，便于学习和测试；同时提供 header() 这类只读辅助函数。
struct HttpRequest {
    std::string method; // 请求方法，例如 GET/HEAD/POST。
    std::string target; // 原始请求目标，例如 /search?q=csl。
    std::string path; // 不含 query 的路径，例如 /index.html。
    std::string query; // query string，不含问号。
    std::string version; // HTTP 版本，例如 HTTP/1.1。
    std::unordered_map<std::string, std::string> headers; // 小写 header key 到 value 的映射。
    std::string body; // 请求体。
    std::size_t contentLength = 0; // Content-Length 解析结果。
    std::string contentType; // Content-Type 解析结果。
    bool keepAlive = false; // 当前请求是否希望保持连接。

    // 获取指定请求头。
    //
    // name 大小写不敏感；不存在时返回空字符串。
    std::string header(const std::string& name) const;

    // 判断指定请求头是否存在。
    bool hasHeader(const std::string& name) const;
};

// HTTP 响应对象。
//
// 应用层回调通过修改该对象来设置状态码、响应头和响应体。
struct HttpResponse {
    int statusCode = 200; // HTTP 状态码。
    std::string statusMessage = "OK"; // 状态描述。
    std::string contentType = "text/html; charset=utf-8"; // 响应内容类型。
    std::unordered_map<std::string, std::string> headers; // 额外响应头。
    std::string body; // 响应体。
    bool close = false; // 响应后是否主动关闭连接。

    // 设置状态码和描述。
    void setStatus(int code, std::string message);

    // 设置响应头。
    void setHeader(std::string name, std::string value);

    // 设置普通文本/HTML/JSON 响应体。
    void setBody(std::string data, std::string type);
};

// 基于 Boost.Asio 的异步 HTTP 服务。
//
// HttpServer 负责监听端口、接收连接、读取 HTTP 请求头，并通过 RequestHandler
// 将请求分发给应用层生成响应。当前实现定位为学习版 HTTP/1.1 服务骨架。
class HttpServer {
public:
    using RequestHandler = std::function<void(const HttpRequest&, HttpResponse*)>;

    // 使用配置创建 HTTP 服务。
    explicit HttpServer(HttpServerConfig config);

    // 使用端口和线程数创建 HTTP 服务。
    //
    // port：监听端口。
    // threadCount：运行 io_context 的线程数，传入 0 时会修正为 1。
    explicit HttpServer(unsigned short port, std::size_t threadCount = 1);

    // 停止服务并等待内部工作线程退出。
    ~HttpServer();

    // 设置应用层请求处理函数。
    //
    // handler 在 IO 回调中被调用，应该避免长时间阻塞。
    void setRequestHandler(RequestHandler handler);

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
    HttpServerConfig config_;
    std::vector<std::thread> threads_;
    RequestHandler handler_;
};

// 将 HttpResponse 序列化为 HTTP/1.1 文本响应。
//
// 当前版本会生成状态行、Connection、Content-Type、Content-Length 和响应体。
std::string serializeResponse(const HttpResponse& response, bool skipBody = false);

}  // namespace csl::asio
