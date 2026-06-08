// Copyright 2026, cpp-server-lab
// Logger - 带时间戳和级别的日志系统
//
// 设计意图：
//   提供统一的日志输出能力，支持 6 级日志级别和自定义输出回调。
//   使用 Pimpl 模式隐藏实现细节（Timestamp、ostringstream 等），
//   避免头文件污染。通过宏简化调用，自动捕获文件名和行号。
//
// 参考：muduo/base/Logging.h
// 差异：
//   - 使用 std::ostringstream + Pimpl 替代 muduo 的 LogStream + 固定栈缓冲
//   - 默认输出到 stderr（而非 stdout）
//   - 不使用 muduo 的 CurrentThread 工具类

#pragma once

#include <memory>
#include <sstream>

namespace csl {

/// @brief 日志级别
///
/// TRACE < DEBUG < INFO < WARN < ERROR < FATAL
/// TRACE/DEBUG/INFO 受全局级别过滤，WARN/ERROR/FATAL 无条件输出。
/// FATAL 级别在析构时调用 std::abort()。
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5,
};

/// @brief 日志输出回调
/// @param msg 日志消息
/// @param len 消息长度
using OutputFunc = void (*)(const char* msg, int len);

/// @brief 日志刷新回调
using FlushFunc = void (*)();

/// @brief 日志器
///
/// 使用 RAII：构造时记录元信息，<< 流式写入内容，析构时格式化并输出。
/// 通常不直接使用，而是通过 LOG_* 宏调用。
class Logger {
public:
    /// @brief 构造日志器
    /// @param file  源文件名
    /// @param line  行号
    /// @param level 日志级别
    /// @param func  函数名（TRACE/DEBUG 级别使用，可选）
    Logger(const char* file, int line, LogLevel level,
           const char* func = nullptr);

    /// @brief 析构时格式化并输出完整日志行
    ~Logger();

    /// @brief 获取底层流，用于链式写入日志内容
    std::ostringstream& stream();

    /// @brief 设置全局日志级别
    static void setLogLevel(LogLevel level);

    /// @brief 获取全局日志级别
    static LogLevel logLevel();

    /// @brief 设置自定义输出回调（默认 fwrite 到 stderr）
    static void setOutput(OutputFunc func);

    /// @brief 设置自定义刷新回调（默认 fflush(stderr)）
    static void setFlush(FlushFunc func);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace csl

// ===== 日志宏 =====
//
// TRACE/DEBUG/INFO 受全局级别过滤，只有当 logLevel() <= 对应级别时才执行。
// WARN/ERROR/FATAL 无条件输出。
//
// TRACE 和 DEBUG 携带 __func__ 信息，INFO 及以上不带。

#define LOG_TRACE \
    if (csl::Logger::logLevel() <= csl::LogLevel::TRACE) \
        csl::Logger(__FILE__, __LINE__, csl::LogLevel::TRACE, __func__).stream()

#define LOG_DEBUG \
    if (csl::Logger::logLevel() <= csl::LogLevel::DEBUG) \
        csl::Logger(__FILE__, __LINE__, csl::LogLevel::DEBUG, __func__).stream()

#define LOG_INFO \
    if (csl::Logger::logLevel() <= csl::LogLevel::INFO) \
        csl::Logger(__FILE__, __LINE__, csl::LogLevel::INFO).stream()

#define LOG_WARN \
    csl::Logger(__FILE__, __LINE__, csl::LogLevel::WARN).stream()

#define LOG_ERROR \
    csl::Logger(__FILE__, __LINE__, csl::LogLevel::ERROR).stream()

#define LOG_FATAL \
    csl::Logger(__FILE__, __LINE__, csl::LogLevel::FATAL).stream()
