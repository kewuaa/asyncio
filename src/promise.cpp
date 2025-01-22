#include "asyncio/promise.hpp"
#include "asyncio/event_loop.hpp"


namespace kwa::asyncio {
    bool BasePromise::canceled() const noexcept {
        // if canceled, return directly
        if (_canceled) {
            return true;
        }

        // else find if any parent promise is canceled
        bool parent_canceled = false;
        auto p = this->_parent;
        while (p) {
            if (p->_canceled) {
                parent_canceled = true;
                break;
            }
            p = p->_parent;
        }
        // if any parent promise is canceled, update state of child promise
        if (parent_canceled) {
            _canceled = true;
            p = this->_parent;
            while (!p->_canceled) {
                p->_canceled = true;
                p = p->_parent;
            }
        }
        return parent_canceled;
    }

    void BasePromise::try_resume_parent() const noexcept {
        if (_parent && !_parent->_canceled) {
            EventLoop::get().call_soon(
                [p = _parent]() {
                    if (!p->_canceled) {
                        p->resume();
                    }
                }
            );
        }
    }
}
