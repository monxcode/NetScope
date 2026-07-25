#include "netscope/export/formatter.h"

#include <sstream>
#include <iomanip>

namespace netscope {
namespace export_ {

std::string Formatter::FormatDevice(const discovery::Device& device) {
    std::ostringstream oss;

    oss << "┌─ " << device.IP();
    if (!device.Hostname().empty()) {
        oss << " (" << device.Hostname() << ")";
    }
    oss << "\n";

    oss << KeyValue("Status", device.Online() ? "Online" : "Offline") << "\n";
    oss << KeyValue("MAC", device.MAC().empty() ? "N/A" : device.MAC()) << "\n";
    oss << KeyValue("Vendor", device.Vendor().empty() ? "N/A" : device.Vendor()) << "\n";

    if (device.Online()) {
        oss << KeyValue("OS", device.OS().name + " (" +
                               std::to_string(static_cast<int>(device.OS().confidence * 100)) +
                               "%)") << "\n";
        oss << KeyValue("TTL", std::to_string(device.TTL())) << "\n";
        oss << KeyValue("Response", std::to_string(device.ResponseTimeMs()) + "ms") << "\n";
    }

    if (!device.OpenPorts().empty()) {
        oss << KeyValue("Open Ports", std::to_string(device.OpenPorts().size())) << "\n";
        for (const auto& port : device.OpenPorts()) {
            oss << "  " << FormatPortResult(port) << "\n";
        }
    }

    oss << "└─\n";

    return oss.str();
}

std::string Formatter::FormatDeviceShort(const discovery::Device& device) {
    std::ostringstream oss;
    oss << device.IP();
    if (!device.Hostname().empty()) {
        oss << " (" << device.Hostname() << ")";
    }
    oss << " - "
        << (device.Online() ? "Online" : "Offline")
        << " - " << device.Vendor();
    return oss.str();
}

std::string Formatter::FormatPortResult(const scan::PortResult& port) {
    std::ostringstream oss;
    oss << "Port " << port.port << "/" << port.protocol
        << " (" << port.service << ")";
    if (!port.banner.empty()) {
        oss << " - " << port.banner.substr(0, 80);
    }
    return oss.str();
}

std::string Formatter::FormatDuration(std::chrono::seconds secs) {
    auto h = std::chrono::duration_cast<std::chrono::hours>(secs);
    secs -= h;
    auto m = std::chrono::duration_cast<std::chrono::minutes>(secs);
    secs -= m;
    auto s = secs;

    std::ostringstream oss;
    if (h.count() > 0) oss << h.count() << "h ";
    if (m.count() > 0) oss << m.count() << "m ";
    oss << s.count() << "s";
    return oss.str();
}

std::string Formatter::Indent(const std::string& text, int level) {
    std::string prefix(level * 2, ' ');
    std::istringstream stream(text);
    std::string line;
    std::ostringstream result;

    bool first = true;
    while (std::getline(stream, line)) {
        if (!first) result << "\n";
        result << prefix << line;
        first = false;
    }

    return result.str();
}

std::string Formatter::KeyValue(const std::string& key, const std::string& value,
                                 int key_width) {
    std::ostringstream oss;
    oss << "  " << std::left << std::setw(key_width) << key << ": " << value;
    return oss.str();
}

} // namespace export_
} // namespace netscope
