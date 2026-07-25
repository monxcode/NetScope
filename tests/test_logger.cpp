#include <gtest/gtest.h>
#include "netscope/core/logger.h"

#include <filesystem>
#include <fstream>

using namespace netscope::core;

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& log = Logger::Instance();
        log.SetLevel(LogLevel::DEBUG);
        log.SetFileEnabled(false);
    }

    void TearDown() override {
        auto& log = Logger::Instance();
        log.SetLevel(LogLevel::INFO);
    }
};

TEST_F(LoggerTest, Singleton) {
    auto& log1 = Logger::Instance();
    auto& log2 = Logger::Instance();
    EXPECT_EQ(&log1, &log2);
}

TEST_F(LoggerTest, SetAndGetLevel) {
    auto& log = Logger::Instance();
    log.SetLevel(LogLevel::DEBUG);
    EXPECT_EQ(log.GetLevel(), LogLevel::DEBUG);
    log.SetLevel(LogLevel::ERROR);
    EXPECT_EQ(log.GetLevel(), LogLevel::ERROR);
    log.SetLevel(LogLevel::INFO);
    EXPECT_EQ(log.GetLevel(), LogLevel::INFO);
}

TEST_F(LoggerTest, LogDoesNotCrash) {
    auto& log = Logger::Instance();
    EXPECT_NO_THROW(log.Debug("Debug message"));
    EXPECT_NO_THROW(log.Info("Info message"));
    EXPECT_NO_THROW(log.Warn("Warning message"));
    EXPECT_NO_THROW(log.Error("Error message"));
    EXPECT_NO_THROW(log.Fatal("Fatal message"));
}

TEST_F(LoggerTest, FileLogging) {
    auto& log = Logger::Instance();
    auto test_path = std::filesystem::current_path() / "test_log.log";

    log.SetLogPath(test_path);
    log.SetFileEnabled(true);
    log.Info("Test log entry");

    log.SetFileEnabled(false);
    EXPECT_TRUE(std::filesystem::exists(test_path));

    std::ifstream file(test_path);
    ASSERT_TRUE(file.is_open());
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("Test log entry"), std::string::npos);
    file.close();

    std::filesystem::remove(test_path);
}
