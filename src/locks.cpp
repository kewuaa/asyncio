#include "asyncio/locks.hpp"


ASYNCIO_NS_BEGIN()

Mutex::Mutex(Mutex&& lock) noexcept:
    _owner(std::exchange(lock._owner, nullptr)) {}

Mutex& Mutex::operator=(Mutex&& lock) noexcept {
    _owner = std::exchange(lock._owner, nullptr);
    return *this;
}


Lock::Lock(Mutex& mtx) noexcept: _mtx(mtx) {}

Lock::~Lock() noexcept {
    assert(_mtx._owner == _handle);
    _mtx._owner = nullptr;
    if (!_mtx._wait_list.empty()) {
        auto handle = _mtx._wait_list.front();
        _mtx._wait_list.pop();
        EventLoop::get().call_soon(*handle);
    }
}

void Lock::await_resume() noexcept {
    assert(_mtx._owner == nullptr || _mtx._owner == _handle);
    _mtx._owner = _handle;
}


Condition::Waiter::Waiter(Condition& cond) noexcept: cond(cond) {}

Condition::Condition(Condition&& cond) noexcept:
    _wait_list(std::exchange(_wait_list, {})) {}

Condition& Condition::operator=(Condition&& cond) noexcept {
    _wait_list = std::exchange(_wait_list, {});
    return *this;
}

Condition::Waiter Condition::wait() noexcept {
    return { *this };
}

void Condition::notify_one() noexcept {
    if (!_wait_list.empty()) {
        auto handle = _wait_list.front();
        _wait_list.pop();
        EventLoop::get().call_soon(*handle);
    }
}

void Condition::notify_all() noexcept {
    while (!_wait_list.empty()) {
        notify_one();
    }
}

ASYNCIO_NS_END
