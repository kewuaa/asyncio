#pragma once
#include <cassert>
#include <future>
#include <fcntl.h>
#include <sys/eventfd.h>

#include "asyncio_ns.hpp"
#include "concepts.hpp"
#include "event_loop.hpp"


ASYNCIO_NS_BEGIN()

template<typename R>
class FutureAwaiter {
public:
    FutureAwaiter() = delete;
    FutureAwaiter(FutureAwaiter&) = delete;
    FutureAwaiter(FutureAwaiter&&) = delete;
    FutureAwaiter& operator=(FutureAwaiter&) = delete;
    FutureAwaiter& operator=(FutureAwaiter&&) = delete;

    template<typename Pool, typename F, typename... Args>
    requires requires(Pool pool, std::function<R(void)>&& f) {
        { pool.submit(std::move(f)) } -> std::same_as<std::future<R>>;
    } && requires (F&& f, Args&&... args) {
        { std::forward<F>(f)(std::forward<Args>(args)...) } -> std::same_as<R>;
    }
    FutureAwaiter(int eventfd, Pool& pool, F&& f, Args&&... args):
        _fut(
            pool.submit(
                [this, eventfd, f = std::forward<F>(f), ...args = std::forward<Args>(args)] mutable -> R {
                    if constexpr (std::is_void_v<R>) {
                        std::forward<F>(f)(std::forward<Args>(args)...);
                        _done_callback(eventfd);
                    } else {
                        R res = std::forward<F>(f)(std::forward<Args>(args)...);
                        _done_callback(eventfd);
                        return res;
                    }
                }
            )
        )
    {
        //
    }

    inline bool await_ready() noexcept {
        return _status.load() != 0;
    }

    template<typename P>
    requires concepts::Promise<P> || std::same_as<P, void>
    bool await_suspend(std::coroutine_handle<P> handle) noexcept {
        uint64_t status;
        do {
            status = _status.load();
            if (status != 0) {
                return false;
            }
        } while (!_status.compare_exchange_weak(status, (uint64_t)&handle.promise()));
        return true;
    }

    inline decltype(auto) await_resume() noexcept {
        if constexpr (!std::is_same_v<R, void>) {
            return _fut.get();
        }
    }
private:
    std::future<R> _fut { nullptr };
    std::atomic<uint64_t> _status { 0 };

    void _done_callback(int eventfd) noexcept {
        uint64_t status;
        do {
            status = _status.load();
            if (status != 0) {
                EventLoop::get().call_soon_threadsafe(*(Handle*)status);
                eventfd_write(eventfd, 1);
                return;
            }
        } while (!_status.compare_exchange_weak(status, 1));
    }
};
static_assert(concepts::Awaitable<FutureAwaiter<void>>, "Future<void> not satisfy the Awaitable concept");
static_assert(concepts::Awaitable<FutureAwaiter<int>>, "Future<int> not satisfy the Awaitable concept");

ASYNCIO_NS_END
