#include <gtest/gtest.h>
#include "netscope/core/config.h"

using namespace netscope::core;

TEST(ConfigTest, Singleton) {
    auto& c1 = Config::Instance();
    auto& c2 = Config::Instance();
    EXPECT_EQ(&c1, &c2);
}

TEST(ConfigTest, DefaultValues) {
    auto& cfg = Config::Instance();
    auto config = cfg.Get();
    EXPECT_EQ(config.network.subnet, "192.168.1.0/24");
    EXPECT_EQ(config.network.timeout_ms, 1000);
    EXPECT_EQ(config.network.max_threads, 64);
    EXPECT_EQ(config.ports.default_ports.size(), 15);
    EXPECT_EQ(config.ui.theme, "dark");
}

TEST(ConfigTest, ModifyAndGet) {
    auto& cfg = Config::Instance();
    cfg.Network().subnet = "10.0.0.0/8";
    EXPECT_EQ(cfg.Get().network.subnet, "10.0.0.0/8");
}

TEST(ConfigTest, SaveAndLoad) {
    auto& cfg = Config::Instance();
    cfg.Network().timeout_ms = 5000;
    EXPECT_TRUE(cfg.Save("test_config_temp.json"));
    cfg.Network().timeout_ms = 1000;
    cfg.Load("test_config_temp.json");
    EXPECT_EQ(cfg.Network().timeout_ms, 5000);
    fs::remove("test_config_temp.json");
}
