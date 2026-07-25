#include "netscope/core/thread_pool.h"

#include <algorithm>

namespace netscope {
namespace core {

ThreadPool::ThreadPool(size_t num_threads) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 2;
    }
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::Worker, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_.store(true, std::memory_order_release);
    }
    condition_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
}

void ThreadPool::Worker() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] {
                return stop_.load(std::memory_order_acquire) || !tasks_.empty();
            });
            if (stop_.load(std::memory_order_acquire) && tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
        pending_.fetch_sub(1, std::memory_order_release);
    }
}

void ThreadPool::WaitAll() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (tasks_.empty()) break;
        }
        std::this_thread::yield();
    }
}

size_t ThreadPool::Pending() const {
    return pending_.load(std::memory_order_acquire);
}

size_t ThreadPool::ThreadCount() const {
    return workers_.size();
}

void ThreadPool::Resize(size_t new_size) {
    if (new_size == workers_.size()) return;
    if (new_size > workers_.size()) {
        workers_.reserve(new_size);
        for (size_t i = workers_.size(); i < new_size; ++i) {
            workers_.emplace_back(&ThreadPool::Worker, this);
        }
    }
}

} // namespace core
} // namespace netscope
