#pragma once
#include <functional>


namespace kwa::asyncio::types {
    using EventLoopHandle = std::function<void()>;
}
