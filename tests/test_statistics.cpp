#include <gtest/gtest.h>
#include "netscope/utils/statistics.h"
#include "netscope/discovery/device.h"

using namespace netscope::utils;
using namespace netscope::discovery;

TEST(StatisticsTest, EmptyDevices) {
    auto stats = Statistics::Compute({});
    EXPECT_EQ(stats.total_devices, 0);
    EXPECT_EQ(stats.online_devices, 0);
    EXPECT_EQ(stats.offline_devices, 0);
}

TEST(StatisticsTest, SingleDevice) {
    std::vector<Device> devices;
    Device d;
    d.SetIP("192.168.1.1");
    d.SetOnline(true);
    d.SetResponseTimeMs(10);
    d.SetTTL(64);
    d.SetMAC("00:11:22:33:44:55");
    devices.push_back(d);

    auto stats = Statistics::Compute(devices, 5.0);
    EXPECT_EQ(stats.total_devices, 1);
    EXPECT_EQ(stats.online_devices, 1);
    EXPECT_EQ(stats.offline_devices, 0);
    EXPECT_EQ(stats.average_latency_ms, 10.0);
    EXPECT_EQ(stats.scan_duration_seconds, 5.0);
}

TEST(StatisticsTest, MixedDevices) {
    std::vector<Device> devices;
    for (int i = 0; i < 5; ++i) {
        Device d;
        d.SetIP("192.168.1." + std::to_string(i + 1));
        d.SetOnline(i < 3);
        d.SetResponseTimeMs((i + 1) * 5);
        devices.push_back(d);
    }

    auto stats = Statistics::Compute(devices, 10.0);
    EXPECT_EQ(stats.total_devices, 5);
    EXPECT_EQ(stats.online_devices, 3);
    EXPECT_EQ(stats.offline_devices, 2);
    EXPECT_GT(stats.average_latency_ms, 0);
}

TEST(StatisticsTest, FormatDuration) {
    std::string s = Statistics::FormatDuration(0.5);
    EXPECT_FALSE(s.empty());

    s = Statistics::FormatDuration(5.0);
    EXPECT_FALSE(s.empty());

    s = Statistics::FormatDuration(125.0);
    EXPECT_FALSE(s.empty());
}

TEST(StatisticsTest, ToString) {
    NetworkStatistics stats;
    stats.total_devices = 10;
    stats.online_devices = 8;
    stats.offline_devices = 2;
    std::string s = stats.ToString();
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("10"), std::string::npos);
    EXPECT_NE(s.find("8"), std::string::npos);
}
