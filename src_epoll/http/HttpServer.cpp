// Copyright 2026, cpp-server-lab
// HttpResponse 实现 + HttpContext + HttpServer

#include "csl/http/HttpResponse.h"
#include "csl/http/HttpContext.h"
#include "csl/http/HttpServer.h"
#include "csl/http/HttpRequest.h"
#include "csl/net/Buffer.h"
#include "csl/net/TcpConnection.h"

#include <algorithm>
#include <any>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <string>

namespace csl {

namespace {

const char kCRLF[] = "\r\n";

const char* findCRLF(const Buffer* buf) {
    const char* begin = buf->peek();
    const char* end = begin + buf->readableBytes();
    const char* crlf = std::search(begin, end, kCRLF, kCRLF + 2);
    return crlf == end ? nullptr : crlf;
}

bool shouldCloseConnection(const HttpRequest& req) {
    std::string connection = req.getHeader("Connection");
    return connection == "close" ||
           (req.version() == HttpRequest::kHttp10 &&
            connection != "Keep-Alive");
}

}  // namespace

// ===== HttpResponse =====

void HttpResponse::setBody(const std::string& body) {
    body_ = body;
}

void HttpResponse::addHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpResponse::setContentType(const std::string& type) {
    addHeader("Content-Type", type);
}

void HttpResponse::appendToBuffer(Buffer* output) const {
    char buf[64];

    // 状态行
    snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", static_cast<int>(statusCode_));
    output->append(buf);
    output->append(statusMessage_);
    output->append("\r\n");

    // 头部
    if (closeConnection_) {
        output->append("Connection: close\r\n");
    } else {
        output->append("Connection: Keep-Alive\r\n");
    }

    if (!body_.empty()) {
        snprintf(buf, sizeof(buf), "Content-Length: %zu\r\n", body_.size());
        output->append(buf);

        if (headers_.find("Content-Type") == headers_.end()) {
            output->append("Content-Type: text/html; charset=utf-8\r\n");
        }
    }

    for (const auto& header : headers_) {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    output->append("\r\n");

    // Body
    if (!body_.empty()) {
        output->append(body_);
    }
}

// ===== HttpContext =====

bool HttpContext::parseRequest(Buffer* buf) {
    bool hasMore = true;
    while (hasMore) {
        if (state_ == kExpectRequestLine) {
            const char* crlf = findCRLF(buf);
            if (crlf) {
                bool ok = processRequestLine(buf->peek(), crlf);
                buf->retrieveUntil(crlf + 2);
                if (ok) {
                    state_ = kExpectHeaders;
                } else {
                    return false; // 解析失败
                }
            } else {
                hasMore = false; // 数据不够，等待更多
            }
        } else if (state_ == kExpectHeaders) {
            const char* crlf = findCRLF(buf);
            if (crlf) {
                const char* start = buf->peek();

                if (crlf == start) {
                    // 空行表示 header 结束。第一阶段暂不解析 body。
                    buf->retrieveUntil(crlf + 2);
                    state_ = kGotAll;
                    return true;
                }

                const char* colon = std::find(start, crlf, ':');
                if (colon != crlf) {
                    std::string key(start, colon - start);
                    const char* valueStart = colon + 1;
                    while (valueStart < crlf &&
                           (*valueStart == ' ' || *valueStart == '\t')) {
                        ++valueStart;
                    }
                    const char* valueEnd = crlf;
                    while (valueEnd > valueStart &&
                           (*(valueEnd - 1) == ' ' || *(valueEnd - 1) == '\t')) {
                        --valueEnd;
                    }
                    request_.addHeader(key, std::string(valueStart, valueEnd));
                }

                buf->retrieveUntil(crlf + 2);
            } else {
                hasMore = false;
            }
        } else if (state_ == kGotAll) {
            return false; // 已完整，不要重复解析
        }
    }
    return false;
}

bool HttpContext::processRequestLine(const char* begin, const char* end) {
    // 格式: METHOD SP PATH SP VERSION
    const char* start = begin;

    // 解析 METHOD
    const char* space = static_cast<const char*>(
        memchr(start, ' ', end - start));
    if (!space) return false;

    std::string methodStr(start, space - start);
    if (methodStr == "GET") {
        request_.setMethod(HttpRequest::kGet);
    } else if (methodStr == "POST") {
        request_.setMethod(HttpRequest::kPost);
    } else if (methodStr == "HEAD") {
        request_.setMethod(HttpRequest::kHead);
    } else {
        request_.setMethod(HttpRequest::kInvalid);
    }

    // 解析 PATH（可能包含 ?query）
    start = space + 1;
    space = static_cast<const char*>(memchr(start, ' ', end - start));
    if (!space) return false;

    std::string pathStr(start, space - start);
    auto qpos = pathStr.find('?');
    if (qpos != std::string::npos) {
        request_.setPath(pathStr.substr(0, qpos));
        request_.setQuery(pathStr.substr(qpos + 1));
    } else {
        request_.setPath(pathStr);
    }

    // 解析 VERSION
    start = space + 1;
    std::string verStr(start, end - start);
    if (verStr == "HTTP/1.1") {
        request_.setVersion(HttpRequest::kHttp11);
    } else if (verStr == "HTTP/1.0") {
        request_.setVersion(HttpRequest::kHttp10);
    } else {
        request_.setVersion(HttpRequest::kUnknown);
    }

    return true;
}

void HttpContext::reset() {
    state_ = kExpectRequestLine;
    request_ = HttpRequest();
}

// ===== HttpServer =====

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const std::string& name)
    : server_(loop, listenAddr, name)
{
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));
}

HttpServer::~HttpServer() = default;

void HttpServer::setThreadNum(int numThreads) {
    server_.setThreadNum(numThreads);
}

void HttpServer::setHttpCallback(const HttpCallback& cb) {
    httpCallback_ = cb;
}

void HttpServer::start() {
    server_.start();
}

void HttpServer::onConnection(const TcpServer::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        // 为每个连接创建 HttpContext
        conn->setContext(std::make_shared<HttpContext>());
    }
}

void HttpServer::onMessage(const TcpServer::TcpConnectionPtr& conn,
                            Buffer* buf,
                            Timestamp receiveTime) {
    (void)receiveTime;

    // 获取或创建 HttpContext
    std::shared_ptr<HttpContext> context;
    try {
        context = std::any_cast<std::shared_ptr<HttpContext>>(conn->getContext());
    } catch (const std::bad_any_cast&) {
        context = std::make_shared<HttpContext>();
        conn->setContext(context);
    }

    while (buf->readableBytes() > 0) {
        if (!context->parseRequest(buf)) {
            break; // 数据不完整，等待下一次读事件
        }

        onRequest(conn, context->request());
        context->reset();
    }
}

void HttpServer::onRequest(const TcpServer::TcpConnectionPtr& conn,
                           const HttpRequest& req) {
    const bool close = shouldCloseConnection(req);
    HttpResponse resp(close);

    if (req.method() == HttpRequest::kInvalid ||
        req.version() == HttpRequest::kUnknown) {
        resp.setStatusCode(HttpResponse::k400BadRequest);
        resp.setStatusMessage("Bad Request");
        resp.setCloseConnection(true);
        resp.setBody("<h1>400 Bad Request</h1>");
    } else if (httpCallback_) {
        httpCallback_(req, &resp);
    } else {
        resp.setStatusCode(HttpResponse::k404NotFound);
        resp.setStatusMessage("Not Found");
        resp.setBody("<h1>404 Not Found</h1>");
    }

    Buffer output;
    resp.appendToBuffer(&output);
    conn->send(output.peek(), output.readableBytes());

    if (resp.closeConnection()) {
        conn->shutdown();
    }
}

}  // namespace csl
