#ifndef NETSCOPE_SCAN_SERVICE_DETECTOR_H
#define NETSCOPE_SCAN_SERVICE_DETECTOR_H

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <memory>

#include "netscope/core/thread_pool.h"

namespace netscope {
namespace scan {

struct ServiceInfo {
    int port{0};
    std::string protocol;
    std::string name;
    std::string banner;
    std::string version;
    double confidence{0.0};
};

class ServiceDetector {
public:
    ServiceDetector();
    ~ServiceDetector();

    void SetMaxThreads(int threads);
    void SetTimeout(int timeout_ms);

    std::vector<ServiceInfo> DetectServices(const std::string& ip,
                                             const std::vector<int>& ports);
    std::optional<ServiceInfo> GrabBanner(const std::string& ip, int port,
                                          const std::string& protocol = "tcp");

    ServiceDetector(const ServiceDetector&) = delete;
    ServiceDetector& operator=(const ServiceDetector&) = delete;

private:
    std::string IdentifyService(int port);
    std::string ParseVersion(const std::string& banner);

    int timeout_ms_{3000};
    int max_threads_{16};
    std::unique_ptr<core::ThreadPool> pool_;

    static const std::unordered_map<int, std::string> kKnownServices;
};

} // namespace scan
} // namespace netscope

#endif
