#include <gtest/gtest.h>
#include "netscope/core/thread_pool.h"

#include <atomic>
#include <chrono>

using namespace netscope::core;

TEST(ThreadPoolTest, CreateAndDestroy) {
    EXPECT_NO_THROW({ ThreadPool pool(4); });
}

TEST(ThreadPoolTest, ExecuteTask) {
    ThreadPool pool(2);
    auto future = pool.Enqueue([]() { return 42; });
    EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPoolTest, MultipleTasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.Enqueue([&counter]() {
            counter.fetch_add(1, std::memory_order_release);
        }));
    }
    for (auto& f : futures) f.get();
    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, ThreadCount) {
    ThreadPool pool(8);
    EXPECT_EQ(pool.ThreadCount(), 8);
}

TEST(ThreadPoolTest, StringResult) {
    ThreadPool pool(2);
    auto future = pool.Enqueue([]() { return std::string("ok"); });
    EXPECT_EQ(future.get(), "ok");
}
