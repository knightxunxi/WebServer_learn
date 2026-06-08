// Copyright 2026, cpp-server-lab
// HttpServer - HTTP 服务器
//
// 设计意图：
//   构建在 TcpServer 之上，处理 HTTP 请求/响应。
//   通过 HttpContext 维护每个连接的解析状态。
//   支持 HTTP/1.1 Keep-Alive。

#pragma once

#include "csl/base/noncopyable.h"
#include "csl/net/TcpServer.h"

#include <functional>

namespace csl {

class HttpRequest;
class HttpResponse;
class Buffer;

class HttpServer : noncopyable {
public:
    using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;

    HttpServer(EventLoop* loop,
               const InetAddress& listenAddr,
               const std::string& name);

    ~HttpServer();

    // ---- 配置 ----
    void setThreadNum(int numThreads);
    void setHttpCallback(const HttpCallback& cb);

    // ---- 启动 ----
    void start();

private:
    void onConnection(const TcpServer::TcpConnectionPtr& conn);
    void onMessage(const TcpServer::TcpConnectionPtr& conn, Buffer* buf, Timestamp);
    void onRequest(const TcpServer::TcpConnectionPtr& conn,
                   const HttpRequest& req);

    TcpServer server_;
    HttpCallback httpCallback_;
};

}  // namespace csl
