#pragma once
#include <cstdio>
#include <coroutine>
#include <expected>

#include "concepts.hpp"
#include "epoll.hpp"
#include "task.hpp"
#include "asyncio_export.hpp"


namespace kwa::asyncio {
    using namespace types;

    class ASYNCIO_EXPORT Socket {
        private:
            class Reader;
            class Writer;
            class Accepter;

            int _fd { -1 };
            bool _own_fd { false };
            const char* _host { nullptr };
            short _port { -1 };
        public:
            Socket();
            Socket(int fd);
            Socket(Socket& s);
            Socket(Socket&& s);
            ~Socket();
            Socket& operator=(Socket& s) noexcept;
            Socket& operator=(Socket&& s) noexcept;
            [[nodiscard]] Result<> bind(const char* host, short port) noexcept;
            [[nodiscard]] Result<> listen(int max_listen_num) const noexcept;
            [[nodiscard]] Result<> connect(const char* host, short port) const noexcept;
            [[nodiscard]] Accepter accept() const noexcept;
            [[nodiscard]] Reader read(char* buffer, size_t size) const noexcept;
            [[nodiscard]] Task<> write(const char* buffer, size_t size) const noexcept;
    };

    class Socket::Reader {
        friend Socket;
        private:
            int _fd { -1 };
            char* _buffer { nullptr };
            size_t _buffer_size { 0 };
            size_t _read_size { 0 };
            bool _closed { false };
            Reader(int fd, char* buffer, size_t size);
            void _read_once() noexcept;
        public:
            Reader() = delete;
            Reader(Reader&) = delete;
            Reader(Reader&&) = delete;
            Reader& operator=(Reader&) = delete;
            Reader& operator=(Reader&&) = delete;

            inline bool await_ready() const noexcept {
                return _read_size > 0 or _closed;
            }

            template<typename P>
            requires concepts::Promise<P> || std::same_as<P, void>
            void await_suspend(std::coroutine_handle<P> handle) const noexcept {
                auto& epoll = Epoll::get();
                epoll.add_reader(
                    _fd,
                    handle.promise().id(),
                    [h = &handle.promise()](){
                        h->run();
                    }
                );
            }

            Result<int> await_resume() noexcept;

            // static_assert(concepts::Awaitable<Reader>, "Reader not satisfy the Awaitable concept");
    };

    class Socket::Writer {
        friend Socket;
        private:
            int _fd { -1 };
            const char* _buffer { nullptr };
            size_t _buffer_size { 0 };
            size_t _write_size { 0 };
            bool _closed { false };
            Writer(int fd, const char* buffer, size_t size);
            void _write_once() noexcept;
        public:
            Writer() = delete;
            Writer(Writer&) = delete;
            Writer(Writer&&) = delete;
            Writer& operator=(Writer&) = delete;
            Writer& operator=(Writer&&) = delete;

            inline bool await_ready() const noexcept {
                return _write_size == _buffer_size or _closed;
            }

            template<typename P>
            requires concepts::Promise<P> || std::same_as<P, void>
            void await_suspend(std::coroutine_handle<P> handle) const noexcept {
                auto& epoll = Epoll::get();
                epoll.add_writer(
                    _fd,
                    handle.promise().id(),
                    [h = &handle.promise()](){
                        h->run();
                    }
                );
            }

            Result<int> await_resume() noexcept;

            // static_assert(concepts::Awaitable<Writer>, "Writer not satisfy the Awaitable concept");
    };

    class Socket::Accepter {
        friend Socket;
        private:
            int _conn { -1 };
            int _fd { -1 };
            const char* _host { nullptr };
            short _port { -1 };
            Accepter(int fd, const char* host, short port);
            void _accept_once() noexcept;
        public:
            Accepter() = delete;
            Accepter(Accepter&) = delete;
            Accepter(Accepter&&) = delete;
            Accepter& operator=(Accepter&) = delete;
            Accepter& operator=(Accepter&&) = delete;

            inline bool await_ready() const noexcept {
                return _conn != -1;
            }

            template<typename P>
            requires concepts::Promise<P> || std::same_as<P, void>
            void await_suspend(std::coroutine_handle<P> handle) const noexcept {
                auto& epoll = Epoll::get();
                epoll.add_reader(
                    _fd,
                    handle.promise().id(),
                    [h = &handle.promise()]() {
                        h->run();
                    }
                );
            }

            [[nodiscard]] int await_resume() noexcept;

            // static_assert(concepts::Awaitable<Accepter>, "Accepter not satisfy the Awaitable concept");
    };
}
