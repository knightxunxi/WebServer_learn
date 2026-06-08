// Logger unit tests
//
// Test coverage:
//   - All log level macros compile and produce output
//   - LOG_INFO output format verification
//   - LOG_TRACE/DEBUG filtered at INFO level
//   - LOG_WARN/ERROR always output regardless of level
//   - setLogLevel() switches levels correctly

#include "csl/base/logger.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Simple global capture mechanism
static std::vector<std::string> g_capturedLines;

static void captureOutput(const char* msg, int len) {
    std::string line(msg, static_cast<size_t>(len));
    if (!line.empty() && line.back() == '\n') {
        line.pop_back();
    }
    g_capturedLines.push_back(line);
    std::cerr << "[CAPTURE] " << line << std::endl;
}

static void setupCapture() {
    g_capturedLines.clear();
    csl::Logger::setOutput(captureOutput);
    csl::Logger::setFlush([]() {});
}

static void restoreOutput() {
    csl::Logger::setOutput(nullptr);
    csl::Logger::setFlush(nullptr);
}

int main() {
    // ===== 1. LOG_INFO output format =====
    {
        setupCapture();
        LOG_INFO << "Hello Logger";
        restoreOutput();

        std::cerr << "[DEBUG] Captured " << g_capturedLines.size() << " lines"
                  << std::endl;
        assert(!g_capturedLines.empty());

        const std::string& line = g_capturedLines.back();

        // Check timestamp format: YYYYMMDD HH:MM:SS.uuuuuu (24 chars)
        assert(line.size() >= 24);
        assert(line[8] == ' ');   // between date and time
        assert(line[11] == ':');  // HH:MM
        assert(line[14] == ':');  // MM:SS
        assert(line[17] == '.');  // SS.uuuuuu

        // Level label
        assert(line.find("INFO  ") != std::string::npos);

        // Log content
        assert(line.find("Hello Logger") != std::string::npos);

        // File:line
        assert(line.find("logger_test.cpp:") != std::string::npos);

        std::cout << "[PASS] LOG_INFO format verified" << std::endl;
    }

    // ===== 2. All log levels work =====
    {
        csl::Logger::setLogLevel(csl::LogLevel::TRACE);
        setupCapture();

        LOG_TRACE << "trace msg";
        LOG_DEBUG << "debug msg";
        LOG_INFO  << "info msg";
        LOG_WARN  << "warn msg";
        LOG_ERROR << "error msg";

        restoreOutput();
        csl::Logger::setLogLevel(csl::LogLevel::INFO);

        assert(g_capturedLines.size() == 5);
        std::cout << "[PASS] All 5 levels produce output" << std::endl;
    }

    // ===== 3. LOG_TRACE/DEBUG filtered at INFO level =====
    {
        csl::Logger::setLogLevel(csl::LogLevel::INFO);
        setupCapture();

        LOG_TRACE << "should not appear";
        LOG_DEBUG << "should not appear either";

        restoreOutput();

        assert(g_capturedLines.empty());
        std::cout << "[PASS] LOG_TRACE/DEBUG filtered at INFO level"
                  << std::endl;
    }

    // ===== 4. LOG_WARN/ERROR always output =====
    {
        csl::Logger::setLogLevel(csl::LogLevel::INFO);
        setupCapture();

        LOG_WARN << "warning msg";
        LOG_ERROR << "error msg";

        restoreOutput();

        assert(g_capturedLines.size() == 2);
        assert(g_capturedLines[0].find("WARN  ") != std::string::npos);
        assert(g_capturedLines[0].find("warning msg") != std::string::npos);
        assert(g_capturedLines[1].find("ERROR ") != std::string::npos);
        assert(g_capturedLines[1].find("error msg") != std::string::npos);

        std::cout << "[PASS] LOG_WARN/ERROR always output" << std::endl;
    }

    // ===== 5. setLogLevel() switches correctly =====
    {
        // Set to WARN - INFO filtered
        csl::Logger::setLogLevel(csl::LogLevel::WARN);
        setupCapture();
        LOG_INFO << "info should not appear";
        assert(g_capturedLines.empty());
        LOG_WARN << "warn should appear";
        assert(g_capturedLines.size() == 1);
        assert(g_capturedLines.back().find("warn should appear")
               != std::string::npos);
        restoreOutput();

        // Set to DEBUG - DEBUG appears
        csl::Logger::setLogLevel(csl::LogLevel::DEBUG);
        setupCapture();
        LOG_DEBUG << "debug should now appear";
        assert(g_capturedLines.size() == 1);
        assert(g_capturedLines.back().find("debug should now appear")
               != std::string::npos);
        restoreOutput();

        // Restore default
        csl::Logger::setLogLevel(csl::LogLevel::INFO);

        std::cout << "[PASS] setLogLevel() switches correctly" << std::endl;
    }

    // ===== 6. Thread ID field present =====
    {
        setupCapture();
        LOG_INFO << "tid check";
        restoreOutput();

        assert(!g_capturedLines.empty());
        const std::string& line = g_capturedLines.back();

        // TID is 5 digits after timestamp space
        auto pos = line.find(' ', 11);
        assert(pos != std::string::npos);
        char nextChar = line[pos + 1];
        assert(nextChar >= '0' && nextChar <= '9');

        std::cout << "[PASS] Thread ID field present" << std::endl;
    }

    std::cout << "\n=== All Logger tests passed ===" << std::endl;
    return 0;
}
