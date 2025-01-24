#pragma once
#include <chrono>
#include <functional>


namespace kwa::asyncio::types {
    using EventLoopHandle = std::function<void()>;
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
}
