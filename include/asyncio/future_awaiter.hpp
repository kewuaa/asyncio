#pragma once
#include <future>
#include <fcntl.h>

#include "tiny_thread_pool.hpp"

#include "concepts.hpp"
#include "promise.hpp"
#include "event_loop.hpp"
#include "epoll.hpp"


namespace kwa::asyncio {
    template<typename R>
    class FutureAwaiter {
        private:
            bool _done { false };
            int _fd[2] { -1, -1 };
            std::future<R> _fut { nullptr };
            std::mutex _lock {};
        public:
            FutureAwaiter() = delete;
            FutureAwaiter(FutureAwaiter&) = delete;
            FutureAwaiter(FutureAwaiter&&) = delete;
            FutureAwaiter& operator=(FutureAwaiter&) = delete;
            FutureAwaiter& operator=(FutureAwaiter&&) = delete;

            template<typename F, typename... Args>
            requires requires(F f, Args... args) {
                { f(args...) } -> std::same_as<R>;
            }
            FutureAwaiter(
                TinyThreadPool& pool,
                F&& f,
                Args&&... args
            ):
                _fut(
                    pool.submit(
                        [this, f = std::forward<F>(f), ...args = std::forward<Args>(args)]() {
                            auto res = f(args...);
                            {
                                std::lock_guard<std::mutex> lock { this->_lock };
                                this->_done = true;
                            }
                            if (this->_fd[1] != -1) {
                                char buf[1] = { 1 };
                                write(_fd[1], buf, 1);
                            }
                            return res;
                        }
                    )
                )
            {
                //
            }

            ~FutureAwaiter() {
                if (_fd[0] != -1) {
                    close(_fd[0]);
                    close(_fd[1]);
                }
            }

            inline bool await_ready() noexcept {
                return _done;
            }

            template<typename P>
            requires concepts::Promise<P> || std::same_as<P, void>
            bool await_suspend(std::coroutine_handle<P> handle) noexcept {
                std::lock_guard<std::mutex> lock { _lock };
                if (_done) {
                    return false;
                }
                pipe2(_fd, O_NONBLOCK);
                Epoll::get().add_reader(
                    _fd[0],
                    handle.promise().id(),
                    [h = &handle.promise()]() {
                        h->run();
                    }
                );
                return true;
            }

            inline decltype(auto) await_resume() noexcept {
#if _DEBUG
                if (_fd[0] != -1) {
                    char flag;
                    read(_fd[0], &flag, 1);
                    assert(flag == 1 && "flag == 1");
                }
#endif
                if constexpr (!std::is_same_v<R, void>) {
                    return _fut.get();
                }
            }
    };

    static_assert(concepts::Awaitable<FutureAwaiter<void>>, "Future<void> not satisfy the Awaitable concept");
    static_assert(concepts::Awaitable<FutureAwaiter<int>>, "Future<int> not satisfy the Awaitable concept");
}
