#include "netscope/scan/service_detector.h"
#include "netscope/network/socket.h"
#include "netscope/core/logger.h"

#include <cstring>
#include <algorithm>

namespace netscope {
namespace scan {

const std::unordered_map<int, std::string> ServiceDetector::kKnownServices = {
    {21, "FTP"}, {22, "SSH"}, {23, "Telnet"},
    {25, "SMTP"}, {53, "DNS"}, {80, "HTTP"},
    {110, "POP3"}, {135, "RPC"}, {139, "NetBIOS"},
    {143, "IMAP"}, {443, "HTTPS"}, {445, "SMB"},
    {3306, "MySQL"}, {3389, "RDP"}, {8080, "HTTP-Alt"}
};

std::vector<ServiceInfo> ServiceDetector::DetectServices(const std::string& ip,
                                                          const std::vector<int>& ports) {
    std::vector<ServiceInfo> services;

    for (int port : ports) {
        auto banner = GrabBanner(ip, port);
        if (banner.has_value()) {
            services.push_back(banner.value());
        }
    }

    return services;
}

std::optional<ServiceInfo> ServiceDetector::GrabBanner(const std::string& ip,
                                                        int port,
                                                        const std::string& protocol) {
    ServiceInfo info;
    info.port = port;
    info.protocol = protocol;

    try {
        network::WinSockGuard guard;

        socket_t fd = network::CreateSocket(AF_INET, SOCK_STREAM, 0);
        if (fd == INVALID_SOCK) {
            return std::nullopt;
        }

        network::SetTimeout(fd, 3000);

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(port));
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        if (!network::ConnectWithTimeout(fd, addr, 3000)) {
            network::CloseSocket(fd);
            return std::nullopt;
        }

        char buffer[1024] = {0};
#ifdef _WIN32
        int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
#else
        int bytes = read(fd, buffer, sizeof(buffer) - 1);
#endif

        if (bytes > 0) {
            buffer[bytes] = '\0';
            info.banner = std::string(buffer);
            info.name = IdentifyService(port);
            info.version = ParseBanner(info.banner);
            info.confidence = 0.8;
        }

        network::CloseSocket(fd);
    } catch (const std::exception& e) {
        core::Logger::Instance().Debug("Banner grab error on " + ip + ":" +
                                        std::to_string(port) + " - " + e.what());
        return std::nullopt;
    }

    return info;
}

std::string ServiceDetector::IdentifyService(int port) {
    auto it = kKnownServices.find(port);
    if (it != kKnownServices.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string ServiceDetector::ParseBanner(const std::string& banner) {
    auto version = DetectVersion(banner, "");
    return version;
}

std::string ServiceDetector::DetectVersion(const std::string& banner,
                                            const std::string& service) {
    if (banner.empty()) return "";

    auto end = banner.find("\r\n");
    if (end == std::string::npos) {
        end = banner.find("\n");
    }
    if (end == std::string::npos) {
        end = std::min(banner.length(), size_t(100));
    }

    return banner.substr(0, end);
}

} // namespace scan
} // namespace netscope
