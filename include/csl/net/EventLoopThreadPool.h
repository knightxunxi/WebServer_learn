// Copyright 2026, cpp-server-lab
// EventLoopThreadPool - 多线程 IO 池
//
// 设计意图：
//   管理多个 EventLoopThread，为 TcpServer 提供 round-robin 分发策略。
//   当 numThreads == 0 时，所有 IO 在 baseLoop_ 中执行（单线程模式）。

#pragma once

#include "csl/base/noncopyable.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace csl {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& name);
    ~EventLoopThreadPool();

    /// @brief 设置线程数（需在 start 前调用）
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    /// @brief 启动所有 IO 线程
    /// @param cb 每个线程初始化时调用的回调
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    /// @brief 获取下一个 EventLoop（round-robin）
    ///        若 numThreads_ == 0，返回 baseLoop
    EventLoop* getNextLoop();

    /// @brief 获取所有 EventLoop
    std::vector<EventLoop*> getAllLoops();

    bool started() const { return started_; }
    const std::string& name() const { return name_; }

private:
    EventLoop* baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;  // round-robin 索引

    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};

}  // namespace csl
