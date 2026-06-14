// Copyright 2026, cpp-server-lab
// 控制台环境初始化工具。

#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace csl::platform {

inline void configureConsoleUtf8() {
#ifdef _WIN32
    // Windows CMD/PowerShell 默认代码页可能不是 UTF-8，直接输出中文会乱码。
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

}  // namespace csl::platform
