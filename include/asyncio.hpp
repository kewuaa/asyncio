#pragma once
#include "asyncio/event_loop.hpp"
#include "asyncio/task.hpp"
#include "asyncio/socket.hpp"
#include "asyncio/timer.hpp"
#include "asyncio/sleep.hpp"
#include "asyncio/future_awaiter.hpp"
#include "asyncio/exception.hpp"


namespace kwa::asyncio {
    template<typename T>
    requires concepts::Task<T>
    auto run(T&& task) -> decltype(task.result()) {
        auto& loop = EventLoop::get();
        return loop.run_until_complete(std::forward<T>(task));
    }
}
