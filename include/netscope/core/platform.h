#ifndef NETSCOPE_CORE_PLATFORM_H
#define NETSCOPE_CORE_PLATFORM_H

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <chrono>

namespace netscope {
namespace core {

enum class PlatformType {
    Windows,
    Linux,
    Unknown
};

struct NetworkInterface {
    std::string name;
    std::string description;
    std::string ip_address;
    std::string netmask;
    std::string broadcast;
    std::string mac_address;
    bool is_loopback;
    bool is_up;
};

class Platform {
public:
    static PlatformType Detect();
    static std::string OSName();
    static bool IsAdmin();
    static std::vector<NetworkInterface> EnumerateInterfaces();
    static std::string GetHostname();
    static bool PingHost(const std::string& ip, int timeout_ms, int retries = 1);
    static std::optional<std::string> ResolveHostname(const std::string& ip);
    static std::chrono::milliseconds GetTimestamp();

    Platform() = delete;

private:
    static PlatformType cached_platform_;
};

} // namespace core
} // namespace netscope

#endif
