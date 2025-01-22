#include <cstring>
#include <utility>

#include "asyncio/exception.hpp"


namespace kwa::asyncio {
    Exception::Exception(int error_code): _msg(strerror(error_code)) {
        //
    }

    Exception::Exception(const char* msg): _msg(msg) {
        //
    }

    Exception::Exception(Exception& e): _msg(e._msg) {
        //
    }

    Exception::Exception(Exception&& e):
        _msg(std::exchange(e._msg, {}))
    {
        //
    }

    Exception& Exception::operator=(Exception& e) noexcept {
        _msg = e._msg;
        return *this;
    }

    Exception& Exception::operator=(Exception&& e) noexcept {
        _msg = std::exchange(e._msg, {});
        return *this;
    }

    std::ostream& operator<<(std::ostream& s, const Exception& e) noexcept {
        s << e._msg;
        return s;
    }
}
