#include "netscope/scan/port_scanner.h"
#include "netscope/network/socket.h"
#include "netscope/core/logger.h"
#include "netscope/core/config.h"

#include <cstring>
#include <unordered_map>
#include <chrono>

namespace netscope {
namespace scan {

static const std::unordered_map<int, std::string> kServiceMap = {
    {21, "FTP"}, {22, "SSH"}, {23, "Telnet"},
    {25, "SMTP"}, {53, "DNS"}, {80, "HTTP"},
    {110, "POP3"}, {135, "RPC"}, {139, "NetBIOS"},
    {143, "IMAP"}, {443, "HTTPS"}, {445, "SMB"},
    {3306, "MySQL"}, {3389, "RDP"}, {8080, "HTTP-Alt"}
};

PortScanner::PortScanner() {
    const auto cfg = core::Config::Instance().Get();
    timeout_ms_ = cfg.network.timeout_ms;
    max_threads_ = cfg.network.max_threads;
    pool_ = std::make_unique<core::ThreadPool>(max_threads_);
    core::Logger::Instance().Debug("PortScanner created");
}

PortScanner::~PortScanner() = default;

void PortScanner::SetProgressCallback(ProgressCallback callback) {
    callback_ = std::move(callback);
}

void PortScanner::SetMaxThreads(int threads) {
    max_threads_ = threads;
    pool_->Resize(threads);
}

void PortScanner::SetTimeout(int timeout_ms) {
    timeout_ms_ = timeout_ms;
}

std::vector<PortResult> PortScanner::Scan(const std::string& ip,
                                           const std::vector<int>& ports) {
    if (ports.empty()) return {};
    running_.store(true, std::memory_order_release);
    cancelled_.store(false, std::memory_order_release);

    std::vector<PortResult> results(ports.size());
    std::atomic<int> completed{0};

    PortScanProgress progress;
    progress.total = static_cast<int>(ports.size());

    std::vector<std::future<void>> futures;
    futures.reserve(ports.size());

    for (size_t i = 0; i < ports.size(); ++i) {
        futures.push_back(pool_->Enqueue([this, ip, &ports, &results, i,
                                           &completed, &progress]() {
            if (cancelled_.load(std::memory_order_acquire)) return;

            results[i] = ScanPort(ip, ports[i]);

            int done = completed.fetch_add(1, std::memory_order_release) + 1;
            if (callback_) {
                int open_count = 0;
                for (int j = 0; j < done && j < static_cast<int>(results.size()); ++j) {
                    if (results[j].open) ++open_count;
                }
                progress.completed = done;
                progress.open = open_count;
                callback_(progress);
            }
        }));
    }

    for (auto& f : futures) f.get();

    running_.store(false, std::memory_order_release);

    std::vector<PortResult> open_ports;
    open_ports.reserve(results.size());
    for (auto& r : results) {
        if (r.open) {
            r.protocol = "tcp";
            open_ports.push_back(std::move(r));
        }
    }

    core::Logger::Instance().Info("Port scan " + ip + ": "
                                  + std::to_string(open_ports.size())
                                  + "/" + std::to_string(ports.size()) + " open");
    return open_ports;
}

std::vector<PortResult> PortScanner::ScanRange(const std::string& ip,
                                                int start, int end) {
    if (start > end || start < 1 || end > 65535) return {};
    std::vector<int> ports;
    ports.reserve(static_cast<size_t>(end - start + 1));
    for (int p = start; p <= end; ++p) ports.push_back(p);
    return Scan(ip, ports);
}

void PortScanner::ScanAsync(const std::string& ip,
                             const std::vector<int>& ports,
                             std::function<void(std::vector<PortResult>)> on_complete) {
    pool_->Enqueue([this, ip, ports, on_complete = std::move(on_complete)]() {
        auto results = Scan(ip, ports);
        if (on_complete) on_complete(std::move(results));
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
    result.protocol = "tcp";

    try {
        network::WinSockGuard guard;
        socket_t fd = network::CreateSocket(AF_INET, SOCK_STREAM, 0);
        if (fd == INVALID_SOCK) return result;

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(port));
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        bool connected = network::ConnectWithTimeout(fd, addr, timeout_ms_);
        if (connected) {
            result.open = true;
            auto it = kServiceMap.find(port);
            if (it != kServiceMap.end()) result.service = it->second;
        }

        network::CloseSocket(fd);
    } catch (const std::exception& e) {
        core::Logger::Instance().Debug("Port scan error " + ip + ":"
                                       + std::to_string(port) + " - " + e.what());
    }

    return result;
}

} // namespace scan
} // namespace netscope
