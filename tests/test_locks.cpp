#include <asyncio.hpp>
#include <boost/ut.hpp>
using namespace kwa;
using namespace boost::ut;
using namespace boost::ut::bdd;


asyncio::Task<> coro1(asyncio::Mutex& mtx, int& v) {
    asyncio::Lock lock(mtx);
    co_await lock;
    ++v;
    expect(v == 1);
    co_await asyncio::sleep<0>();
    --v;
}


asyncio::Task<> coro2(asyncio::Event<int>& ev, int& v) {
    ++v;
    auto msg = co_await ev.wait();
    expect(*msg == 999);
    ++v;
}


asyncio::Task<> coro3(asyncio::Condition& cond, int& v) {
    co_await cond.wait();
    --v;
}


int main() {
    "lock"_test = [] {
        asyncio::run(
            [] -> asyncio::Task<> {
                asyncio::Mutex mtx;
                int v = 0;
                std::vector<asyncio::Task<>> tasks;
                tasks.reserve(10);
                for (int i = 0; i < 10; ++i) {
                    tasks.push_back(coro1(mtx, v));
                }
                for (auto& task : tasks) {
                    co_await task;
                }
            }()
        );
    };
    "event"_test = [] {
        asyncio::run(
            [] -> asyncio::Task<> {
                asyncio::Event<int> ev;
                int v = 0;
                auto task = coro2(ev, v);
                expect(!ev.is_set());
                co_await asyncio::sleep<0>();
                expect(ev.is_set());
                expect(v == 1);
                ev.set(999);
                co_await task;
                expect(v == 2);
            }()
        );
    };
    "condition"_test = [] {
        asyncio::run(
            [] -> asyncio::Task<> {
                asyncio::Condition cond;
                int v = 10;
                std::vector<asyncio::Task<>> tasks;
                tasks.reserve(v);
                for (int i = 0; i < v; ++i) {
                    tasks.push_back(coro3(cond, v));
                }
                co_await asyncio::sleep<0>();
                expect(v == 10);
                cond.notify_one();
                co_await asyncio::sleep<0>();
                expect(v == 9);
                cond.notify_all();
                co_await asyncio::sleep<0>();
                expect(v == 0);
            }()
        );
    };
}
