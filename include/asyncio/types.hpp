#pragma once
#include <expected>
#include <chrono>
#include <functional>

#include "exception.hpp"


namespace kwa::asyncio::types {
    using EventLoopCallback = std::function<void()>;
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    template<typename R = void>
    using Result = std::expected<R, Exception>;
    using Error = std::unexpected<Exception>;
}
