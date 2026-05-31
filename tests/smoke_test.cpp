#include <cassert>

int main() {
    static_assert(__cplusplus >= 202002L, "cpp-server-lab 需要 C++20");
    assert(1 + 1 == 2);
    return 0;
}
