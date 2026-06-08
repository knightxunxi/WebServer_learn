// Copyright 2026, cpp-server-lab
// HttpContext - 单连接的 HTTP 解析状态
//
// 设计意图：
//   维护单个 TCP 连接上的 HTTP 协议解析状态。
//   支持分次到达的数据（通过 parseRequest 逐步解析）。
//
// 解析流程：
//   1. expectRequestLine → 解析请求行
//   2. expectHeaders → 解析头部
//   3. expectBody → 解析 body（如有 Content-Length）
//   4. gotAll → 解析完成

#pragma once

#include "csl/http/HttpRequest.h"

namespace csl {

class Buffer;

class HttpContext {
public:
    enum ParseState {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

    HttpContext() : state_(kExpectRequestLine) {}

    /// @brief 从 Buffer 解析数据，返回是否完整解析了一个请求
    /// @param buf 输入缓冲（解析后会 consume 已处理的数据）
    bool parseRequest(Buffer* buf);

    /// @brief 重置解析状态（用于连接复用 / Keep-Alive）
    void reset();

    const HttpRequest& request() const { return request_; }

    ParseState state() const { return state_; }

private:
    bool processRequestLine(const char* begin, const char* end);

    ParseState state_;
    HttpRequest request_;
};

}  // namespace csl
