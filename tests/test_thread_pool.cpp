#include <gtest/gtest.h>
#include "netscope/core/thread_pool.h"

#include <atomic>
#include <chrono>

using namespace netscope::core;

TEST(ThreadPoolTest, CreateAndDestroy) {
    EXPECT_NO_THROW({
        ThreadPool pool(4);
    });
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
            counter.fetch_add(1);
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, ThreadCount) {
    ThreadPool pool(8);
    EXPECT_EQ(pool.ThreadCount(), 8);
}

TEST(ThreadPoolTest, Resize) {
    ThreadPool pool(4);
    EXPECT_EQ(pool.ThreadCount(), 4);
    pool.Resize(8);
    EXPECT_EQ(pool.ThreadCount(), 8);
    pool.Resize(2);
    EXPECT_EQ(pool.ThreadCount(), 2);
}

TEST(ThreadPoolTest, WaitAll) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 50; ++i) {
        pool.Enqueue([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            counter.fetch_add(1);
        });
    }

    pool.WaitAll();
    EXPECT_EQ(counter.load(), 50);
}

TEST(ThreadPoolTest, StringResult) {
    ThreadPool pool(2);
    auto future = pool.Enqueue([]() {
        return std::string("hello");
    });
    EXPECT_EQ(future.get(), "hello");
}
