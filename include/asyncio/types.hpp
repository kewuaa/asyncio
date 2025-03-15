#pragma once
#include <expected>
#include <chrono>
#include <functional>

#include "asyncio_ns.hpp"
#include "exception.hpp"


ASYNCIO_NS_BEGIN(types)

using EventLoopCallback = std::function<void()>;
using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
template<typename R = void>
using Result = std::expected<R, Exception>;
using Error = std::unexpected<Exception>;

ASYNCIO_NS_END
