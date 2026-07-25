#include "netscope/scan/service_detector.h"
#include "netscope/network/socket.h"
#include "netscope/core/logger.h"

#include <cstring>
#include <algorithm>
#include <chrono>

namespace netscope {
namespace scan {

const std::unordered_map<int, std::string> ServiceDetector::kKnownServices = {
    {21, "FTP"}, {22, "SSH"}, {23, "Telnet"},
    {25, "SMTP"}, {53, "DNS"}, {80, "HTTP"},
    {110, "POP3"}, {135, "RPC"}, {139, "NetBIOS"},
    {143, "IMAP"}, {443, "HTTPS"}, {445, "SMB"},
    {3306, "MySQL"}, {3389, "RDP"}, {8080, "HTTP-Alt"}
};

ServiceDetector::ServiceDetector() {
    pool_ = std::make_unique<core::ThreadPool>(max_threads_);
    core::Logger::Instance().Debug("ServiceDetector created");
}

ServiceDetector::~ServiceDetector() = default;

void ServiceDetector::SetMaxThreads(int threads) {
    max_threads_ = threads;
    pool_->Resize(threads);
}

void ServiceDetector::SetTimeout(int timeout_ms) {
    timeout_ms_ = timeout_ms;
}

std::vector<ServiceInfo> ServiceDetector::DetectServices(const std::string& ip,
                                                          const std::vector<int>& ports) {
    std::vector<ServiceInfo> services;
    services.reserve(ports.size());

    std::mutex services_mutex;
    std::vector<std::future<void>> futures;

    for (int port : ports) {
        futures.push_back(pool_->Enqueue([this, ip, port, &services, &services_mutex]() {
            auto banner = GrabBanner(ip, port);
            if (banner.has_value()) {
                std::lock_guard<std::mutex> lock(services_mutex);
                services.push_back(banner.value());
            }
        }));
    }

    for (auto& f : futures) f.get();

    core::Logger::Instance().Info("Service detection for " + ip + ": "
                                  + std::to_string(services.size()) + " services");
    return services;
}

std::optional<ServiceInfo> ServiceDetector::GrabBanner(const std::string& ip,
                                                        int port,
                                                        const std::string& protocol) {
    ServiceInfo info;
    info.port = port;
    info.protocol = protocol;
    info.name = IdentifyService(port);

    try {
        network::WinSockGuard guard;

        socket_t fd = network::CreateSocket(AF_INET, SOCK_STREAM, 0);
        if (fd == INVALID_SOCK) return std::nullopt;

        network::SetSocketTimeout(fd, timeout_ms_);

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(port));
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        if (!network::ConnectWithTimeout(fd, addr, timeout_ms_)) {
            network::CloseSocket(fd);
            return std::nullopt;
        }

        char buffer[2048] = {0};
#ifdef _WIN32
        int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
#else
        int bytes = read(fd, buffer, sizeof(buffer) - 1);
#endif

        if (bytes > 0) {
            buffer[bytes] = '\0';
            info.banner = std::string(buffer);
            info.version = ParseVersion(info.banner);
            info.confidence = 0.8;
        } else {
            info.banner = "(no banner)";
            info.confidence = 0.4;
        }

        network::CloseSocket(fd);
    } catch (const std::exception& e) {
        core::Logger::Instance().Debug("Banner grab error " + ip + ":"
                                       + std::to_string(port) + " - " + e.what());
        return std::nullopt;
    }

    if (!info.banner.empty()) core::Logger::Instance().Debug(
        "Banner " + ip + ":" + std::to_string(port) + " - "
        + info.banner.substr(0, 60));

    return info;
}

std::string ServiceDetector::IdentifyService(int port) {
    auto it = kKnownServices.find(port);
    return (it != kKnownServices.end()) ? it->second : "Unknown";
}

std::string ServiceDetector::ParseVersion(const std::string& banner) {
    if (banner.empty()) return "";

    auto end = banner.find("\r\n");
    if (end == std::string::npos) end = banner.find("\n");
    if (end == std::string::npos) end = std::min(banner.length(), size_t(120));

    std::string first_line = banner.substr(0, end);

    if (first_line.length() > 80) first_line = first_line.substr(0, 80);

    return first_line;
}

} // namespace scan
} // namespace netscope
