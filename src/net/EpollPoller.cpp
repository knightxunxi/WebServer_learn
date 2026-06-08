// Copyright 2026, cpp-server-lab
// EpollPoller - Linux epoll 实现
//
// 平台依赖：
//   - epoll_create1 (Linux 2.6.27+)
//   - epoll_ctl / epoll_wait
//
// 触发模式：默认 LT（边缘触发 ET 需设置 EPOLLET，计划在里程后续实现）

#include "csl/net/EpollPoller.h"
#include "csl/net/Channel.h"
#include "csl/net/EventLoop.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <unistd.h>

namespace csl {

// ---- epoll_ctl 操作名（调试用） ----
static const char* operationToString(int op) {
    switch (op) {
        case EPOLL_CTL_ADD: return "ADD";
        case EPOLL_CTL_MOD: return "MOD";
        case EPOLL_CTL_DEL: return "DEL";
        default: return "UNKNOWN";
    }
}

// ===== 构造 / 析构 =====

EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)
{
    if (epollfd_ < 0) {
        // epoll_create1 失败是不可恢复的错误
        std::ostringstream oss;
        oss << "EpollPoller::EpollPoller - epoll_create1() 失败: "
            << strerror(errno);
        perror(oss.str().c_str());
        std::abort();
    }
}

EpollPoller::~EpollPoller() {
    ::close(epollfd_);
}

// ===== poll() =====

Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    int numEvents = ::epoll_wait(
        epollfd_,
        events_.data(),
        static_cast<int>(events_.size()),
        timeoutMs);

    int savedErrno = errno;
    Timestamp now = Timestamp::now();

    if (numEvents > 0) {
        // 有活跃事件
        fillActiveChannels(numEvents, activeChannels);

        // 动态扩容：如果本次事件数达到了数组容量，下次翻倍
        if (static_cast<size_t>(numEvents) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        // 超时，无事件 — 正常情况
    } else {
        // numEvents < 0：错误
        // EINTR 是信号中断，不属于错误
        if (savedErrno != EINTR) {
            std::ostringstream oss;
            oss << "EpollPoller::poll() - epoll_wait() 错误: "
                << strerror(savedErrno);
            perror(oss.str().c_str());
        }
    }

    return now;
}

// ===== updateChannel =====

void EpollPoller::updateChannel(Channel* channel) {
    Poller::assertInLoopThread();
    const int index = channel->index();

    if (index == -1 || index == 2) {
        // 新建 Channel 或之前被移除过
        int fd = channel->fd();
        if (index == -1) {
            // 新 Channel：加入映射
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        // 否则之前是 kDeleted（index==2），已从映射中移除但还未重建

        channel->set_index(1);  // kAdded
        update(EPOLL_CTL_ADD, channel);
    } else {
        // 已存在的 Channel：更新监听事件
        if (channel->isNoneEvent()) {
            // 不再监听任何事件 → 从 epoll 移除
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(2);  // kDeleted
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// ===== removeChannel =====

void EpollPoller::removeChannel(Channel* channel) {
    Poller::assertInLoopThread();

    int fd = channel->fd();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());

    int index = channel->index();
    assert(index == 1 || index == 2);  // kAdded 或 kDeleted

    size_t n = channels_.erase(fd);
    assert(n == 1); (void)n;

    if (index == 1) {
        // 仍注册在 epoll 中 → 需要 DEL
        update(EPOLL_CTL_DEL, channel);
    }
    // index == 2：已从 epoll 移除，只需清理映射

    channel->set_index(-1);  // kNew
}

// ===== 私有方法 =====

void EpollPoller::fillActiveChannels(
    int numEvents, ChannelList* activeChannels) const
{
    assert(static_cast<size_t>(numEvents) <= events_.size());

    for (int i = 0; i < numEvents; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EpollPoller::update(int operation, Channel* channel) {
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;

    int fd = channel->fd();
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        // EEXIST/DEL 时的 ENOENT 在某些场景下可忽略
        // 这里保持日志记录，实际生产环境可优化
        if (operation == EPOLL_CTL_DEL && errno == ENOENT) {
            return; // 已不存在，视为成功
        }
        std::ostringstream oss;
        oss << "EpollPoller::update() - epoll_ctl("
            << operationToString(operation) << ", fd=" << fd
            << ") 失败: " << strerror(errno);
        perror(oss.str().c_str());
    }
}

}  // namespace csl
