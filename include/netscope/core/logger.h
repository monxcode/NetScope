#ifndef NETSCOPE_CORE_LOGGER_H
#define NETSCOPE_CORE_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <filesystem>
#include <unordered_map>

namespace netscope {
namespace core {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger {
public:
    static Logger& Instance();

    void SetLevel(LogLevel level);
    LogLevel GetLevel() const;

    void SetFileEnabled(bool enabled);
    void SetLogPath(const std::filesystem::path& path);
    void SetMaxSizeMB(size_t mb);

    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warn(const std::string& message);
    void Error(const std::string& message);
    void Fatal(const std::string& message);

    void Log(LogLevel level, const std::string& message);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

private:
    Logger();
    ~Logger();

    void WriteFile(const std::string& formatted);
    std::string FormatMessage(LogLevel level, const std::string& message);
    std::string LevelToString(LogLevel level);
    void RotateIfNeeded();

    LogLevel level_{LogLevel::INFO};
    bool file_enabled_{true};
    std::filesystem::path log_path_;
    size_t max_size_mb_{10};
    std::ofstream file_stream_;
    mutable std::mutex mutex_;
};

class ScopedLogger {
public:
    explicit ScopedLogger(const std::string& scope_name);
    ~ScopedLogger();
    ScopedLogger(const ScopedLogger&) = delete;
    ScopedLogger& operator=(const ScopedLogger&) = delete;

private:
    std::string name_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace core
} // namespace netscope

#endif
