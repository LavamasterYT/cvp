#pragma once

#include <chrono>

namespace timer {
    inline auto now() {
        return std::chrono::high_resolution_clock::now();
    }

    inline long long ms(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
}