#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>

#include <spdlog/spdlog.h>

#include "asyncio/socket.hpp"
#include "asyncio/epoll.hpp"
#include "asyncio/utils.hpp"


static void init_address(sockaddr_in& addr, const char* host, short port) noexcept {
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(port);
}


static void nonblock_socket(int fd) noexcept {
    auto flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}


ASYNCIO_NS_BEGIN()

////////////////////////////////////////////////////
///ASocket
////////////////////////////////////////////////////
Socket::Socket(): _fd(socket(AF_INET, SOCK_STREAM, 0)), _own_fd(true) {
    SPDLOG_INFO("successfully open socket fd {}", _fd);
    nonblock_socket(_fd);
}

Socket::Socket(int fd): _fd(fd), _own_fd(false) {
    nonblock_socket(fd);
}

Socket::Socket(Socket& s):
    _fd(s._fd),
    _own_fd(false),
    _host(s._host),
    _port(s._port)
{
    //
}

Socket::Socket(Socket&& s):
    _fd(std::exchange(s._fd, -1)),
    _own_fd(std::exchange(s._own_fd, false)),
    _host(std::exchange(s._host, nullptr)),
    _port(std::exchange(s._port, -1))
{
    //
}

Socket::~Socket() {
    if (_own_fd && _fd != -1) {
        close(_fd);
        SPDLOG_INFO("close socket fd {}", _fd);
    }
}

Socket& Socket::operator=(Socket& s) noexcept {
    _fd = s._fd;
    _own_fd = false;
    _host = s._host;
    _port = s._port;
    return *this;
}

Socket& Socket::operator=(Socket&& s) noexcept {
    _fd = std::exchange(s._fd, -1);
    _own_fd = std::exchange(s._own_fd, false);
    _host = std::exchange(s._host, nullptr);
    _port = std::exchange(s._port, -1);
    return *this;
}

int Socket::bind(const char* host, short port) noexcept {
    _host = host;
    _port = port;
    sockaddr_in addr { 0 };
    init_address(addr, host, port);
    return ::bind(_fd, (sockaddr*)&addr, sizeof(addr));
}

int Socket::listen(int max_listen_num) const noexcept {
    return ::listen(_fd, max_listen_num);
}

int Socket::connect(const char* host, short port) const noexcept {
    sockaddr_in addr { 0 };
    init_address(addr, host, port);
    return ::connect(_fd, (sockaddr*)&addr, sizeof(addr));
}

Socket::Accepter Socket::accept() const noexcept {
    exit_if(!_host, "socket fd {} not bind to any address yet", _fd);
    return { _fd, _host, _port };
}

Socket::Reader Socket::read(char* buffer, size_t size) const noexcept {
    return { _fd, buffer, size };
}

Task<void, const char*> Socket::write(const char* buffer, size_t size) const noexcept {
    int buffer_size = size;
    int pos = 0;
    while (true) {
        auto res = co_await Writer(
            _fd,
            buffer + pos,
            buffer_size
        );
        if (!res) {
            co_return res.error();
        }
        auto nbytes = *res;
        if (nbytes == buffer_size) {
            break;
        }
        buffer_size -= nbytes;
        pos += nbytes;
    }
}


////////////////////////////////////////////////////
///ASocket::Reader
////////////////////////////////////////////////////
Socket::Reader::Reader(int fd, char* buffer, size_t size):
    _fd(fd), _buffer(buffer), _buffer_size(size)
{
    _read_once();
}

void Socket::Reader::_read_once() noexcept {
    while (_read_size < _buffer_size) {
        auto nbytes = ::read(_fd, _buffer+_read_size, _buffer_size-_read_size);
        if (nbytes > 0) {
            _read_size += nbytes;
        } else if (nbytes == -1) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
        } else if (nbytes == 0) {
            SPDLOG_INFO("connection to socket fd {} closed", _fd);
            _closed = true;
            break;
        }
    }
}

TaskResult<int, const char*> Socket::Reader::await_resume() noexcept {
    if (_read_size > 0) {
        return _read_size;
    }
    if (_closed) {
        return std::unexpected("connection closed");
    }
    auto& epoll = Epoll::get();
    epoll.remove_reader(_fd);
    _read_once();
    if (_closed) {
        return std::unexpected("connection closed");
    }
    return _read_size;
}

////////////////////////////////////////////////////
///ASocket::Writer
////////////////////////////////////////////////////
Socket::Writer::Writer(int fd, const char* buffer, size_t size):
    _fd(fd), _buffer(buffer), _buffer_size(size)
{
    _write_once();
}

void Socket::Writer::_write_once() noexcept {
    while (_write_size < _buffer_size) {
        auto nbytes = ::write(_fd, _buffer+_write_size, _buffer_size-_write_size);
        if (nbytes > 0) {
            _write_size += nbytes;
        } else if (nbytes == -1) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
        } else if (nbytes == 0) {
            SPDLOG_INFO("connection to socket fd {} closed", _fd);
            _closed = true;
            break;
        }
    }
}

TaskResult<int, const char*> Socket::Writer::await_resume() noexcept {
    if (_write_size == _buffer_size) {
        return _write_size;
    }
    if (_closed) {
        return std::unexpected("connection closed");
    }
    auto& epoll = Epoll::get();
    epoll.remove_writer(_fd);
    _write_once();
    if (_closed) {
        return std::unexpected("connection closed");
    }
    return _write_size;
}


////////////////////////////////////////////////////
///ASocket::Accepter
////////////////////////////////////////////////////
Socket::Accepter::Accepter(int fd, const char* host, short port):
    _fd(fd), _host(host), _port(port)
{
    _accept_once();
}

void Socket::Accepter::_accept_once() noexcept {
    sockaddr_in addr { 0 };
    init_address(addr, _host, _port);
    socklen_t addr_len = sizeof(addr);
    _conn = ::accept(_fd, (sockaddr*)&addr, &addr_len);
}

int Socket::Accepter::await_resume() noexcept {
    if (_conn != -1) {
        return _conn;
    }
    auto& epoll = Epoll::get();
    epoll.remove_reader(_fd);
    _accept_once();
    return _conn;
}

ASYNCIO_NS_END
