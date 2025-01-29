#pragma once
#include <unordered_map>
#include <sys/epoll.h>

#include "types.hpp"
#include "asyncio_export.hpp"


namespace kwa::asyncio {
    using namespace types;

    class ASYNCIO_EXPORT Epoll {
        public:
            struct Event;
        private:
            int _fd { -1 };
            std::unordered_map<int, Event> _map;

            Epoll();
        public:
            [[nodiscard]] static Epoll& get() noexcept;
            Epoll(Epoll&) = delete;
            Epoll(Epoll&&) = delete;
            Epoll& operator=(Epoll&) = delete;
            Epoll& operator=(Epoll&&) = delete;
            ~Epoll();
            void add_reader(int fd, EventLoopHandle&& handle) noexcept;
            void add_writer(int fd, EventLoopHandle&& handle) noexcept;
            void remove_reader(int fd) noexcept;
            void remove_writer(int fd) noexcept;

            [[nodiscard]] inline int wait(epoll_event* events, int event_num, int timeout) noexcept {
                return epoll_wait(_fd, events, event_num, timeout);
            }
    };

    struct Epoll::Event {
        EventLoopHandle reader { nullptr };
        EventLoopHandle writer { nullptr };
        epoll_event event {};
    };
}
