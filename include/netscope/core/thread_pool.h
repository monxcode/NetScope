#ifndef NETSCOPE_CORE_THREAD_POOL_H
#define NETSCOPE_CORE_THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <memory>
#include <type_traits>

namespace netscope {
namespace core {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = 0);
    ~ThreadPool();

    template <typename F, typename... Args>
    auto Enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result_t<F, Args...>>;

    void WaitAll();
    size_t Pending() const;
    size_t ThreadCount() const;
    void Resize(size_t new_size);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

private:
    void Worker();
    void CreateWorkers(size_t count);
    void DestroyWorkers();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> pending_{0};
};

template <typename F, typename... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result_t<F, Args...>> {

    using return_type = typename std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (stop_) {
            throw std::runtime_error("enqueue on stopped thread pool");
        }
        tasks_.emplace([task]() { (*task)(); });
        pending_.fetch_add(1, std::memory_order_release);
    }

    condition_.notify_one();
    return result;
}

} // namespace core
} // namespace netscope

#endif
