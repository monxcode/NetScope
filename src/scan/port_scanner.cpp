#include "netscope/scan/port_scanner.h"
#include "netscope/network/socket.h"
#include "netscope/core/logger.h"
#include "netscope/core/config.h"

#include <cstring>
#include <algorithm>
#include <unordered_map>

namespace netscope {
namespace scan {

PortScanner::PortScanner() {
    auto& cfg = core::Config::Instance().Get();
    timeout_ms_ = cfg.network.timeout_ms;
    pool_ = std::make_unique<core::ThreadPool>(cfg.network.max_threads);
}

PortScanner::~PortScanner() = default;

void PortScanner::SetProgressCallback(ProgressCallback callback) {
    callback_ = std::move(callback);
}

void PortScanner::SetMaxThreads(int threads) {
    pool_->Resize(threads);
}

void PortScanner::SetTimeout(int timeout_ms) {
    timeout_ms_ = timeout_ms;
}

std::vector<PortResult> PortScanner::Scan(const std::string& ip,
                                           const std::vector<int>& ports) {
    running_.store(true, std::memory_order_release);
    cancelled_.store(false, std::memory_order_release);

    std::vector<PortResult> results(ports.size());
    std::atomic<int> completed{0};

    PortScanProgress progress;
    progress.total = static_cast<int>(ports.size());

    std::vector<std::future<void>> futures;
    for (size_t i = 0; i < ports.size(); ++i) {
        futures.push_back(pool_->Enqueue([this, ip, &ports, &results, i, &completed, &progress]() {
            if (cancelled_.load(std::memory_order_acquire)) return;

            results[i] = ScanPort(ip, ports[i]);

            int done = completed.fetch_add(1, std::memory_order_release) + 1;
            if (callback_) {
                progress.completed = done;
                progress.open = static_cast<int>(
                    std::count_if(results.begin(), results.begin() + done,
                                  [](const PortResult& r) { return r.open; }));
                callback_(progress);
            }
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    running_.store(false, std::memory_order_release);

    std::vector<PortResult> open_ports;
    for (auto& r : results) {
        if (r.open) {
            r.protocol = "tcp";
            open_ports.push_back(std::move(r));
        }
    }

    core::Logger::Instance().Info("Port scan for " + ip + ": " +
                                  std::to_string(open_ports.size()) + " open ports");
    return open_ports;
}

std::vector<PortResult> PortScanner::ScanRange(const std::string& ip,
                                                int start, int end) {
    std::vector<int> ports;
    for (int p = start; p <= end; ++p) {
        ports.push_back(p);
    }
    return Scan(ip, ports);
}

void PortScanner::ScanAsync(const std::string& ip,
                             const std::vector<int>& ports,
                             std::function<void(std::vector<PortResult>)> on_complete) {
    pool_->Enqueue([this, ip, ports, on_complete = std::move(on_complete)]() {
        auto results = Scan(ip, ports);
        if (on_complete) {
            on_complete(std::move(results));
        }
    });
}

void PortScanner::Cancel() {
    cancelled_.store(true, std::memory_order_release);
}

bool PortScanner::IsRunning() const {
    return running_.load(std::memory_order_acquire);
}

PortResult PortScanner::ScanPort(const std::string& ip, int port) {
    PortResult result;
    result.port = port;

    try {
        network::WinSockGuard guard;

        socket_t fd = network::CreateSocket(AF_INET, SOCK_STREAM, 0);
        if (fd == INVALID_SOCK) {
            return result;
        }

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(port));
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        bool connected = network::ConnectWithTimeout(fd, addr, timeout_ms_);
        if (connected) {
            result.open = true;

            static const std::unordered_map<int, std::string> kServices = {
                {21, "FTP"}, {22, "SSH"}, {23, "Telnet"},
                {25, "SMTP"}, {53, "DNS"}, {80, "HTTP"},
                {110, "POP3"}, {135, "RPC"}, {139, "NetBIOS"},
                {143, "IMAP"}, {443, "HTTPS"}, {445, "SMB"},
                {3306, "MySQL"}, {3389, "RDP"}, {8080, "HTTP-Alt"}
            };

            auto it = kServices.find(port);
            if (it != kServices.end()) {
                result.service = it->second;
            }
        }

        network::CloseSocket(fd);
    } catch (const std::exception& e) {
        core::Logger::Instance().Debug("Port scan error on " + ip + ":" +
                                        std::to_string(port) + " - " + e.what());
    }

    return result;
}

} // namespace scan
} // namespace netscope
