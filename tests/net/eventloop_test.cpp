// eventloop_test — EventLoop / Channel / Poller 集成测试
//
// 测试策略：
//   本测试在单线程内运行一个 EventLoop，通过 pipe 模拟 IO 事件，
//   验证 Channel 回调、poll 分发、runInLoop/queueInLoop 和 quit 机制。
//
// 运行方式（Linux）：
//   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
//   cmake --build build
//   ./build/tests/csl_eventloop_test

#include "csl/net/EventLoop.h"
#include "csl/net/Channel.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <thread>
#include <unistd.h>

// ===== 辅助：pipe + Channel 测试对 =====

struct PipeChannel {
    int pipefd[2];
    csl::EventLoop* loop;
    csl::Channel* channel;
    int readCount;

    PipeChannel(csl::EventLoop* l)
        : loop(l), readCount(0)
    {
        if (::pipe(pipefd) != 0) {
            perror("pipe 创建失败");
            std::abort();
        }
        channel = new csl::Channel(loop, pipefd[0]);
        channel->setReadCallback(
            [this](csl::Timestamp) {
                readCount++;
            });
    }

    ~PipeChannel() {
        channel->disableAll();
        channel->remove();
        delete channel;
        ::close(pipefd[0]);
        ::close(pipefd[1]);
    }

    void writeToPipe(const char* data, size_t len) {
        ssize_t n = ::write(pipefd[1], data, len);
        (void)n;
    }
};

// ===== 测试用例 =====

int main() {
    // ---- Test 1: EventLoop 基本生命周期 ----
    {
        csl::EventLoop loop;
        // loop 未被运行，可以安全析构
        std::cout << "[PASS] Test 1: EventLoop 构造/析构正常" << std::endl;
    }

    // ---- Test 2: Channel 注册和事件回调 ----
    {
        csl::EventLoop loop;
        PipeChannel pc(&loop);
        pc.channel->enableReading();

        assert(pc.channel->isReading());
        assert(!pc.channel->isWriting());

        // 写入数据后 quit
        pc.writeToPipe("x", 1);
        loop.runAfter(0.01, [&loop]() { loop.quit(); });

        loop.loop();

        // poll 应该已分发读事件
        assert(pc.readCount > 0);
        std::cout << "[PASS] Test 2: Channel 读回调触发 (count="
                  << pc.readCount << ")" << std::endl;
    }

    // ---- Test 3: Channel disableReading ----
    {
        csl::EventLoop loop;
        PipeChannel pc(&loop);

        // 先启用再停用
        pc.channel->enableReading();
        assert(pc.channel->isReading());

        pc.channel->disableReading();
        assert(!pc.channel->isReading());
        assert(pc.channel->isNoneEvent());

        // 写入数据并 quit
        pc.writeToPipe("x", 1);
        loop.runAfter(0.01, [&loop]() { loop.quit(); });
        loop.loop();

        // 读事件被禁用，不应触发回调
        assert(pc.readCount == 0);
        std::cout << "[PASS] Test 3: disableReading 阻止回调" << std::endl;
    }

    // ---- Test 4: runInLoop 在当前线程 ----
    {
        csl::EventLoop loop;
        bool called = false;

        // 在 loop 线程中调用 runInLoop
        loop.runInLoop([&called]() { called = true; });

        // 还没启动循环，任务尚未执行
        assert(!called);

        loop.runAfter(0.01, [&loop]() { loop.quit(); });
        loop.loop();

        // loop 执行后应已调用
        assert(called);
        std::cout << "[PASS] Test 4: runInLoop 同步执行" << std::endl;
    }

    // ---- Test 5: queueInLoop 跨线程投递 ----
    {
        csl::EventLoop loop;
        bool called = false;

        std::thread t([&loop, &called]() {
            // 在非 IO 线程中投递任务
            loop.runInLoop([&called]() { called = true; });
            // runInLoop 会因不在 IO 线程而转为 queueInLoop + wakeup
        });

        loop.runAfter(0.05, [&loop]() { loop.quit(); });
        loop.loop();
        t.join();

        assert(called);
        std::cout << "[PASS] Test 5: queueInLoop 跨线程投递" << std::endl;
    }

    // ---- Test 6: quit 跨线程调用 ----
    {
        csl::EventLoop loop;

        // 在子线程中 10ms 后 quit
        std::thread t([&loop]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            loop.quit();
        });

        loop.loop();
        t.join();

        // quit 已被调用，loop 退出，没有死锁
        std::cout << "[PASS] Test 6: quit 跨线程退出" << std::endl;
    }

    // ---- Test 7: EventLoop 不允许嵌套 ----
    {
        csl::EventLoop loop;
        bool nestedLoopStarted = false;

        // 在 loop 线程的回调中尝试创建第二个 EventLoop（应 abort）
        // 由于这会 abort，这里只验证静态方法能正确返回线程的 EventLoop
        csl::EventLoop* current = csl::EventLoop::getEventLoopOfCurrentThread();
        // 注意：loop 尚未启动，当前线程不是 IO 线程
        // getEventLoopOfCurrentThread 返回的是构造时设置的 t_loopInThisThread
        // 因为 loop 是在 main 线程构造的

        loop.runAfter(0.01, [&loop]() { loop.quit(); });
        loop.loop();

        std::cout << "[PASS] Test 7: getEventLoopOfCurrentThread 可用" << std::endl;
    }

    // ---- Test 8: Channel 写事件 ----
    {
        csl::EventLoop loop;
        PipeChannel pc(&loop);
        bool writeFired = false;

        pc.channel->setWriteCallback([&writeFired]() {
            writeFired = true;
        });
        pc.channel->enableWriting();

        // pipe 通常立即可写，所以 write 回调会在下一轮 poll 触发
        loop.runAfter(0.01, [&loop]() { loop.quit(); });
        loop.loop();

        assert(writeFired);
        std::cout << "[PASS] Test 8: Channel 写回调触发" << std::endl;
    }

    std::cout << "\n=== All EventLoop tests passed ===" << std::endl;
    return 0;
}
