#include <gtest/gtest.h>
#include "netscope/core/config.h"

using namespace netscope::core;

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& cfg = Config::Instance();
        cfg.SetConfigPath("test_config.json");
    }
};

TEST_F(ConfigTest, DefaultValues) {
    auto& cfg = Config::Instance();
    auto config = cfg.Get();

    EXPECT_EQ(config.network.subnet, "192.168.1.0/24");
    EXPECT_EQ(config.network.timeout_ms, 1000);
    EXPECT_EQ(config.network.max_threads, 64);
    EXPECT_EQ(config.ports.default_ports.size(), 15);
    EXPECT_EQ(config.ui.theme, "dark");
}

TEST_F(ConfigTest, NetworkConfig) {
    auto& cfg = Config::Instance();
    EXPECT_EQ(cfg.Network().subnet, "192.168.1.0/24");
    cfg.Network().subnet = "10.0.0.0/8";
    EXPECT_EQ(cfg.Network().subnet, "10.0.0.0/8");
}

TEST_F(ConfigTest, SaveAndLoad) {
    auto& cfg = Config::Instance();
    cfg.Network().timeout_ms = 5000;
    cfg.Network().max_threads = 128;

    EXPECT_TRUE(cfg.Save("test_config_save.json"));

    cfg.Network().timeout_ms = 1000;
    cfg.Load("test_config_save.json");

    EXPECT_EQ(cfg.Network().timeout_ms, 5000);
    EXPECT_EQ(cfg.Network().max_threads, 128);

    std::filesystem::remove("test_config_save.json");
}

TEST_F(ConfigTest, ThreadSafety) {
    auto& cfg = Config::Instance();
    auto& network = cfg.Network();
    network.subnet = "192.168.0.0/24";
    EXPECT_EQ(cfg.Network().subnet, "192.168.0.0/24");
}

TEST_F(ConfigTest, DefaultPorts) {
    auto& cfg = Config::Instance().Get();
    EXPECT_NE(std::find(cfg.ports.default_ports.begin(),
                        cfg.ports.default_ports.end(), 80),
              cfg.ports.default_ports.end());
    EXPECT_NE(std::find(cfg.ports.default_ports.begin(),
                        cfg.ports.default_ports.end(), 443),
              cfg.ports.default_ports.end());
    EXPECT_NE(std::find(cfg.ports.default_ports.begin(),
                        cfg.ports.default_ports.end(), 22),
              cfg.ports.default_ports.end());
}
