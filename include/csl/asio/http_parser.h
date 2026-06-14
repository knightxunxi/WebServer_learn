// Copyright 2026, cpp-server-lab
// HTTP/1.x 请求头解析工具。

#pragma once

#include "csl/asio/http_server.h"
#include "csl/asio/server_config.h"

#include <string>

namespace csl::asio {

enum class HttpParseStatus {
    Ok,
    BadRequest,
    HeaderTooLarge,
    BodyTooLarge,
    UnsupportedMediaType,
};

struct HttpParseResult {
    HttpParseStatus status = HttpParseStatus::Ok;
    HttpRequest request;
    std::string message;
};

// 解析 HTTP 请求头。
//
// rawHeaders 必须包含请求行和头部，不包含末尾空行后的 body。
HttpParseResult parseHttpRequestHead(const std::string& rawHeaders,
                                     const HttpServerConfig& config);

// 校验并写入请求体。
//
// 第一版只支持 text/plain、application/json、application/x-www-form-urlencoded。
HttpParseStatus assignRequestBody(HttpRequest* request,
                                  std::string body,
                                  const HttpServerConfig& config);

// 返回解析错误对应的 HTTP 状态码。
int statusCodeForParseError(HttpParseStatus status);

// 返回状态码对应的标准描述。
std::string reasonPhrase(int statusCode);

}  // namespace csl::asio
