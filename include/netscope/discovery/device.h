#ifndef NETSCOPE_DISCOVERY_DEVICE_H
#define NETSCOPE_DISCOVERY_DEVICE_H

#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "netscope/scan/port_scanner.h"
#include "netscope/scan/service_detector.h"

namespace netscope {
namespace discovery {

struct OSGuess {
    std::string name;
    double confidence{0.0};
    int ttl{0};
};

class Device {
public:
    Device() = default;

    const std::string& IP() const { return ip_; }
    void SetIP(const std::string& ip) { ip_ = ip; }

    const std::string& MAC() const { return mac_; }
    void SetMAC(const std::string& mac) { mac_ = mac; }

    const std::string& Hostname() const { return hostname_; }
    void SetHostname(const std::string& hostname) { hostname_ = hostname; }

    const std::string& Vendor() const { return vendor_; }
    void SetVendor(const std::string& vendor) { vendor_ = vendor; }

    bool Online() const { return online_; }
    void SetOnline(bool online) { online_ = online; }

    const OSGuess& OS() const { return os_; }
    void SetOS(const OSGuess& os) { os_ = os; }

    int ResponseTimeMs() const { return response_time_ms_; }
    void SetResponseTimeMs(int ms) { response_time_ms_ = ms; }

    int TTL() const { return ttl_; }
    void SetTTL(int ttl) { ttl_ = ttl; }

    const std::vector<scan::PortResult>& OpenPorts() const { return ports_; }
    void SetOpenPorts(const std::vector<scan::PortResult>& ports) { ports_ = ports; }
    void AddOpenPort(const scan::PortResult& port) { ports_.push_back(port); }

    const std::vector<scan::ServiceInfo>& Services() const { return services_; }
    void SetServices(const std::vector<scan::ServiceInfo>& services) { services_ = services; }

    std::chrono::steady_clock::time_point LastSeen() const { return last_seen_; }
    void UpdateLastSeen() { last_seen_ = std::chrono::steady_clock::now(); }

    bool operator==(const Device& other) const { return ip_ == other.ip_; }
    bool operator<(const Device& other) const { return ip_ < other.ip_; }

    nlohmann::json ToJSON() const;
    std::string ToCSV() const;
    std::string ToString() const;

private:
    std::string ip_;
    std::string mac_;
    std::string hostname_;
    std::string vendor_;
    bool online_{false};
    OSGuess os_;
    int response_time_ms_{0};
    int ttl_{0};
    std::vector<scan::PortResult> ports_;
    std::vector<scan::ServiceInfo> services_;
    std::chrono::steady_clock::time_point last_seen_;
};

} // namespace discovery
} // namespace netscope

#endif
