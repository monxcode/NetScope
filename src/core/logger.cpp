#include "netscope/core/logger.h"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

namespace netscope {
namespace core {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    log_path_ = std::filesystem::current_path() / "logs" / "netscope.log";
    std::filesystem::create_directories(log_path_.parent_path());
    if (file_enabled_) {
        file_stream_.open(log_path_, std::ios::app);
    }
}

Logger::~Logger() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::SetLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::GetLevel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

void Logger::SetFileEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_enabled_ = enabled;
    if (enabled && !file_stream_.is_open()) {
        file_stream_.open(log_path_, std::ios::app);
    } else if (!enabled && file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::SetLogPath(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_path_ = path;
    std::filesystem::create_directories(log_path_.parent_path());
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    if (file_enabled_) {
        file_stream_.open(log_path_, std::ios::app);
    }
}

void Logger::SetMaxSizeMB(size_t mb) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_size_mb_ = mb;
}

void Logger::Debug(const std::string& message) {
    Log(LogLevel::DEBUG, message);
}

void Logger::Info(const std::string& message) {
    Log(LogLevel::INFO, message);
}

void Logger::Warn(const std::string& message) {
    Log(LogLevel::WARN, message);
}

void Logger::Error(const std::string& message) {
    Log(LogLevel::ERROR, message);
}

void Logger::Fatal(const std::string& message) {
    Log(LogLevel::FATAL, message);
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (level < level_) {
        return;
    }

    auto formatted = FormatMessage(level, message);

    std::cout << formatted << std::endl;

    if (file_enabled_) {
        WriteFile(formatted);
    }
}

void Logger::WriteFile(const std::string& formatted) {
    if (!file_stream_.is_open()) {
        return;
    }
    RotateIfNeeded();
    file_stream_ << formatted << std::endl;
    file_stream_.flush();
}

std::string Logger::FormatMessage(LogLevel level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    std::tm bt;
#ifdef _WIN32
    localtime_s(&bt, &in_time_t);
#else
    localtime_r(&in_time_t, &bt);
#endif

    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count()
        << " [" << LevelToString(level) << "] " << message;

    return oss.str();
}

std::string Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "UNKNOWN";
    }
}

void Logger::RotateIfNeeded() {
    if (!std::filesystem::exists(log_path_)) {
        return;
    }

    auto size = std::filesystem::file_size(log_path_);
    if (size < max_size_mb_ * 1024 * 1024) {
        return;
    }

    file_stream_.close();

    auto backup = log_path_;
    backup += ".old";
    if (std::filesystem::exists(backup)) {
        std::filesystem::remove(backup);
    }
    std::filesystem::rename(log_path_, backup);

    file_stream_.open(log_path_, std::ios::app);
}

ScopedLogger::ScopedLogger(const std::string& scope_name)
    : name_(scope_name), start_(std::chrono::steady_clock::now()) {
    Logger::Instance().Debug("[ENTER] " + name_);
}

ScopedLogger::~ScopedLogger() {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - start_).count();
    Logger::Instance().Debug("[EXIT] " + name_ + " (" + std::to_string(elapsed) + "ms)");
}

} // namespace core
} // namespace netscope
