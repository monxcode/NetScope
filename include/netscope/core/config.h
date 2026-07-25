#ifndef NETSCOPE_CORE_CONFIG_H
#define NETSCOPE_CORE_CONFIG_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "netscope/core/filesystem.h"
#include <optional>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace netscope {
namespace core {

struct NetworkConfig {
    std::string subnet = "192.168.1.0/24";
    int timeout_ms = 1000;
    int retries = 2;
    int max_threads = 64;
    int ping_count = 2;
};

struct PortConfig {
    std::vector<int> default_ports = {21, 22, 23, 25, 53, 80, 110, 135,
                                      139, 143, 443, 445, 3306, 3389, 8080};
    std::vector<int> custom_ports;
    std::optional<int> range_start;
    std::optional<int> range_end;
};

struct MonitorConfig {
    int interval_seconds = 30;
    bool enabled = false;
};

struct ExportConfig {
    std::string default_format = "json";
    bool auto_export = false;
};

struct LoggingConfig {
    std::string level = "info";
    bool file_enabled = true;
    size_t max_size_mb = 10;
};

struct UIConfig {
    std::string theme = "dark";
    int refresh_rate_ms = 500;
};

struct AppConfig {
    NetworkConfig network;
    PortConfig ports;
    MonitorConfig monitor;
    ExportConfig export_;
    LoggingConfig logging;
    UIConfig ui;
};

class Config {
public:
    static Config& Instance();

    bool Load(const fs::path& path = "");
    bool Save(const fs::path& path = "");

    AppConfig Get() const;
    void Set(const AppConfig& config);

    NetworkConfig& Network();
    PortConfig& Ports();
    MonitorConfig& Monitor();
    ExportConfig& Export();
    LoggingConfig& Logging();
    UIConfig& UI();

    fs::path GetConfigPath() const;
    void SetConfigPath(const fs::path& path);

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;

private:
    Config();
    ~Config() = default;

    void ApplyDefaults();
    void FromJson(const nlohmann::json& j);
    nlohmann::json ToJson() const;

    AppConfig config_;
    fs::path config_path_;
    mutable std::mutex mutex_;
};

} // namespace core
} // namespace netscope

#endif
