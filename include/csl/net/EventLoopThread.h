// Copyright 2026, cpp-server-lab
// EventLoopThread - 启动一个线程并在其中运行 EventLoop
//
// 设计意图：
//   one loop per thread 的实现，每个 EventLoopThread 创建一个线程，
//   在线程中初始化 EventLoop 并等待外部获取其指针。

#pragma once

#include "csl/base/noncopyable.h"

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace csl {

class EventLoop;

class EventLoopThread : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    explicit EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                             const std::string& name = "");
    ~EventLoopThread();

    /// @brief 启动线程，返回线程中的 EventLoop 指针（阻塞直到线程初始化完成）
    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    std::unique_ptr<std::thread> thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};

}  // namespace csl
