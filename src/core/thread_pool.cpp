#include "netscope/core/thread_pool.h"

#include <algorithm>

namespace netscope {
namespace core {

ThreadPool::ThreadPool(size_t num_threads) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 2;
        }
    }
    CreateWorkers(num_threads);
}

ThreadPool::~ThreadPool() {
    DestroyWorkers();
}

void ThreadPool::Worker() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] {
                return stop_.load(std::memory_order_acquire) || !tasks_.empty();
            });

            if (stop_.load(std::memory_order_acquire) && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();

        pending_.fetch_sub(1, std::memory_order_release);
    }
}

void ThreadPool::CreateWorkers(size_t count) {
    for (size_t i = 0; i < count; ++i) {
        workers_.emplace_back(&ThreadPool::Worker, this);
    }
}

void ThreadPool::DestroyWorkers() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_.store(true, std::memory_order_release);
    }

    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
}

void ThreadPool::WaitAll() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (tasks_.empty()) {
                break;
            }
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
    if (new_size == workers_.size()) {
        return;
    }

    if (new_size > workers_.size()) {
        CreateWorkers(new_size - workers_.size());
    } else {
        auto to_remove = workers_.size() - new_size;
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_.store(true, std::memory_order_release);
        }
        condition_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        workers_.clear();
        stop_.store(false, std::memory_order_release);
        CreateWorkers(new_size);
    }
}

} // namespace core
} // namespace netscope
