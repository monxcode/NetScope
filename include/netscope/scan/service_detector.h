#ifndef NETSCOPE_SCAN_SERVICE_DETECTOR_H
#define NETSCOPE_SCAN_SERVICE_DETECTOR_H

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace netscope {
namespace scan {

struct ServiceInfo {
    int port;
    std::string protocol;
    std::string name;
    std::string banner;
    std::string version;
    double confidence;
};

class ServiceDetector {
public:
    ServiceDetector() = default;

    std::vector<ServiceInfo> DetectServices(const std::string& ip,
                                             const std::vector<int>& ports);
    std::optional<ServiceInfo> GrabBanner(const std::string& ip, int port,
                                          const std::string& protocol = "tcp");

    ServiceDetector(const ServiceDetector&) = delete;
    ServiceDetector& operator=(const ServiceDetector&) = delete;

private:
    std::string IdentifyService(int port);
    std::string ParseBanner(const std::string& banner);
    std::string DetectVersion(const std::string& banner, const std::string& service);

    static const std::unordered_map<int, std::string> kKnownServices;
};

} // namespace scan
} // namespace netscope

#endif
