#include "netscope/scan/ping_sweep.h"
#include "netscope/network/icmp.h"
#include "netscope/network/socket.h"
#include "netscope/core/logger.h"
#include "netscope/core/config.h"

#include <sstream>
#include <algorithm>
#include <chrono>

namespace netscope {
namespace scan {

PingSweep::PingSweep() {
    const auto cfg = core::Config::Instance().Get();
    timeout_ms_ = cfg.network.timeout_ms;
    max_threads_ = cfg.network.max_threads;
    pool_ = std::make_unique<core::ThreadPool>(max_threads_);
}

PingSweep::~PingSweep() = default;

void PingSweep::SetProgressCallback(ProgressCallback callback) {
    callback_ = std::move(callback);
}

void PingSweep::SetMaxThreads(int threads) {
    max_threads_ = threads;
    pool_->Resize(threads);
}

void PingSweep::SetTimeout(int timeout_ms) {
    timeout_ms_ = timeout_ms;
}

std::vector<discovery::Device> PingSweep::Sweep(const std::string& subnet) {
    cancelled_.store(false, std::memory_order_release);

    auto slash_pos = subnet.find('/');
    if (slash_pos == std::string::npos) {
        core::Logger::Instance().Error("PingSweep: no CIDR prefix in " + subnet);
        return {};
    }

    std::string base = subnet.substr(0, slash_pos);
    int prefix = std::stoi(subnet.substr(slash_pos + 1));

    unsigned long addr = inet_addr(base.c_str());
    unsigned long mask = htonl(~((1u << (32 - prefix)) - 1));
    unsigned long network = addr & mask;
    unsigned long broadcast = network | ~mask;
    unsigned long total = ntohl(broadcast) - ntohl(network) - 1;
    if (total > 1024) total = 1024;

    std::vector<discovery::Device> results;
    std::mutex results_mutex;
    std::atomic<int> completed{0};
    std::atomic<int> alive{0};

    PingSweepProgress progress;
    progress.total = static_cast<int>(total);

    auto start_time = std::chrono::steady_clock::now();

    std::vector<std::future<void>> futures;
    futures.reserve(total);

    for (unsigned long i = 0; i < total; ++i) {
        unsigned long host_addr = htonl(ntohl(network) + i + 1);
        struct in_addr in;
        in.s_addr = host_addr;
        std::string ip = inet_ntoa(in);

        futures.push_back(pool_->Enqueue([this, ip, &results, &results_mutex,
                                           &completed, &alive, &progress,
                                           &start_time]() {
            if (cancelled_.load(std::memory_order_acquire)) return;

            network::ICMPScanner icmp(timeout_ms_, 1);
            bool reachable = icmp.IsReachable(ip);

            if (reachable) {
                discovery::Device device;
                device.SetIP(ip);
                device.SetOnline(true);
                device.UpdateLastSeen();

                {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    results.push_back(std::move(device));
                }
                alive.fetch_add(1, std::memory_order_release);
            }

            int done = completed.fetch_add(1, std::memory_order_release) + 1;
            if (callback_) {
                progress.completed = done;
                progress.alive = alive.load(std::memory_order_acquire);
                callback_(progress);
            }
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    core::Logger::Instance().Info("Ping sweep: " + std::to_string(results.size())
                                  + " hosts alive in " + subnet);
    return results;
}

void PingSweep::Cancel() {
    cancelled_.store(true, std::memory_order_release);
}

} // namespace scan
} // namespace netscope
