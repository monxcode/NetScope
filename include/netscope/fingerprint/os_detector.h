#ifndef NETSCOPE_FINGERPRINT_OS_DETECTOR_H
#define NETSCOPE_FINGERPRINT_OS_DETECTOR_H

#include <string>
#include <vector>
#include <unordered_map>

#include "netscope/discovery/device.h"

namespace netscope {
namespace fingerprint {

struct OSFingerprint {
    std::string os_name;
    std::string os_family;
    std::string os_version;
    double confidence{0.0};
    std::vector<std::string> indicators;
};

class OSDetector {
public:
    OSDetector() = default;

    discovery::OSGuess DetectFromTTL(int ttl);
    discovery::OSGuess DetectFromPorts(const std::vector<int>& open_ports);
    discovery::OSGuess DetectFromTCPBehavior(int ttl,
                                              const std::vector<int>& open_ports,
                                              int response_time_ms);
    discovery::OSGuess Detect(const discovery::Device& device);

    static std::string GetOSFamily(const std::string& os_name);
    static std::string OSDescription(const discovery::OSGuess& guess);

private:
    struct TTLEntry {
        std::string name;
        double confidence;
        int min_ttl;
        int max_ttl;
    };

    static const std::vector<TTLEntry> kTTLTable;
    static const std::unordered_map<int, std::string> kPortOSMap;
};

} // namespace fingerprint
} // namespace netscope

#endif
