#pragma once
#include <unordered_set>
#include <source_location>
#include <cstdint>

#include "types.hpp"
#include "asyncio_ns.hpp"
#include "asyncio_export.hpp"


ASYNCIO_NS_BEGIN()

using namespace types;

class EventLoop;
struct EventLoopHandle;
class ASYNCIO_EXPORT Handle {
    friend EventLoop;
public:
    using ID = uint32_t;
    static ID new_id() noexcept;
    static bool canceled(ID id) noexcept;
    Handle(Handle&) = delete;
    Handle& operator=(Handle&) = delete;
    Handle() noexcept;
    Handle(Handle&&) noexcept;
    ~Handle() noexcept;
    Handle& operator=(Handle&&) noexcept;
    virtual void cancel() noexcept;
    virtual void run() noexcept = 0;
    inline ID id() const noexcept { return _id; }
    inline bool canceled() const noexcept { return _canceled; }
private:
    static std::unordered_set<ID> _canceled_handles;
    ID _id { 0 };
    bool _canceled { true };
};


class ASYNCIO_EXPORT CoroHandle: public Handle {
public:
    void set_parent(CoroHandle&) noexcept;
    void try_resume_parent() const noexcept;
    void traceback(int depth) const noexcept;
    virtual const std::source_location& get_loc() const noexcept = 0;
private:
    CoroHandle* _parent { nullptr };
};


struct EventLoopHandle {
    Handle::ID id;
    EventLoopCallback cb;
};

ASYNCIO_NS_END
