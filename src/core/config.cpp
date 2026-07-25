#include "netscope/core/config.h"

#include <fstream>
#include <iostream>

namespace netscope {
namespace core {

Config& Config::Instance() {
    static Config instance;
    return instance;
}

Config::Config() {
    config_path_ = fs::current_path() / "config" / "config.json";
    ApplyDefaults();
    Load();
}

bool Config::Load(const fs::path& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto load_path = path.empty() ? config_path_ : path;
    if (!path.empty()) config_path_ = path;

    if (!fs::exists(load_path)) {
        ApplyDefaults();
        return false;
    }

    try {
        std::ifstream file(load_path);
        if (!file.is_open()) {
            ApplyDefaults();
            return false;
        }
        nlohmann::json j;
        file >> j;
        FromJson(j);
        return true;
    } catch (...) {
        ApplyDefaults();
        return false;
    }
}

bool Config::Save(const fs::path& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto save_path = path.empty() ? config_path_ : path;
    try {
        fs::create_directories(save_path.parent_path());
        std::ofstream file(save_path);
        if (!file.is_open()) return false;
        file << ToJson().dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

AppConfig Config::Get() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void Config::Set(const AppConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

NetworkConfig& Config::Network() { return config_.network; }
PortConfig& Config::Ports() { return config_.ports; }
MonitorConfig& Config::Monitor() { return config_.monitor; }
ExportConfig& Config::Export() { return config_.export_; }
LoggingConfig& Config::Logging() { return config_.logging; }
UIConfig& Config::UI() { return config_.ui; }

fs::path Config::GetConfigPath() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_path_;
}

void Config::SetConfigPath(const fs::path& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_path_ = path;
}

void Config::ApplyDefaults() {
    config_ = AppConfig{};
}

void Config::FromJson(const nlohmann::json& j) {
    if (j.contains("network")) {
        const auto& n = j["network"];
        config_.network.subnet = n.value("subnet", config_.network.subnet);
        config_.network.timeout_ms = n.value("timeout_ms", config_.network.timeout_ms);
        config_.network.retries = n.value("retries", config_.network.retries);
        config_.network.max_threads = n.value("max_threads", config_.network.max_threads);
        config_.network.ping_count = n.value("ping_count", config_.network.ping_count);
    }
    if (j.contains("ports")) {
        const auto& p = j["ports"];
        if (p.contains("default"))
            config_.ports.default_ports = p["default"].get<std::vector<int>>();
        if (p.contains("custom") && !p["custom"].is_null())
            config_.ports.custom_ports = p["custom"].get<std::vector<int>>();
        if (p.contains("range_start") && !p["range_start"].is_null())
            config_.ports.range_start = p["range_start"].get<int>();
        if (p.contains("range_end") && !p["range_end"].is_null())
            config_.ports.range_end = p["range_end"].get<int>();
    }
    if (j.contains("monitor")) {
        config_.monitor.interval_seconds = j["monitor"].value("interval_seconds", 30);
        config_.monitor.enabled = j["monitor"].value("enabled", false);
    }
    if (j.contains("export")) {
        config_.export_.default_format = j["export"].value("default_format", "json");
        config_.export_.auto_export = j["export"].value("auto_export", false);
    }
    if (j.contains("logging")) {
        config_.logging.level = j["logging"].value("level", "info");
        config_.logging.file_enabled = j["logging"].value("file_enabled", true);
        config_.logging.max_size_mb = j["logging"].value("max_size_mb", 10);
    }
    if (j.contains("ui")) {
        config_.ui.theme = j["ui"].value("theme", "dark");
        config_.ui.refresh_rate_ms = j["ui"].value("refresh_rate_ms", 500);
    }
}

nlohmann::json Config::ToJson() const {
    nlohmann::json j;
    j["network"] = {
        {"subnet", config_.network.subnet},
        {"timeout_ms", config_.network.timeout_ms},
        {"retries", config_.network.retries},
        {"max_threads", config_.network.max_threads},
        {"ping_count", config_.network.ping_count}
    };
    j["ports"] = {
        {"default", config_.ports.default_ports},
        {"custom", config_.ports.custom_ports},
        {"range_start", config_.ports.range_start.has_value()
                            ? nlohmann::json(config_.ports.range_start.value())
                            : nlohmann::json(nullptr)},
        {"range_end", config_.ports.range_end.has_value()
                          ? nlohmann::json(config_.ports.range_end.value())
                          : nlohmann::json(nullptr)}
    };
    j["monitor"] = {{"interval_seconds", config_.monitor.interval_seconds},
                    {"enabled", config_.monitor.enabled}};
    j["export"] = {{"default_format", config_.export_.default_format},
                   {"auto_export", config_.export_.auto_export}};
    j["logging"] = {{"level", config_.logging.level},
                    {"file_enabled", config_.logging.file_enabled},
                    {"max_size_mb", config_.logging.max_size_mb}};
    j["ui"] = {{"theme", config_.ui.theme},
               {"refresh_rate_ms", config_.ui.refresh_rate_ms}};
    return j;
}

} // namespace core
} // namespace netscope
