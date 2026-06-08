// Copyright 2026, cpp-server-lab
// noncopyable - 禁止拷贝语义的基类
//
// 设计意图：
//   通过继承 noncopyable，子类自动删除拷贝构造和拷贝赋值运算符，
//   从而在编译期阻止对象被拷贝。这适用于大多数持有资源的 RAII 对象
//   （如 EventLoop、Poller、TcpConnection 等）。
//
// 与 muduo 的差异：
//   muduo 同时提供了 copyable 和 noncopyable，本项目假设默认禁止拷贝，
//   需要拷贝的类型自行实现拷贝语义，因此不提供 copyable。

#pragma once

namespace csl {

/// @brief 禁止拷贝的基类
///
/// 构造和析构函数为 protected，意味着：
///   - 不能独立实例化 noncopyable 对象
///   - 只能通过继承使用
///
/// 使用 C++11 = delete 语法删除拷贝操作，是现代 C++ 推荐做法。
/// 得益于空基类优化（EBO），不影响子类对象大小。
class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

}  // namespace csl
