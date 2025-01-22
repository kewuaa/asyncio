#pragma once
#include <chrono>

#include "concepts.hpp"
#include "event_loop.hpp"
#include "timer.hpp"
#include "asyncio_export.hpp"


namespace kwa::asyncio {
    using namespace std::chrono;

    template<uint64_t MS>
    class ASYNCIO_EXPORT sleep {
        private:
            std::shared_ptr<Timer> _timer { nullptr };
        public:
            sleep() = default;
            sleep(sleep&) = delete;
            sleep(sleep&&) = delete;
            sleep& operator=(sleep&) = delete;
            sleep& operator=(sleep&&) = delete;

            constexpr bool await_ready() const noexcept { return false; }

            template<typename P>
            requires concepts::Promise<P> || std::same_as<P, void>
            void await_suspend(std::coroutine_handle<P> handle) noexcept {
                _timer = EventLoop::get().call_later(
                    milliseconds(MS),
                    [handle]() {
                        if (!handle.promise().canceled()) {
                            handle.resume();
                        }
                    }
                );
            }

            constexpr void await_resume() const noexcept {}

            inline void cancel() noexcept { _timer->cancel(); }

            inline bool canceled() const noexcept { return _timer->canceled(); }
    };

    template<>
    class sleep<0> {
        private:
            bool _canceled { false };
        public:
            sleep() = default;
            sleep(sleep&) = delete;
            sleep(sleep&&) = delete;
            sleep& operator=(sleep&) = delete;
            sleep& operator=(sleep&&) = delete;

            constexpr bool await_ready() const noexcept { return false; }

            template<typename P>
            requires concepts::Promise<P> || std::same_as<P, void>
            void await_suspend(std::coroutine_handle<P> handle) noexcept {
                EventLoop::get().call_soon(
                    [this, handle]() {
                        if (!this->_canceled) {
                            handle.resume();
                        }
                    }
                );
            }

            constexpr void await_resume() const noexcept {}

            inline void cancel() noexcept { _canceled = true; }

            inline bool canceled() const noexcept { return _canceled; }
    };

    static_assert(concepts::Awaitable<sleep<0>>, "sleep<0> not satisfy the Awaitable concept");
    static_assert(concepts::Cancelable<sleep<0>>, "sleep<0> not satisfy the Cancelable concept");
    static_assert(concepts::Awaitable<sleep<1000>>, "sleep<1000> not satisfy the Awaitable concept");
    static_assert(concepts::Cancelable<sleep<1000>>, "sleep<1000> not satisfy the Cancelable concept");
}
