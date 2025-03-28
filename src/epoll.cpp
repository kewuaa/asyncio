#include <sys/epoll.h>

#include <spdlog/spdlog.h>

#include "asyncio/epoll.hpp"
#include "asyncio/utils.hpp"


ASYNCIO_NS_BEGIN()

Epoll::Epoll(): _fd(epoll_create1(0)) {
    SPDLOG_INFO("successfully create epoll fd {}", _fd);
}

Epoll::~Epoll() {
    if (_fd != -1) {
        close(_fd);
        SPDLOG_INFO("close epoll fd {}", _fd);
    }
}

Epoll& Epoll::get() noexcept {
    static Epoll epoll;
    return epoll;
}

void Epoll::add_reader(int fd, Handle::ID id, EventLoopCallback&& cb) noexcept {
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
    exit_if(event->reader.has_value(), "repeatly add reader for fd {}", fd);
    event->reader = { id, std::move(cb) };
    exit_if(
        epoll_ctl(_fd, op, fd, &event->event) == -1,
        "failed to add reader for fd {}",
        fd
    );
    SPDLOG_DEBUG("successfully add reader for fd {}", fd);
}

void Epoll::add_writer(int fd, Handle::ID id, EventLoopCallback&& cb) noexcept {
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
    exit_if(event->writer.has_value(), "repeatly add writer for fd {}", fd);
    event->writer = { id, std::move(cb) };
    exit_if(
        epoll_ctl(_fd, op, fd, &event->event) == -1,
        "failed to add writer for fd {}",
        fd
    );
    SPDLOG_DEBUG("successfully add writer for fd {}", fd);
}

void Epoll::remove_reader(int fd) noexcept {
    exit_if(_map.find(fd) == _map.end(), "fd {} not register yet", fd);
    if (!_map[fd].reader) {
        SPDLOG_WARN("reader of fd {} not registered", fd);
    }
    _map[fd].reader = std::nullopt;
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
    SPDLOG_DEBUG("successfully remove reader for fd {}", fd);
}

void Epoll::remove_writer(int fd) noexcept {
    exit_if(_map.find(fd) == _map.end(), "fd {} not register yet", fd);
    if (!_map[fd].writer) {
        SPDLOG_WARN("writer of fd {} not registered", fd);
    }
    _map[fd].writer = std::nullopt;
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
    SPDLOG_DEBUG("successfully remove writer for fd {}", fd);
}

ASYNCIO_NS_END
