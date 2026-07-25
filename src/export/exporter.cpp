#include "netscope/export/exporter.h"
#include "netscope/core/logger.h"

#include <fstream>
#include <algorithm>

namespace netscope {
namespace export_ {

bool Exporter::Export(const std::vector<discovery::Device>& devices,
                      const std::filesystem::path& path,
                      ExportFormat format) {
    switch (format) {
        case ExportFormat::JSON:
            return ExportJSON(devices, path);
        case ExportFormat::CSV:
            return ExportCSV(devices, path);
        case ExportFormat::TXT:
            return ExportTXT(devices, path);
        default:
            core::Logger::Instance().Error("Unsupported export format");
            return false;
    }
}

bool Exporter::ExportJSON(const std::vector<discovery::Device>& devices,
                          const std::filesystem::path& path) {
    try {
        std::filesystem::create_directories(path.parent_path());

        nlohmann::json j;
        j["netscope_version"] = "1.0.0";
        j["export_time"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        j["device_count"] = devices.size();

        j["devices"] = nlohmann::json::array();
        for (const auto& device : devices) {
            j["devices"].push_back(device.ToJSON());
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            core::Logger::Instance().Error("Failed to open file for JSON export: " + path.string());
            return false;
        }

        file << j.dump(2);
        core::Logger::Instance().Info("Exported " + std::to_string(devices.size()) +
                                       " devices to JSON: " + path.string());
        return true;
    } catch (const std::exception& e) {
        core::Logger::Instance().Error("JSON export failed: " + std::string(e.what()));
        return false;
    }
}

bool Exporter::ExportCSV(const std::vector<discovery::Device>& devices,
                         const std::filesystem::path& path) {
    try {
        std::filesystem::create_directories(path.parent_path());

        std::ofstream file(path);
        if (!file.is_open()) {
            core::Logger::Instance().Error("Failed to open file for CSV export: " + path.string());
            return false;
        }

        file << "IP,MAC,Hostname,Vendor,Status,OS,TTL,ResponseTimeMs\n";
        for (const auto& device : devices) {
            file << device.ToCSV() << "\n";
        }

        core::Logger::Instance().Info("Exported " + std::to_string(devices.size()) +
                                       " devices to CSV: " + path.string());
        return true;
    } catch (const std::exception& e) {
        core::Logger::Instance().Error("CSV export failed: " + std::string(e.what()));
        return false;
    }
}

bool Exporter::ExportTXT(const std::vector<discovery::Device>& devices,
                         const std::filesystem::path& path) {
    try {
        std::filesystem::create_directories(path.parent_path());

        std::ofstream file(path);
        if (!file.is_open()) {
            core::Logger::Instance().Error("Failed to open file for TXT export: " + path.string());
            return false;
        }

        file << "NetScope Scan Results\n";
        file << "====================\n\n";
        file << "Devices found: " << devices.size() << "\n\n";

        for (size_t i = 0; i < devices.size(); ++i) {
            file << "Device " << (i + 1) << ":\n";
            file << devices[i].ToString() << "\n\n";
        }

        core::Logger::Instance().Info("Exported " + std::to_string(devices.size()) +
                                       " devices to TXT: " + path.string());
        return true;
    } catch (const std::exception& e) {
        core::Logger::Instance().Error("TXT export failed: " + std::string(e.what()));
        return false;
    }
}

bool Exporter::ExportTopology(const discovery::Topology& topology,
                              const std::filesystem::path& path,
                              ExportFormat format) {
    if (format == ExportFormat::DOT) {
        return topology.ExportDOT(path.string());
    }

    try {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path);
        if (!file.is_open()) {
            core::Logger::Instance().Error("Failed to open topology export: " + path.string());
            return false;
        }

        if (format == ExportFormat::TXT) {
            file << topology.GenerateASCII();
        }

        core::Logger::Instance().Info("Topology exported: " + path.string());
        return true;
    } catch (const std::exception& e) {
        core::Logger::Instance().Error("Topology export failed: " + std::string(e.what()));
        return false;
    }
}

std::string Exporter::FormatToString(ExportFormat format) {
    switch (format) {
        case ExportFormat::JSON: return "json";
        case ExportFormat::CSV:  return "csv";
        case ExportFormat::TXT:  return "txt";
        case ExportFormat::DOT:  return "dot";
        default: return "unknown";
    }
}

ExportFormat Exporter::StringToFormat(const std::string& str) {
    auto lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "json") return ExportFormat::JSON;
    if (lower == "csv")  return ExportFormat::CSV;
    if (lower == "txt")  return ExportFormat::TXT;
    if (lower == "dot")  return ExportFormat::DOT;
    return ExportFormat::JSON;
}

} // namespace export_
} // namespace netscope
