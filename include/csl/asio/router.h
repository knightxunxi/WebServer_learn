// Copyright 2026, cpp-server-lab
// WebServer 请求路由与静态文件处理。

#pragma once

#include "csl/asio/http_server.h"
#include "csl/asio/server_config.h"

#include <filesystem>
#include <optional>
#include <string>

namespace csl::asio {

// 根据文件扩展名推断 MIME 类型。
std::string guessMimeType(const std::filesystem::path& path);

// URL path 解码。
//
// 成功时返回解码后的路径；遇到非法百分号编码时返回 std::nullopt。
std::optional<std::string> urlDecodePath(const std::string& path);

// 配置驱动 Router。
//
// Router 负责内置 API 和静态文件映射。当前版本不提供动态注册接口，
// 静态资源根目录和默认首页由 HttpServerConfig 决定。
class Router {
public:
    explicit Router(HttpServerConfig config);

    // 根据请求生成响应。
    //
    // 支持 GET/HEAD 静态文件、GET /api/status 和 POST /api/echo。
    void handle(const HttpRequest& request, HttpResponse* response) const;

private:
    void handleStatus(const HttpRequest& request, HttpResponse* response) const;
    void handleEcho(const HttpRequest& request, HttpResponse* response) const;
    void handleStaticFile(const HttpRequest& request, HttpResponse* response) const;

    std::optional<std::filesystem::path> resolveStaticPath(const std::string& requestPath) const;
    void setError(HttpResponse* response, int statusCode, std::string message) const;

    HttpServerConfig config_;
    std::filesystem::path documentRoot_;
};

}  // namespace csl::asio
