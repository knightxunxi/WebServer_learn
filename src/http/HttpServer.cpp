// Copyright 2026, cpp-server-lab
// HttpResponse 实现 + HttpContext + HttpServer

#include "csl/http/HttpResponse.h"
#include "csl/http/HttpContext.h"
#include "csl/http/HttpServer.h"
#include "csl/http/HttpRequest.h"
#include "csl/net/Buffer.h"
#include "csl/net/TcpConnection.h"

#include <algorithm>
#include <cstdio>

namespace csl {

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
            const char* crlf = static_cast<const char*>(
                memmem(buf->peek(), buf->readableBytes(), "\r\n", 2));
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
            const char* crlf = static_cast<const char*>(
                memmem(buf->peek(), buf->readableBytes(), "\r\n\r\n", 4));
            if (crlf) {
                // 解析每一行头部
                const char* start = buf->peek();
                const char* end = crlf;
                while (start < end) {
                    const char* colon = static_cast<const char*>(
                        memchr(start, ':', end - start));
                    if (colon) {
                        std::string key(start, colon - start);
                        const char* valueStart = colon + 1;
                        // 跳过空格
                        while (valueStart < end && *valueStart == ' ') {
                            ++valueStart;
                        }
                        std::string value(valueStart, end - valueStart);
                        request_.addHeader(key, value);
                    }
                    start = end + 2; // 跳到下一行
                    // 查找下一行的 \r\n
                    if (start < crlf + 4) {
                        end = static_cast<const char*>(
                            memmem(start, crlf + 4 - start, "\r\n", 2));
                    }
                }

                buf->retrieveUntil(crlf + 4);
                state_ = kGotAll;
                return true;
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
                  std::placeholders::_1, std::placeholders::_2));
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
                            Timestamp receiveTime) {
    // 获取或创建 HttpContext
    std::shared_ptr<HttpContext> context;
    try {
        context = std::any_cast<std::shared_ptr<HttpContext>>(conn->getContext());
    } catch (const std::bad_any_cast&) {
        context = std::make_shared<HttpContext>();
        conn->setContext(context);
    }

    // 注意：inputBuffer() 中的数据会被 parseRequest consume
    // 目前 TcpConnection::inputBuffer_ 是 std::string，不支持 consume
    // 简化方案：直接读取 inputBuffer_ 的全部内容手动解析

    // 临时方案：把 inputBuffer_ 复制到本地 Buffer 解析
    // 里程后续会整合 Buffer 到 TcpConnection

    if (httpCallback_) {
        // 构造简单的请求/响应
        HttpRequest req;
        HttpResponse resp(false);

        // 简易解析（完整解析需要 HttpContext + Buffer）
        std::string& raw = conn->inputBuffer();
        auto space1 = raw.find(' ');
        auto space2 = raw.find(' ', space1 + 1);
        if (space1 != std::string::npos && space2 != std::string::npos) {
            std::string path = raw.substr(space1 + 1, space2 - space1 - 1);
            auto qpos = path.find('?');
            if (qpos != std::string::npos) {
                req.setPath(path.substr(0, qpos));
                req.setQuery(path.substr(qpos + 1));
            } else {
                req.setPath(path);
            }
            req.setMethod(HttpRequest::kGet);
            req.setVersion(HttpRequest::kHttp11);
        }

        httpCallback_(req, &resp);

        Buffer buf;
        resp.appendToBuffer(&buf);
        conn->send(buf.peek(), buf.readableBytes());
        conn->shutdown();
    }
}

}  // namespace csl
