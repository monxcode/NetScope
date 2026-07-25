#ifndef NETSCOPE_EXPORT_EXPORTER_H
#define NETSCOPE_EXPORT_EXPORTER_H

#include <string>
#include <vector>
#include <filesystem>

#include "netscope/discovery/device.h"
#include "netscope/discovery/topology.h"

namespace netscope {
namespace export_ {

enum class ExportFormat {
    JSON,
    CSV,
    TXT,
    DOT
};

class Exporter {
public:
    Exporter() = default;

    bool Export(const std::vector<discovery::Device>& devices,
                const std::filesystem::path& path,
                ExportFormat format);

    bool ExportJSON(const std::vector<discovery::Device>& devices,
                    const std::filesystem::path& path);
    bool ExportCSV(const std::vector<discovery::Device>& devices,
                   const std::filesystem::path& path);
    bool ExportTXT(const std::vector<discovery::Device>& devices,
                   const std::filesystem::path& path);

    bool ExportTopology(const discovery::Topology& topology,
                        const std::filesystem::path& path,
                        ExportFormat format);

    static std::string FormatToString(ExportFormat format);
    static ExportFormat StringToFormat(const std::string& str);

    Exporter(const Exporter&) = delete;
    Exporter& operator=(const Exporter&) = delete;
};

} // namespace export_
} // namespace netscope

#endif
