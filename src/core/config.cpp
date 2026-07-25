#include "netscope/core/config.h"

#include <fstream>
#include <iostream>
#include <filesystem>

namespace netscope {
namespace core {

Config& Config::Instance() {
    static Config instance;
    return instance;
}

Config::Config() {
    config_path_ = std::filesystem::current_path() / "config" / "config.json";
    ApplyDefaults();
    Load();
}

bool Config::Load(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto load_path = path.empty() ? config_path_ : path;
    if (!path.empty()) {
        config_path_ = path;
    }

    if (!std::filesystem::exists(load_path)) {
        Logger::Instance().Warn("Config file not found, using defaults: " + load_path.string());
        ApplyDefaults();
        return false;
    }

    try {
        std::ifstream file(load_path);
        if (!file.is_open()) {
            Logger::Instance().Error("Failed to open config file: " + load_path.string());
            ApplyDefaults();
            return false;
        }

        nlohmann::json j;
        file >> j;
        FromJson(j);
        Logger::Instance().Info("Configuration loaded from: " + load_path.string());
        return true;
    } catch (const std::exception& e) {
        Logger::Instance().Error("Failed to parse config file: " + std::string(e.what()));
        ApplyDefaults();
        return false;
    }
}

bool Config::Save(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto save_path = path.empty() ? config_path_ : path;

    try {
        std::filesystem::create_directories(save_path.parent_path());
        std::ofstream file(save_path);
        if (!file.is_open()) {
            Logger::Instance().Error("Failed to write config file: " + save_path.string());
            return false;
        }

        auto j = ToJson();
        file << j.dump(4);
        Logger::Instance().Info("Configuration saved to: " + save_path.string());
        return true;
    } catch (const std::exception& e) {
        Logger::Instance().Error("Failed to save config: " + std::string(e.what()));
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

NetworkConfig& Config::Network() {
    return config_.network;
}

PortConfig& Config::Ports() {
    return config_.ports;
}

MonitorConfig& Config::Monitor() {
    return config_.monitor;
}

ExportConfig& Config::Export() {
    return config_.export_;
}

LoggingConfig& Config::Logging() {
    return config_.logging;
}

UIConfig& Config::UI() {
    return config_.ui;
}

std::filesystem::path Config::GetConfigPath() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_path_;
}

void Config::SetConfigPath(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_path_ = path;
}

void Config::ApplyDefaults() {
    config_ = AppConfig{};
}

void Config::FromJson(const nlohmann::json& j) {
    auto& network = config_.network;
    if (j.contains("network")) {
        const auto& n = j["network"];
        network.subnet = n.value("subnet", network.subnet);
        network.timeout_ms = n.value("timeout_ms", network.timeout_ms);
        network.retries = n.value("retries", network.retries);
        network.max_threads = n.value("max_threads", network.max_threads);
        network.ping_count = n.value("ping_count", network.ping_count);
    }

    auto& ports = config_.ports;
    if (j.contains("ports")) {
        const auto& p = j["ports"];
        if (p.contains("default")) {
            ports.default_ports = p["default"].get<std::vector<int>>();
        }
        if (p.contains("custom") && !p["custom"].is_null()) {
            ports.custom_ports = p["custom"].get<std::vector<int>>();
        }
        if (p.contains("range_start") && !p["range_start"].is_null()) {
            ports.range_start = p["range_start"].get<int>();
        }
        if (p.contains("range_end") && !p["range_end"].is_null()) {
            ports.range_end = p["range_end"].get<int>();
        }
    }

    auto& monitor = config_.monitor;
    if (j.contains("monitor")) {
        const auto& m = j["monitor"];
        monitor.interval_seconds = m.value("interval_seconds", monitor.interval_seconds);
        monitor.enabled = m.value("enabled", monitor.enabled);
    }

    auto& export_cfg = config_.export_;
    if (j.contains("export")) {
        const auto& e = j["export"];
        export_cfg.default_format = e.value("default_format", export_cfg.default_format);
        export_cfg.auto_export = e.value("auto_export", export_cfg.auto_export);
    }

    auto& logging = config_.logging;
    if (j.contains("logging")) {
        const auto& l = j["logging"];
        logging.level = l.value("level", logging.level);
        logging.file_enabled = l.value("file_enabled", logging.file_enabled);
        logging.max_size_mb = l.value("max_size_mb", logging.max_size_mb);
    }

    auto& ui = config_.ui;
    if (j.contains("ui")) {
        const auto& u = j["ui"];
        ui.theme = u.value("theme", ui.theme);
        ui.refresh_rate_ms = u.value("refresh_rate_ms", ui.refresh_rate_ms);
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

    j["monitor"] = {
        {"interval_seconds", config_.monitor.interval_seconds},
        {"enabled", config_.monitor.enabled}
    };

    j["export"] = {
        {"default_format", config_.export_.default_format},
        {"auto_export", config_.export_.auto_export}
    };

    j["logging"] = {
        {"level", config_.logging.level},
        {"file_enabled", config_.logging.file_enabled},
        {"max_size_mb", config_.logging.max_size_mb}
    };

    j["ui"] = {
        {"theme", config_.ui.theme},
        {"refresh_rate_ms", config_.ui.refresh_rate_ms}
    };

    return j;
}

} // namespace core
} // namespace netscope
