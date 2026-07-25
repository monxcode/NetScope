#ifndef NETSCOPE_NETWORK_ICMP_H
#define NETSCOPE_NETWORK_ICMP_H

#include <string>
#include <chrono>
#include <optional>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif

namespace netscope {
namespace network {

struct ICMPReply {
    std::string ip;
    int ttl{0};
    std::chrono::milliseconds rtt{0};
    bool success{false};
    int bytes_received{0};
};

class ICMPScanner {
public:
    explicit ICMPScanner(int timeout_ms = 1000, int retries = 2);
    ~ICMPScanner();

    std::optional<ICMPReply> Ping(const std::string& ip);
    bool IsReachable(const std::string& ip);

    ICMPScanner(const ICMPScanner&) = delete;
    ICMPScanner& operator=(const ICMPScanner&) = delete;

private:
    bool Initialize();
    void Cleanup();

    int timeout_ms_;
    int retries_;
    bool initialized_{false};
#ifdef _WIN32
    HANDLE icmp_handle_{nullptr};
#else
    int raw_socket_{-1};
#endif
};

} // namespace network
} // namespace netscope

#endif
