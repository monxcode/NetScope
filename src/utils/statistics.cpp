#include "netscope/utils/statistics.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <set>
#include <sstream>

namespace netscope {
namespace utils {

NetworkStatistics Statistics::Compute(const std::vector<discovery::Device>& devices,
                                       double scan_duration_seconds) {
    NetworkStatistics stats;
    stats.total_devices = static_cast<int>(devices.size());
    stats.scan_duration_seconds = scan_duration_seconds;

    if (devices.empty()) return stats;

    double total_latency = 0.0;
    int latency_count = 0;
    stats.min_latency_ms = std::numeric_limits<double>::max();
    stats.max_latency_ms = 0.0;

    std::set<std::string> vendors;
    std::set<std::string> os_types;

    for (const auto& d : devices) {
        if (d.Online()) {
            ++stats.online_devices;
            stats.total_open_ports += static_cast<int>(d.OpenPorts().size());

            if (d.ResponseTimeMs() > 0) {
                double rtt = static_cast<double>(d.ResponseTimeMs());
                total_latency += rtt;
                ++latency_count;
                stats.min_latency_ms = std::min(stats.min_latency_ms, rtt);
                stats.max_latency_ms = std::max(stats.max_latency_ms, rtt);
            }
        } else {
            ++stats.offline_devices;
        }

        if (!d.Vendor().empty()) vendors.insert(d.Vendor());
        if (!d.OS().name.empty()) os_types.insert(d.OS().name);
    }

    stats.average_latency_ms = (latency_count > 0)
        ? (total_latency / latency_count) : 0.0;
    if (stats.min_latency_ms == std::numeric_limits<double>::max()) {
        stats.min_latency_ms = 0.0;
    }

    stats.unique_vendors = static_cast<int>(vendors.size());
    stats.unique_os_types = static_cast<int>(os_types.size());

    return stats;
}

void Statistics::Print(const NetworkStatistics& stats) {
    std::cout << "\n  Network Statistics:\n";
    std::cout << "  " << std::string(40, '-') << "\n";
    std::cout << "  Total devices:    " << stats.total_devices << "\n";
    std::cout << "  Online devices:   " << stats.online_devices << "\n";
    std::cout << "  Offline devices:  " << stats.offline_devices << "\n";
    std::cout << "  Total open ports: " << stats.total_open_ports << "\n";

    if (stats.scan_duration_seconds > 0) {
        std::cout << "  Scan duration:    "
                  << FormatDuration(stats.scan_duration_seconds) << "\n";
    }

    if (stats.average_latency_ms > 0) {
        std::cout << "  Avg latency:      "
                  << std::fixed << std::setprecision(1)
                  << stats.average_latency_ms << " ms\n";
        std::cout << "  Min latency:      "
                  << stats.min_latency_ms << " ms\n";
        std::cout << "  Max latency:      "
                  << stats.max_latency_ms << " ms\n";
    }

    if (stats.unique_vendors > 0) {
        std::cout << "  Unique vendors:   " << stats.unique_vendors << "\n";
    }
    if (stats.unique_os_types > 0) {
        std::cout << "  OS types:         " << stats.unique_os_types << "\n";
    }
    std::cout << "\n";
}

std::string Statistics::FormatDuration(double seconds) {
    std::ostringstream oss;
    if (seconds < 1.0) {
        oss << std::fixed << std::setprecision(0)
            << (seconds * 1000) << " ms";
    } else if (seconds < 60.0) {
        oss << std::fixed << std::setprecision(1) << seconds << " s";
    } else {
        int mins = static_cast<int>(seconds) / 60;
        int secs = static_cast<int>(seconds) % 60;
        oss << mins << "m " << secs << "s";
    }
    return oss.str();
}

std::string NetworkStatistics::ToString() const {
    std::ostringstream oss;
    oss << "Total: " << total_devices
        << " | Online: " << online_devices
        << " | Offline: " << offline_devices
        << " | Ports: " << total_open_ports
        << " | Vendors: " << unique_vendors;
    if (scan_duration_seconds > 0) {
        oss << " | Duration: " << Statistics::FormatDuration(scan_duration_seconds);
    }
    if (average_latency_ms > 0) {
        oss << " | Avg RTT: " << std::fixed << std::setprecision(1)
            << average_latency_ms << "ms";
    }
    return oss.str();
}

std::string NetworkStatistics::ToJSON() const {
    std::ostringstream oss;
    oss << "{"
        << "\"total_devices\":" << total_devices << ","
        << "\"online_devices\":" << online_devices << ","
        << "\"offline_devices\":" << offline_devices << ","
        << "\"total_open_ports\":" << total_open_ports << ","
        << "\"scan_duration_seconds\":" << scan_duration_seconds << ","
        << "\"average_latency_ms\":" << average_latency_ms
        << "}";
    return oss.str();
}

} // namespace utils
} // namespace netscope
