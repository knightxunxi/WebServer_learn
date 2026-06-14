// Copyright 2026, cpp-server-lab
// Timestamp implementation
//
// Design notes:
//   - now() uses std::chrono::system_clock (C++20, cross-platform)
//   - toFormattedString() uses gmtime_r (POSIX) or gmtime_s (Windows)

#include "csl/base/timestamp.h"

#include <chrono>
#include <cstdio>

namespace csl {

// ===== Construction =====

Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

// ===== Validity =====

bool Timestamp::valid() const {
    return microSecondsSinceEpoch_ > 0;
}

// ===== Accessors =====

int64_t Timestamp::microSecondsSinceEpoch() const {
    return microSecondsSinceEpoch_;
}

time_t Timestamp::secondsSinceEpoch() const {
    return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
}

// ===== Formatting =====

std::string Timestamp::toString() const {
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;

    // 确保 microSeconds 始终非负
    // C++11 起 % 结果符号跟随被除数，负数会导致格式化异常
    if (microseconds < 0) {
        microseconds += kMicroSecondsPerSecond;
        seconds -= 1;
    }

    // Format: "seconds.microseconds" (6-digit zero-padded microseconds)
    snprintf(buf, sizeof(buf), "%lld.%06lld",
             static_cast<long long>(seconds),
             static_cast<long long>(microseconds));
    return buf;
}

std::string Timestamp::toFormattedString() const {
    // 负数时间戳在 gmtime_r/gmtime_s 中行为未定义
    if (microSecondsSinceEpoch_ < 0) {
        return "INVALID_TIMESTAMP";
    }

    char buf[32] = {0};
    time_t seconds = static_cast<time_t>(
        microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;

    struct tm tm_time;
#ifdef _WIN32
    // Windows: use gmtime_s (safe version)
    gmtime_s(&tm_time, &seconds);
#else
    // POSIX: use gmtime_r (thread-safe)
    gmtime_r(&seconds, &tm_time);
#endif

    // Format: YYYYMMDD HH:MM:SS.uuuuuu (UTC)
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06lld",
             tm_time.tm_year + 1900,
             tm_time.tm_mon + 1,
             tm_time.tm_mday,
             tm_time.tm_hour,
             tm_time.tm_min,
             tm_time.tm_sec,
             static_cast<long long>(microseconds));
    return buf;
}

// ===== Static factories =====

Timestamp Timestamp::now() {
    // Use std::chrono::system_clock for cross-platform microsecond precision
    auto now = std::chrono::system_clock::now();
    auto dur = now.time_since_epoch();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(dur);
    return Timestamp(static_cast<int64_t>(micros.count()));
}

Timestamp Timestamp::invalid() {
    return Timestamp();
}

// ===== Comparison operators =====

bool Timestamp::operator==(const Timestamp& rhs) const {
    return microSecondsSinceEpoch_ == rhs.microSecondsSinceEpoch_;
}

bool Timestamp::operator<(const Timestamp& rhs) const {
    return microSecondsSinceEpoch_ < rhs.microSecondsSinceEpoch_;
}

// ===== Free functions =====

double timeDifference(Timestamp high, Timestamp low) {
    int64_t diff = high.microSecondsSinceEpoch() -
                   low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

Timestamp addTime(Timestamp timestamp, double seconds) {
    int64_t delta = static_cast<int64_t>(
        seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}  // namespace csl
