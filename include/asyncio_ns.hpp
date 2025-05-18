#pragma once
#define ASYNCIO_NS kwa::asyncio
#define ASYNCIO_NS_BEGIN(...) namespace ASYNCIO_NS __VA_OPT__(::) __VA_ARGS__ {
#define ASYNCIO_NS_END }
