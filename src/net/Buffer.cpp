// Copyright 2026, cpp-server-lab
// Buffer 实现（非模板部分）

#include "csl/net/Buffer.h"

#include <sys/uio.h>  // readv
#include <cerrno>

namespace csl {

/// @brief 从 fd 读取数据到 Buffer
///
/// 使用 scatter/gather IO (readv)，先写到 Buffer 的 writable 区域，
/// 如果不够再写到栈上的临时缓冲区（避免频繁 resize）。
/// 返回读取的字节数，错误时设置 *savedErrno。
ssize_t Buffer::readFd(int fd, int* savedErrno) {
    // 栈上额外缓冲，避免 1500 字节的帧导致多次 readv
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();

    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len  = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len  = sizeof(extrabuf);

    // 如果有足够的 writable 空间，只用一个 iovec
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0) {
        *savedErrno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        // 全部读到 Buffer 的 writable 区域
        writerIndex_ += n;
    } else {
        // writable 空间不够，部分读入了 extrabuf
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;
}

}  // namespace csl
