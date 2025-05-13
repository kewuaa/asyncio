#include <spdlog/fmt/fmt.h>

#include "asyncio/event_loop.hpp"
#include "asyncio/handle.hpp"
#include "asyncio/utils.hpp"


ASYNCIO_NS_BEGIN()

void CoroHandle::set_parent(CoroHandle& handle) noexcept {
    _parent = &handle;
}

void CoroHandle::try_resume_parent() const noexcept {
    if (!_parent) {
        return;
    }

    auto h = _parent;
    while (h) {
        if (h->canceled()) {
            auto ptr = _parent;
            while (ptr != h) {
                ptr->cancel();
                ptr = ptr->_parent;
            }
            break;
        }
        h = h->_parent;
    }

    if (!_parent->canceled()) {
        EventLoop::get().call_soon(*_parent);
    }
}

void CoroHandle::traceback(int depth) const noexcept {
    const auto& loc = get_loc();
    utils::print_location(loc, depth);
    if (_parent) {
        _parent->traceback(depth+1);
    }
}

ASYNCIO_NS_END
