#include <utility>

#include <spdlog/spdlog.h>

#include "asyncio/utils.hpp"
#include "asyncio/handle.hpp"


namespace kwa::asyncio {
    std::unordered_set<Handle::ID> Handle::_canceled_handles {};

    bool Handle::canceled(ID id) noexcept {
        return _canceled_handles.contains(id);
    }

    Handle::ID Handle::new_id() noexcept {
        static ID id = 0;
        return ++id;
    }

    Handle::Handle() noexcept: _id(new_id()), _canceled(false) {
        SPDLOG_DEBUG("handle {} created", _id);
    }

    Handle::Handle(Handle&& h) noexcept:
        _id(std::exchange(h._id, 0)),
        _canceled(std::exchange(h._canceled, true)) {}

    Handle::~Handle() noexcept {
        if (_canceled) {
            _canceled_handles.erase(_id);
        }
    }

    Handle& Handle::operator=(Handle&& h) noexcept {
        _id = std::exchange(h._id, 0);
        _canceled = std::exchange(h._canceled, true);
        return *this;
    }

    void Handle::cancel() noexcept {
        exit_if(!_id, "try cancel invalid handle");
        if (_canceled) {
            SPDLOG_WARN("handle {} already canceled, cancel repeatly", _id);
            return;
        }
        _canceled = true;
        _canceled_handles.insert(_id);
    }
}
