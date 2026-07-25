#include "netscope/discovery/device.h"

#include <sstream>
#include <iomanip>

namespace netscope {
namespace discovery {

nlohmann::json Device::ToJSON() const {
    nlohmann::json j;
    j["ip"] = ip_;
    j["mac"] = mac_;
    j["hostname"] = hostname_;
    j["vendor"] = vendor_;
    j["online"] = online_;
    j["response_time_ms"] = response_time_ms_;
    j["ttl"] = ttl_;
    j["os"] = {{"name", os_.name}, {"confidence", os_.confidence}, {"ttl", os_.ttl}};
    j["ports"] = nlohmann::json::array();
    for (const auto& port : ports_) {
        j["ports"].push_back({
            {"port", port.port}, {"protocol", port.protocol},
            {"open", port.open}, {"service", port.service}, {"banner", port.banner}
        });
    }
    return j;
}

std::string Device::ToCSV() const {
    std::ostringstream oss;
    oss << ip_ << ","
        << mac_ << ","
        << "\"" << hostname_ << "\","
        << "\"" << vendor_ << "\","
        << (online_ ? "Online" : "Offline") << ","
        << os_.name << ","
        << os_.ttl << ","
        << response_time_ms_;
    return oss.str();
}

std::string Device::ToString() const {
    std::ostringstream oss;
    oss << "Device: " << ip_;
    if (!hostname_.empty()) oss << " (" << hostname_ << ")";
    oss << "\n  MAC: " << (mac_.empty() ? "N/A" : mac_);
    oss << "\n  Vendor: " << (vendor_.empty() ? "N/A" : vendor_);
    oss << "\n  Status: " << (online_ ? "Online" : "Offline");
    oss << "\n  OS: " << os_.name << " (" << std::fixed << std::setprecision(0)
        << (os_.confidence * 100) << "%)";
    oss << "\n  Response: " << response_time_ms_ << "ms";
    return oss.str();
}

} // namespace discovery
} // namespace netscope
