// Copyright 2026, cpp-server-lab
// 轻量跨平台日志模块。

#pragma once

#include "csl/asio/server_config.h"

#include <string>
#include <string_view>

namespace csl::asio {

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Off = 5,
};

// 解析日志级别字符串。
//
// 支持 trace/debug/info/warn/error/off，大小写不敏感。
LogLevel parseLogLevel(std::string_view value);

// 使用配置初始化全局日志输出。
//
// 该函数会打开配置中的日志文件；多次调用会替换原有日志文件句柄。
void initializeLogger(const LogConfig& config);

// 输出一条日志。
//
// 线程安全；如果 level 低于全局日志级别，则直接丢弃。
void logMessage(LogLevel level, std::string_view message);

// 将日志级别转为固定文本。
std::string_view logLevelName(LogLevel level);

}  // namespace csl::asio
