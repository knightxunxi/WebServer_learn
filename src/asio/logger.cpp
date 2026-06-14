// Copyright 2026, cpp-server-lab

#include "csl/asio/logger.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace csl::asio {

namespace {

struct LoggerState {
    LogLevel level = LogLevel::Info;
    bool console = true;
    std::ofstream file;
    std::mutex mutex;
};

LoggerState& state() {
    static LoggerState logger;
    return logger;
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string formatNow() {
    auto now = std::chrono::system_clock::now();
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now - seconds).count();
    std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(6) << std::setfill('0') << micros;
    return out.str();
}

}  // namespace

LogLevel parseLogLevel(std::string_view value) {
    std::string normalized = lower(std::string(value));
    if (normalized == "trace") {
        return LogLevel::Trace;
    }
    if (normalized == "debug") {
        return LogLevel::Debug;
    }
    if (normalized == "info") {
        return LogLevel::Info;
    }
    if (normalized == "warn" || normalized == "warning") {
        return LogLevel::Warn;
    }
    if (normalized == "error") {
        return LogLevel::Error;
    }
    if (normalized == "off") {
        return LogLevel::Off;
    }
    throw std::runtime_error("未知日志级别: " + std::string(value));
}

void initializeLogger(const LogConfig& config) {
    LoggerState& logger = state();
    std::lock_guard<std::mutex> lock(logger.mutex);

    logger.level = parseLogLevel(config.level);
    logger.console = config.console;
    logger.file.close();

    if (!config.file.empty()) {
        std::filesystem::path path(config.file);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        logger.file.open(path, std::ios::app);
        if (!logger.file.is_open()) {
            throw std::runtime_error("无法打开日志文件: " + config.file);
        }
    }
}

void logMessage(LogLevel level, std::string_view message) {
    LoggerState& logger = state();
    if (level < logger.level || logger.level == LogLevel::Off) {
        return;
    }

    std::ostringstream line;
    line << formatNow()
         << " [" << logLevelName(level) << ']'
         << " [tid=" << std::hash<std::thread::id>{}(std::this_thread::get_id()) << "] "
         << message << '\n';

    std::lock_guard<std::mutex> lock(logger.mutex);
    if (logger.console) {
        std::cerr << line.str();
    }
    if (logger.file.is_open()) {
        logger.file << line.str();
        logger.file.flush();
    }
}

std::string_view logLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warn: return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Off: return "OFF";
    }
    return "UNKNOWN";
}

}  // namespace csl::asio
