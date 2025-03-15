#pragma once
#define ASYNCIO_NS_BEGIN(...) namespace kwa::asyncio __VA_OPT__(::) __VA_ARGS__ {
#define ASYNCIO_NS_END }
