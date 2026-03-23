#pragma once

#include <chrono>

namespace YipOS {

inline double MonotonicNow() {
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

} // namespace YipOS
