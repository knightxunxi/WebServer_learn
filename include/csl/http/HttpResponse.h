// Copyright 2026, cpp-server-lab
// HttpResponse - HTTP 响应
//
// 设计意图：
//   构建 HTTP/1.1 响应，包含状态行、头部和 body。
//   支持 Content-Type、Content-Length、Connection 等常用头部。
//   通过 appendToBuffer() 序列化到 Buffer（或 std::string）。

#pragma once

#include <map>
#include <string>

namespace csl {

class Buffer;

class HttpResponse {
public:
    enum HttpStatusCode {
        k200Ok = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
    };

    explicit HttpResponse(bool close = false)
        : statusCode_(k200Ok), closeConnection_(close) {}

    // ---- 设置器 ----
    void setStatusCode(HttpStatusCode code) { statusCode_ = code; }
    void setStatusMessage(const std::string& msg) { statusMessage_ = msg; }
    void setCloseConnection(bool on) { closeConnection_ = on; }
    bool closeConnection() const { return closeConnection_; }
    void setBody(const std::string& body);
    void addHeader(const std::string& key, const std::string& value);
    void setContentType(const std::string& type);

    // ---- 序列化 ----
    /// 将完整 HTTP 响应序列化到 Buffer
    void appendToBuffer(Buffer* output) const;

private:
    HttpStatusCode statusCode_;
    std::string statusMessage_;
    std::map<std::string, std::string> headers_;
    std::string body_;
    bool closeConnection_;
};

}  // namespace csl
