#pragma once
#include <expected>
#include <chrono>
#include <functional>

#include "asyncio_ns.hpp"


ASYNCIO_NS_BEGIN(types)

using EventLoopCallback = std::function<void()>;
using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

template<typename R, typename E>
struct task_result_struct {
    using type = std::expected<R, E>;
};
template<typename R>
struct task_result_struct<R, void> {
    using type = R;
};
template<typename R = void, typename E = void>
using TaskResult = task_result_struct<R, E>::type;

template<typename R, typename E>
struct task_callback_struct {
    using type = std::function<void(const TaskResult<R, E>&)>;
};
template<>
struct task_callback_struct<void, void> {
    using type = std::function<void()>;
};
template<typename R, typename E>
using TaskCallback = task_callback_struct<R, E>::type;

ASYNCIO_NS_END
