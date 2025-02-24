#include "asyncio/event_loop.hpp"
#include "asyncio/handle.hpp"


namespace kwa::asyncio {
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
}
