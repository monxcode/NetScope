#ifndef NETSCOPE_EXPORT_FORMATTER_H
#define NETSCOPE_EXPORT_FORMATTER_H

#include <string>
#include <sstream>

#include "netscope/discovery/device.h"
#include "netscope/scan/port_scanner.h"

namespace netscope {
namespace export_ {

class Formatter {
public:
    static std::string FormatDevice(const discovery::Device& device);
    static std::string FormatDeviceShort(const discovery::Device& device);
    static std::string FormatPortResult(const scan::PortResult& port);
    static std::string FormatDuration(std::chrono::seconds secs);

    static std::string Indent(const std::string& text, int level);
    static std::string KeyValue(const std::string& key, const std::string& value,
                                int key_width = 20);

    Formatter() = delete;
};

} // namespace export_
} // namespace netscope

#endif
