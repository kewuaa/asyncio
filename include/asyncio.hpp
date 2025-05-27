#pragma once
#include "asyncio_ns.hpp"
#include "asyncio/event_loop.hpp"
#include "asyncio/task.hpp"
#include "asyncio/socket.hpp"
#include "asyncio/timer.hpp"
#include "asyncio/sleep.hpp"
#include "asyncio/future_awaiter.hpp"
#include "asyncio/locks.hpp"
#include "asyncio/traceback.hpp"
#include "asyncio/types.hpp"


ASYNCIO_NS_BEGIN()

using namespace types;

template<typename T>
requires concepts::Task<std::decay_t<T>>
inline decltype(auto) run(T&& task) {
    auto& loop = EventLoop::get();
    return loop.run_until_complete(std::forward<T>(task));
}


template<typename T>
requires concepts::Task<std::decay_t<T>>
asyncio::Task<bool> wait_for(T&& task, std::chrono::milliseconds timeout) {
    if (task.done()) {
        co_return false;
    }
    Event<bool> ev;
    auto& loop = EventLoop::get();
    auto timer = loop.call_later(timeout, [&ev, &task] {
        task.cancel();
        if (!ev.is_set()) {
            ev.set(true);
        }
    });
    if constexpr (std::is_void_v<typename std::decay_t<T>::result_type>) {
        task.add_done_callback([timer, &ev] {
            timer->cancel();
            if (!ev.is_set()) {
                ev.set(false);
            }
        });
    } else {
        task.add_done_callback([timer, &ev](const auto& _) {
            timer->cancel();
            if (!ev.is_set()) {
                ev.set(false);
            }
        });
    }
    co_return *(co_await ev.wait());
}

ASYNCIO_NS_END
