#include <boost/ut.hpp>

#include "asyncio.hpp"
using namespace kwa;
using namespace boost::ut;


template<size_t N>
asyncio::Task<> coro_depth_n(std::vector<int>& result) {
    result.push_back(N);
    if constexpr (N > 0) {
        co_await coro_depth_n<N - 1>(result);
        result.push_back(N * 10);
    }
}

asyncio::Task<int64_t> square(int64_t x) {
    co_return x * x;
}


int main() {
    "task test"_test = []() {
        "simple await"_test = []() {
            std::vector<int> result;
            auto res = asyncio::run(coro_depth_n<0>(result));
            expect(res.has_value());
            expect(result == std::vector<int>{0});
        };
        "nest await"_test = []() {
            std::vector<int> result;
            auto res = asyncio::run(coro_depth_n<1>(result));
            expect(res.has_value());
            expect(result == std::vector<int>{1, 0, 10});
        };
        "3 depth await"_test = []() {
            std::vector<int> result;
            auto res = asyncio::run(coro_depth_n<2>(result));
            expect(res.has_value());
            expect(result == std::vector<int>{2, 1, 0, 10, 20});
        };
        "4 depth await"_test = []() {
            std::vector<int> result;
            auto res = asyncio::run(coro_depth_n<3>(result));
            expect(res.has_value());
            expect(result == std::vector<int>{ 3, 2, 1, 0, 10, 20, 30 });
        };
        "5 depth await"_test = []() {
            std::vector<int> result;
            auto res = asyncio::run(coro_depth_n<4>(result));
            expect(res.has_value());
            expect(result == std::vector<int>{4, 3, 2, 1, 0, 10, 20, 30, 40});
        };
    };

    "Task<> test"_test = []() {
        bool called { false };
        auto res = asyncio::run(
            [&]() -> asyncio::Task<> {
                auto t = square(5);
                auto tt = std::move(t);
                auto res = co_await t;
                expect(!t.valid());
                expect(tt.valid());
                expect(!res.has_value());
                called = true;
            }()
        );
        expect(res.has_value());
        expect(called) << "task not run successfully";
    };
}
