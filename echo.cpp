#include "asyncio.hpp"
using namespace kwa;


asyncio::Task<> response(int conn) {
    asyncio::Socket s { conn };
    constexpr auto SIZE = 1024;
    char buf[SIZE];
    while (true) {
        bzero(buf, SIZE);
        auto res = co_await s.read(buf, SIZE);
        if (!res) {
            break;
        }
        SPDLOG_INFO("recv from socket fd {}: {}", conn, buf);
    }
}


asyncio::Task<> start_server() noexcept {
    auto s = asyncio::Socket();
    if (auto res = s.bind("127.0.0.1", 12345); !res) {
        SPDLOG_ERROR(res.error().message());
        co_return;
    }
    if (auto res = s.listen(10); !res) {
        SPDLOG_ERROR(res.error().message());
        co_return;
    }
    std::vector<asyncio::Task<>> tasks;
    while (true) {
        auto conn = co_await s.accept();
        SPDLOG_INFO("accept socket fd {}", conn);
        tasks.push_back(response(conn));
    }
}


int main() {
    auto res = asyncio::run(start_server());
    if (!res) {
        SPDLOG_ERROR(res.error().message());
    }
}
