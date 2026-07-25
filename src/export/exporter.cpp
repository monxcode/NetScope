#include "netscope/export/exporter.h"
#include "netscope/core/logger.h"

#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace netscope {
namespace export_ {

bool Exporter::Export(const std::vector<discovery::Device>& devices,
                      const fs::path& path,
                      ExportFormat format) {
    switch (format) {
        case ExportFormat::JSON: return ExportJSON(devices, path);
        case ExportFormat::CSV:  return ExportCSV(devices, path);
        case ExportFormat::TXT:  return ExportTXT(devices, path);
        case ExportFormat::DOT:  return ExportDOT(devices, path);
        default: return false;
    }
}

bool Exporter::ExportJSON(const std::vector<discovery::Device>& devices,
                          const fs::path& path) {
    try {
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        if (!file.is_open()) {
            core::Logger::Instance().Error("Export JSON: cannot open " + path.string());
            return false;
        }

        nlohmann::json j;
        j["application"] = "NetScope";
        j["version"] = "1.0.0";
        j["timestamp"] = std::to_string(
            std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

        auto now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm bt;
#ifdef _WIN32
        localtime_s(&bt, &now_c);
#else
        localtime_r(&now_c, &bt);
#endif
        std::ostringstream ts;
        ts << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");
        j["datetime"] = ts.str();

        j["device_count"] = devices.size();
        j["devices"] = nlohmann::json::array();
        for (const auto& d : devices) {
            j["devices"].push_back(d.ToJSON());
        }

        file << j.dump(2) << std::endl;
        core::Logger::Instance().Info("Exported JSON to " + path.string());
        return true;
    } catch (const std::exception& e) {
        core::Logger::Instance().Error("Export JSON failed: " + std::string(e.what()));
        return false;
    }
}

bool Exporter::ExportCSV(const std::vector<discovery::Device>& devices,
                         const fs::path& path) {
    try {
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        if (!file.is_open()) {
            core::Logger::Instance().Error("Export CSV: cannot open " + path.string());
            return false;
        }

        file << "IP,MAC,Hostname,Vendor,Status,OS,OS Confidence,TTL,Response Time (ms),"
             << "Open Ports,Last Seen\n";

        for (const auto& d : devices) {
            auto last_seen = std::chrono::duration_cast<std::chrono::seconds>(
                d.LastSeen().time_since_epoch()).count();

            std::string ports_str;
            for (const auto& p : d.OpenPorts()) {
                if (!ports_str.empty()) ports_str += "; ";
                ports_str += std::to_string(p.port) + "/" + p.service;
            }

            file << d.IP() << ","
                 << "\"" << d.MAC() << "\","
                 << "\"" << d.Hostname() << "\","
                 << "\"" << d.Vendor() << "\","
                 << (d.Online() ? "Online" : "Offline") << ","
                 << "\"" << d.OS().name << "\","
                 << std::fixed << std::setprecision(2) << d.OS().confidence << ","
                 << d.TTL() << ","
                 << d.ResponseTimeMs() << ","
                 << "\"" << ports_str << "\","
                 << last_seen
                 << "\n";
        }

        core::Logger::Instance().Info("Exported CSV to " + path.string());
        return true;
    } catch (const std::exception& e) {
        core::Logger::Instance().Error("Export CSV failed: " + std::string(e.what()));
        return false;
    }
}

bool Exporter::ExportTXT(const std::vector<discovery::Device>& devices,
                         const fs::path& path) {
    try {
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        if (!file.is_open()) {
            core::Logger::Instance().Error("Export TXT: cannot open " + path.string());
            return false;
        }

        file << "========================================\n";
        file << "  NetScope Network Scan Report\n";
        file << "========================================\n";

        auto now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm bt;
#ifdef _WIN32
        localtime_s(&bt, &now_c);
#else
        localtime_r(&now_c, &bt);
#endif
        file << "  Generated: " << std::put_time(&bt, "%Y-%m-%d %H:%M:%S") << "\n";
        file << "  Devices:   " << devices.size() << "\n";
        file << "========================================\n\n";

        for (size_t i = 0; i < devices.size(); ++i) {
            file << "Device #" << (i + 1) << "\n";
            file << "----------------------------------------\n";
            file << devices[i].ToString() << "\n";
            if (!devices[i].OpenPorts().empty()) {
                file << "  Open Ports:\n";
                for (const auto& p : devices[i].OpenPorts()) {
                    file << "    " << p.port << "/" << p.protocol
                         << " (" << p.service << ")\n";
                }
            }
            file << "\n";
        }

        core::Logger::Instance().Info("Exported TXT to " + path.string());
        return true;
    } catch (const std::exception& e) {
        core::Logger::Instance().Error("Export TXT failed: " + std::string(e.what()));
        return false;
    }
}

bool Exporter::ExportDOT(const std::vector<discovery::Device>& devices,
                         const fs::path& path) {
    try {
        fs::create_directories(path.parent_path());
        discovery::Topology topo;
        for (const auto& d : devices) {
            topo.AddDevice(d);
        }
        return topo.ExportDOT(path.string());
    } catch (const std::exception& e) {
        core::Logger::Instance().Error("Export DOT failed: " + std::string(e.what()));
        return false;
    }
}

bool Exporter::ExportTopology(const discovery::Topology& topology,
                              const fs::path& path,
                              ExportFormat format) {
    try {
        fs::create_directories(path.parent_path());
        switch (format) {
            case ExportFormat::DOT:
                return topology.ExportDOT(path.string());
            case ExportFormat::TXT: {
                std::ofstream file(path);
                if (!file.is_open()) return false;
                file << topology.GenerateASCII();
                return true;
            }
            case ExportFormat::JSON:
            case ExportFormat::CSV:
            default:
                return false;
        }
    } catch (...) {
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
