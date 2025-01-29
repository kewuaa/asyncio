#include <sys/epoll.h>

#include <spdlog/spdlog.h>

#include "asyncio/types.hpp"
#include "asyncio/epoll.hpp"
#include "asyncio/utils.hpp"


namespace kwa::asyncio {
    Epoll::Epoll(): _fd(epoll_create1(0)) {
        spdlog::info("successfully create epoll fd {}", _fd);
    }

    Epoll::~Epoll() {
        if (_fd != -1) {
            close(_fd);
            spdlog::info("close epoll fd {}", _fd);
        }
    }

    Epoll& Epoll::get() noexcept {
        static Epoll epoll;
        return epoll;
    }

    void Epoll::add_reader(int fd, EventLoopHandle&& handle) noexcept {
        Event* event;
        int op;
        if (_map.find(fd) == _map.end()) {
            event = &(_map[fd] = {});
            event->event.data.ptr = event;
            event->event.events = EPOLLIN | EPOLLET;
            op = EPOLL_CTL_ADD;
        } else {
            event = &_map[fd];
            event->event.events |= EPOLLIN;
            op = EPOLL_CTL_MOD;
        }
        exit_if(event->reader != nullptr, "repeatly add reader for fd {}", fd);
        event->reader = std::move(handle);
        exit_if(
            epoll_ctl(_fd, op, fd, &event->event) == -1,
            "failed to add reader for fd {}",
            fd
        );
        spdlog::info("successfully add reader for fd {}", fd);
    }

    void Epoll::add_writer(int fd, EventLoopHandle&& handle) noexcept {
        Event* event;
        int op;
        if (_map.find(fd) == _map.end()) {
            event = &(_map[fd] = {});
            event->event.data.ptr = event;
            op = EPOLL_CTL_ADD;
            event->event.events = EPOLLOUT | EPOLLET;
        } else {
            event = &_map[fd];
            event->event.events |= EPOLLOUT;
            op = EPOLL_CTL_MOD;
        }
        exit_if(event->writer != nullptr, "repeatly add writer for fd {}", fd);
        event->writer = std::move(handle);
        exit_if(
            epoll_ctl(_fd, op, fd, &event->event) == -1,
            "failed to add writer for fd {}",
            fd
        );
        spdlog::info("successfully add writer for fd {}", fd);
    }

    void Epoll::remove_reader(int fd) noexcept {
        exit_if(_map.find(fd) == _map.end(), "fd {} not register yet", fd);
        if (!_map[fd].reader) {
            spdlog::warn("reader of fd {} not registered", fd);
        }
        _map[fd].reader = nullptr;
        _map[fd].event.events &= ~EPOLLIN;
        if (_map[fd].writer) {
            exit_if(
                epoll_ctl(_fd, EPOLL_CTL_MOD, fd, &_map[fd].event) == -1,
                "failed to ctl fd {}",
                fd
            );
        } else {
            _map.erase(fd);
            exit_if(
                epoll_ctl(_fd, EPOLL_CTL_DEL, fd, nullptr) == -1,
                "failed to remove reader for fd {}",
                fd
            );
        }
        spdlog::info("successfully remove reader for fd {}", fd);
    }

    void Epoll::remove_writer(int fd) noexcept {
        exit_if(_map.find(fd) == _map.end(), "fd {} not register yet", fd);
        if (!_map[fd].writer) {
            spdlog::warn("writer of fd {} not registered", fd);
        }
        _map[fd].writer = nullptr;
        _map[fd].event.events &= ~EPOLLOUT;
        if (_map[fd].reader) {
            exit_if(
                epoll_ctl(_fd, EPOLL_CTL_MOD, fd, &_map[fd].event) == -1,
                "failed to ctl fd {}",
                fd
            );
        } else {
            _map.erase(fd);
            exit_if(
                epoll_ctl(_fd, EPOLL_CTL_DEL, fd, nullptr) == -1,
                "failed to remove writer for fd {}",
                fd
            );
        }
        spdlog::info("successfully remove writer for fd {}", fd);
    }
}
