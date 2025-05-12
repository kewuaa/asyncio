# Introduction

Drawing on the c++ asynchronous framework implemented by python asyncio library

# usage

simple example:

*python*:
```python
import asyncio


async def main() -> None:
    print('Hello ...')
    await asyncio.sleep(1)
    print('... World!')
#enddef


if __name__ == "__main__":
    asyncio.run(main())
#endif
```

*cpp*
```cpp
#include <iostream>
#include <asyncio.hpp>
using namespace kwa;


asyncio::Task<> async_main() {
    std::cout << "Hello ..." << std::endl;
    co_await asyncio::sleep<1000>();
    std::cout << "... World" << std::endl;
}


int main() {
    asyncio::run(async_main());
}
```

`Task` object is awaitable and contains methods below:

```cpp
result_type result();
void cancel();
bool canceled();
bool done();
void add_done_callback(std::function<void(const result_type&)>);
```

timer callback:

```cpp
void call_soon(EventLoopCallback&& callback);
void call_soon(Handle& handle);
void call_soon_threadsafe(EventLoopCallback&& callback);
std::shared_ptr<Timer> call_at(TimePoint when, EventLoopCallback&& callback);
std::shared_ptr<Timer> call_later(std::chrono::milliseconds delay, EventLoopCallback&& callback);
```

`Timer` object is able to cancel.

thread executor:

```cpp
template<typename F, typename... Args>
requires requires(F f, Args... args) { f(args...); }
[[nodiscard]] std::shared_ptr<FutureAwaiter<result_type>> run_in_thread(F&& f, Args&&... args);
```

`run_in_thread` will submit `f` with `args` to thread pool, and return an awaitable object
`FutureAwaiter`.
