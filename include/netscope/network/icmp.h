#ifndef NETSCOPE_NETWORK_ICMP_H
#define NETSCOPE_NETWORK_ICMP_H

#include <string>
#include <chrono>
#include <optional>

namespace netscope {
namespace network {

struct ICMPReply {
    std::string ip;
    int ttl;
    std::chrono::milliseconds rtt;
    bool success;
    int bytes_received;
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
#ifdef _WIN32
    HANDLE icmp_handle_{nullptr};
#else
    int raw_socket_{-1};
#endif
};

} // namespace network
} // namespace netscope

#endif
