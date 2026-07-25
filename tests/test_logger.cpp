#include <gtest/gtest.h>
#include "netscope/core/logger.h"

#include <filesystem>
#include <fstream>

using namespace netscope::core;

TEST(LoggerTest, Singleton) {
    auto& l1 = Logger::Instance();
    auto& l2 = Logger::Instance();
    EXPECT_EQ(&l1, &l2);
}

TEST(LoggerTest, SetAndGetLevel) {
    auto& log = Logger::Instance();
    log.SetLevel(LogLevel::DEBUG);
    EXPECT_EQ(log.GetLevel(), LogLevel::DEBUG);
    log.SetLevel(LogLevel::ERROR);
    EXPECT_EQ(log.GetLevel(), LogLevel::ERROR);
    log.SetLevel(LogLevel::INFO);
}

TEST(LoggerTest, LogDoesNotCrash) {
    auto& log = Logger::Instance();
    EXPECT_NO_THROW(log.Debug("test debug"));
    EXPECT_NO_THROW(log.Info("test info"));
    EXPECT_NO_THROW(log.Warn("test warn"));
    EXPECT_NO_THROW(log.Error("test error"));
}

TEST(LoggerTest, FileLogging) {
    auto& log = Logger::Instance();
    auto test_path = fs::current_path() / "test_log.log";
    log.SetFileEnabled(false);
    log.SetLogPath(test_path);
    log.SetFileEnabled(true);
    log.Info("test file logging");
    log.SetFileEnabled(false);

    EXPECT_TRUE(fs::exists(test_path));
    std::ifstream file(test_path);
    ASSERT_TRUE(file.is_open());
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("test file logging"), std::string::npos);
    file.close();
    fs::remove(test_path);
}
