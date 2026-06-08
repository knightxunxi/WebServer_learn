// Copyright 2026, cpp-server-lab
// Timestamp - 微秒精度时间戳
//
// 设计意图：
//   统一的、轻量级的时间表示，内部以 int64_t 存储自 Epoch 以来的微秒数。
//   sizeof(Timestamp) == 8，可以作为值类型高效传递。
//   提供格式化和比较能力，供 Logger、TimerQueue 等模块使用。
//
// 参考：muduo/base/Timestamp.h
// 差异：不依赖 Boost；暂不支持时区设置，统一使用 UTC。

#pragma once

#include <cstdint>
#include <ctime>
#include <string>

namespace csl {

/// @brief 微秒精度时间戳
///
/// 内部存储自 Unix Epoch (1970-01-01 00:00:00 UTC) 以来的微秒数。
/// 使用 int64_t 确保 8 字节对齐，支持高效的值传递。
class Timestamp {
public:
    /// @brief 构造无效时间戳（微秒数为 0）
    Timestamp();

    /// @brief 用微秒数构造时间戳
    /// @param microSecondsSinceEpoch 自 Epoch 以来的微秒数
    explicit Timestamp(int64_t microSecondsSinceEpoch);

    /// @brief 是否为有效时间戳（微秒数 > 0）
    bool valid() const;

    /// @brief 获取微秒数
    int64_t microSecondsSinceEpoch() const;

    /// @brief 获取秒数（向下取整）
    time_t secondsSinceEpoch() const;

    /// @brief 格式化为 "秒.微秒" 字符串
    /// @return 如 "1234567890.123456"
    std::string toString() const;

    /// @brief 格式化为 "YYYYMMDD HH:MM:SS.uuuuuu" 字符串（UTC）
    /// @return 如 "20260602 18:30:45.123456"
    std::string toFormattedString() const;

    /// @brief 获取当前时间
    static Timestamp now();

    /// @brief 返回无效时间戳
    static Timestamp invalid();

    bool operator==(const Timestamp& rhs) const;
    bool operator<(const Timestamp& rhs) const;

    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t microSecondsSinceEpoch_;
};

// 确保 sizeof(Timestamp) == 8，保证值语义效率
static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp 大小必须等于 int64_t");

// ===== 自由函数 =====

/// @brief 计算两个时间戳之间的秒差（double 精度）
/// @param high 较晚的时间戳
/// @param low  较早的时间戳
/// @return 秒差，可能为负数
double timeDifference(Timestamp high, Timestamp low);

/// @brief 在时间戳上加秒数偏移
/// @param timestamp 基准时间戳
/// @param seconds   偏移秒数（可为负）
/// @return 新的 Timestamp
Timestamp addTime(Timestamp timestamp, double seconds);

// ===== 组合比较运算符（内联，通过 == 和 < 实现） =====

inline bool operator!=(const Timestamp& lhs, const Timestamp& rhs) {
    return !(lhs == rhs);
}

inline bool operator>(const Timestamp& lhs, const Timestamp& rhs) {
    return rhs < lhs;
}

inline bool operator<=(const Timestamp& lhs, const Timestamp& rhs) {
    return !(rhs < lhs);
}

inline bool operator>=(const Timestamp& lhs, const Timestamp& rhs) {
    return !(lhs < rhs);
}

}  // namespace csl
