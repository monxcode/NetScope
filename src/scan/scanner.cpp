#include "netscope/scan/scanner.h"
#include "netscope/network/icmp.h"
#include "netscope/network/arp.h"
#include "netscope/network/dns.h"
#include "netscope/network/socket.h"
#include "netscope/core/logger.h"
#include "netscope/core/config.h"

#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace netscope {
namespace scan {

Scanner::Scanner() {
    const auto cfg = core::Config::Instance().Get();
    timeout_ms_ = cfg.network.timeout_ms;
    max_threads_ = cfg.network.max_threads;
    pool_ = std::make_unique<core::ThreadPool>(max_threads_);
}

Scanner::~Scanner() = default;

void Scanner::SetProgressCallback(ProgressCallback callback) {
    progress_callback_ = std::move(callback);
}

void Scanner::SetMaxThreads(int threads) {
    max_threads_ = threads;
    pool_->Resize(threads);
}

void Scanner::SetTimeout(int timeout_ms) {
    timeout_ms_ = timeout_ms;
}

std::vector<discovery::Device> Scanner::ScanSubnet(const std::string& subnet) {
    running_.store(true, std::memory_order_release);
    cancelled_.store(false, std::memory_order_release);

    auto hosts = ExpandSubnet(subnet);
    if (hosts.empty()) {
        running_.store(false, std::memory_order_release);
        return {};
    }

    std::vector<discovery::Device> results(hosts.size());
    std::atomic<int> completed{0};
    auto start_time = std::chrono::steady_clock::now();

    ScanProgress progress;
    progress.total_hosts = static_cast<int>(hosts.size());

    std::vector<std::future<void>> futures;
    futures.reserve(hosts.size());

    for (size_t i = 0; i < hosts.size(); ++i) {
        futures.push_back(pool_->Enqueue([this, &results, &hosts, i, &completed,
                                           &progress, &start_time]() {
            if (cancelled_.load(std::memory_order_acquire)) return;

            results[i] = ScanHost(hosts[i]);

            int done = completed.fetch_add(1, std::memory_order_release) + 1;
            if (progress_callback_) {
                int found = 0;
                for (int j = 0; j < done && j < static_cast<int>(results.size()); ++j) {
                    if (results[j].Online()) ++found;
                }
                progress.completed = done;
                progress.found = found;
                progress.elapsed_seconds = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - start_time).count();
                progress_callback_(progress);
            }
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    running_.store(false, std::memory_order_release);

    std::vector<discovery::Device> devices;
    devices.reserve(results.size());
    for (auto& d : results) {
        if (d.Online()) devices.push_back(std::move(d));
    }

    core::Logger::Instance().Info("Scan complete: " + std::to_string(devices.size())
                                  + " devices found in " + subnet);
    return devices;
}

void Scanner::ScanSubnetAsync(const std::string& subnet,
                               std::function<void(std::vector<discovery::Device>)> on_complete) {
    pool_->Enqueue([this, subnet, on_complete = std::move(on_complete)]() {
        auto results = ScanSubnet(subnet);
        if (on_complete) on_complete(std::move(results));
    });
}

void Scanner::Cancel() {
    cancelled_.store(true, std::memory_order_release);
    core::Logger::Instance().Info("Scan cancelled by user");
}

bool Scanner::IsRunning() const {
    return running_.load(std::memory_order_acquire);
}

std::vector<std::string> Scanner::ExpandSubnet(const std::string& subnet) {
    std::vector<std::string> hosts;

    auto slash_pos = subnet.find('/');
    if (slash_pos == std::string::npos) {
        hosts.push_back(subnet);
        return hosts;
    }

    std::string base = subnet.substr(0, slash_pos);
    int prefix = std::stoi(subnet.substr(slash_pos + 1));

    unsigned long addr = inet_addr(base.c_str());
    if (addr == INADDR_NONE) {
        core::Logger::Instance().Error("Scanner: invalid subnet " + subnet);
        return hosts;
    }

    unsigned long mask = (prefix == 0) ? 0 : htonl(~((1u << (32 - prefix)) - 1));
    unsigned long network = addr & mask;
    unsigned long broadcast = network | ~mask;
    unsigned long host_count = ntohl(broadcast) - ntohl(network) - 1;
    if (host_count > 1024) host_count = 1024;

    hosts.reserve(host_count);
    for (unsigned long i = 0; i < host_count; ++i) {
        unsigned long host_addr = htonl(ntohl(network) + i + 1);
        struct in_addr in;
        in.s_addr = host_addr;
        hosts.push_back(inet_ntoa(in));
    }

    return hosts;
}

discovery::Device Scanner::ScanHost(const std::string& ip) {
    discovery::Device device;
    device.SetIP(ip);

    network::ICMPScanner icmp(timeout_ms_, 1);
    auto reply = icmp.Ping(ip);

    if (reply.has_value() && reply->success) {
        device.SetOnline(true);
        device.SetResponseTimeMs(static_cast<int>(reply->rtt.count()));
        device.SetTTL(reply->ttl);

        auto hostname = network::DNSResolver::ResolveHostname(ip);
        if (hostname.has_value()) {
            device.SetHostname(hostname.value());
        }

        network::ARPScanner arp;
        auto arp_entry = arp.Resolve(ip);
        if (arp_entry.has_value()) {
            device.SetMAC(arp_entry->mac_address);
            device.SetVendor(arp_entry->vendor);
        }

        int ttl = reply->ttl;
        discovery::OSGuess os;
        if (ttl <= 64) {
            os = {"Linux/Unix", 0.7, ttl};
        } else if (ttl <= 128) {
            os = {"Windows", 0.7, ttl};
        } else {
            os = {"Network Device", 0.5, ttl};
        }
        device.SetOS(os);

        if (device.MAC().empty() && ttl > 0) {
            std::string fallback_mac;
            for (int b = 0; b < 6; ++b) {
                if (b > 0) fallback_mac += ":";
                fallback_mac += "XX";
            }
            device.SetMAC(fallback_mac);
        }
    } else {
        device.SetOnline(false);
    }

    device.UpdateLastSeen();
    return device;
}

} // namespace scan
} // namespace netscope
