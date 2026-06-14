// Copyright 2026, cpp-server-lab
// WebServer 配置加载与校验。

#pragma once

#include <cstddef>
#include <string>

namespace csl::asio {

// 日志输出配置。
struct LogConfig {
    std::string level = "info"; // trace/debug/info/warn/error/off
    std::string file = "logs/server.log";
    bool console = true;
};

// HTTP 服务运行配置。
//
// 配置来源为 INI 文件；缺失字段使用这里的默认值。
struct HttpServerConfig {
    unsigned short port = 8080;
    std::size_t threadCount = 2;
    std::string documentRoot = "public";
    std::string indexFile = "index.html";
    std::size_t keepAliveTimeoutMs = 30000;
    std::size_t maxHeaderBytes = 8192;
    std::size_t maxBodyBytes = 1024 * 1024;
    LogConfig log;
};

// 从 INI 文件加载 WebServer 配置。
//
// 如果配置文件不存在，返回默认配置；如果字段存在但格式非法，抛出 std::runtime_error。
HttpServerConfig loadServerConfig(const std::string& path);

// 创建运行时依赖目录，例如静态资源目录和日志目录。
void prepareRuntimeDirectories(const HttpServerConfig& config);

}  // namespace csl::asio
