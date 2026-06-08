// Copyright 2026, cpp-server-lab
// Logger 实现
//
// 架构：
//   - Pimpl 模式，Impl 负责所有格式化逻辑
//   - 默认输出到 stderr，可替换为自定义 OutputFunc/FlushFunc
//   - 输出格式：YYYYMMDD HH:MM:SS.uuuuuu TID LEVEL MSG - filename:line\n
//
// 线程 ID 策略：
//   使用 std::hash<std::thread::id> 生成数值型线程标识，
//   具备跨平台可移植性。
//
// 平台依赖：
//   - Timestamp::now() 依赖 POSIX gettimeofday()

#include "csl/base/logger.h"
#include "csl/base/timestamp.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <thread>

namespace csl {

// ===== 全局状态 =====

namespace {
    // 默认日志级别为 INFO
    LogLevel g_logLevel = LogLevel::INFO;

    // 默认输出函数：写入 stderr
    void defaultOutput(const char* msg, int len) {
        fwrite(msg, 1, static_cast<size_t>(len), stderr);
    }

    // 默认刷新函数：刷新 stderr
    void defaultFlush() {
        fflush(stderr);
    }

    OutputFunc g_output = defaultOutput;
    FlushFunc g_flush = defaultFlush;
}  // anonymous namespace

// ===== 级别名称映射 =====

// 6 字符定宽级别名称，用于格式化输出
static const char* logLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE ";
        case LogLevel::DEBUG: return "DEBUG ";
        case LogLevel::INFO:  return "INFO  ";
        case LogLevel::WARN:  return "WARN  ";
        case LogLevel::ERROR: return "ERROR ";
        case LogLevel::FATAL: return "FATAL ";
    }
    return "UNKNOW";
}

// ===== Logger::Impl =====

class Logger::Impl {
public:
    Impl(const char* file, int line, LogLevel level, const char* func)
        : file_(file)
        , line_(line)
        , level_(level)
        , func_(func)
        , timestamp_(Timestamp::now()) {
        // 只保留文件名（去掉路径）
        const char* lastSlash = strrchr(file_, '/');
#ifdef _WIN32
        const char* lastBackslash = strrchr(file_, '\\');
        if (lastBackslash && (!lastSlash || lastBackslash > lastSlash)) {
            lastSlash = lastBackslash;
        }
#endif
        if (lastSlash) {
            file_ = lastSlash + 1;
        }
    }

    /// @brief 格式化并输出完整日志行
    void finish() {
        // 1. 时间戳
        std::string timeStr = timestamp_.toFormattedString();

        // 2. 线程 ID（使用 hash 生成数值型标识）
        auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

        // 3. 获取用户写入的日志内容
        std::string msg = stream_.str();

        // 4. 拼接完整日志行
        // 格式：YYYYMMDD HH:MM:SS.uuuuuu TID LEVEL MSG - file:line\n
        char buf[4096];
        int len = 0;

        if (func_) {
            // TRACE/DEBUG 级别：在内容后附上函数名
            len = snprintf(buf, sizeof(buf),
                           "%s %05zu %s %s [%s] - %s:%d\n",
                           timeStr.c_str(),
                           tid % 100000,  // 取后 5 位便于阅读
                           logLevelName(level_),
                           msg.c_str(),
                           func_,
                           file_,
                           line_);
        } else {
            len = snprintf(buf, sizeof(buf),
                           "%s %05zu %s %s - %s:%d\n",
                           timeStr.c_str(),
                           tid % 100000,
                           logLevelName(level_),
                           msg.c_str(),
                           file_,
                           line_);
        }

        // 5. 输出 — 防御性截断，防止 snprintf 截断时 len 超出 buf
        int actualLen = (len < 0) ? 0
                       : (len >= static_cast<int>(sizeof(buf)))
                           ? static_cast<int>(sizeof(buf) - 1)
                           : len;
        g_output(buf, actualLen);
        g_flush();

        // 6. FATAL 级别终止程序
        if (level_ == LogLevel::FATAL) {
            std::abort();
        }
    }

    std::ostringstream& stream() { return stream_; }

private:
    const char* file_;
    int line_;
    LogLevel level_;
    const char* func_;
    Timestamp timestamp_;
    std::ostringstream stream_;
};

// ===== Logger 公开接口 =====

Logger::Logger(const char* file, int line, LogLevel level, const char* func)
    : impl_(std::make_unique<Impl>(file, line, level, func)) {}

Logger::~Logger() {
    impl_->finish();
}

std::ostringstream& Logger::stream() {
    return impl_->stream();
}

void Logger::setLogLevel(LogLevel level) {
    g_logLevel = level;
}

LogLevel Logger::logLevel() {
    return g_logLevel;
}

void Logger::setOutput(OutputFunc func) {
    g_output = func ? func : defaultOutput;
}

void Logger::setFlush(FlushFunc func) {
    g_flush = func ? func : defaultFlush;
}

}  // namespace csl
