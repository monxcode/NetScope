#ifndef NETSCOPE_UTILS_STATISTICS_H
#define NETSCOPE_UTILS_STATISTICS_H

#include <string>
#include <vector>
#include <chrono>

#include "netscope/discovery/device.h"

namespace netscope {
namespace utils {

struct NetworkStatistics {
    int total_devices{0};
    int online_devices{0};
    int offline_devices{0};
    int total_open_ports{0};
    double scan_duration_seconds{0.0};
    double average_latency_ms{0.0};
    double min_latency_ms{0.0};
    double max_latency_ms{0.0};
    int unique_vendors{0};
    int unique_os_types{0};

    std::string ToString() const;
    std::string ToJSON() const;
};

class Statistics {
public:
    static NetworkStatistics Compute(const std::vector<discovery::Device>& devices,
                                      double scan_duration_seconds = 0.0);
    static void Print(const NetworkStatistics& stats);
    static std::string FormatDuration(double seconds);
};

} // namespace utils
} // namespace netscope

#endif
