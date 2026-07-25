#include <gtest/gtest.h>
#include "netscope/discovery/device.h"

using namespace netscope::discovery;

TEST(DeviceTest, DefaultState) {
    Device d;
    EXPECT_FALSE(d.Online());
    EXPECT_TRUE(d.IP().empty());
    EXPECT_TRUE(d.MAC().empty());
    EXPECT_TRUE(d.Hostname().empty());
    EXPECT_EQ(d.ResponseTimeMs(), 0);
    EXPECT_EQ(d.TTL(), 0);
    EXPECT_TRUE(d.OpenPorts().empty());
}

TEST(DeviceTest, SetProperties) {
    Device d;
    d.SetIP("192.168.1.10");
    d.SetMAC("00:11:22:33:44:55");
    d.SetHostname("test-pc.local");
    d.SetVendor("Intel");
    d.SetOnline(true);
    d.SetResponseTimeMs(5);
    d.SetTTL(128);

    EXPECT_EQ(d.IP(), "192.168.1.10");
    EXPECT_EQ(d.MAC(), "00:11:22:33:44:55");
    EXPECT_EQ(d.Hostname(), "test-pc.local");
    EXPECT_EQ(d.Vendor(), "Intel");
    EXPECT_TRUE(d.Online());
    EXPECT_EQ(d.ResponseTimeMs(), 5);
    EXPECT_EQ(d.TTL(), 128);
}

TEST(DeviceTest, OSGuess) {
    Device d;
    OSGuess os;
    os.name = "Windows";
    os.confidence = 0.85;
    os.ttl = 128;
    d.SetOS(os);

    EXPECT_EQ(d.OS().name, "Windows");
    EXPECT_DOUBLE_EQ(d.OS().confidence, 0.85);
    EXPECT_EQ(d.OS().ttl, 128);
}

TEST(DeviceTest, OpenPorts) {
    Device d;
    netscope::scan::PortResult port;
    port.port = 80;
    port.open = true;
    port.service = "HTTP";
    d.AddOpenPort(port);

    EXPECT_EQ(d.OpenPorts().size(), 1);
    EXPECT_EQ(d.OpenPorts()[0].port, 80);
    EXPECT_TRUE(d.OpenPorts()[0].open);
    EXPECT_EQ(d.OpenPorts()[0].service, "HTTP");
}

TEST(DeviceTest, Comparison) {
    Device a, b;
    a.SetIP("192.168.1.1");
    b.SetIP("192.168.1.2");

    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a == b);

    b.SetIP("192.168.1.1");
    EXPECT_TRUE(a == b);
}

TEST(DeviceTest, LastSeen) {
    Device d;
    auto before = std::chrono::steady_clock::now();
    d.UpdateLastSeen();
    auto after = std::chrono::steady_clock::now();

    EXPECT_GE(d.LastSeen(), before);
    EXPECT_LE(d.LastSeen(), after);
}

TEST(DeviceTest, ToString) {
    Device d;
    d.SetIP("192.168.1.10");
    d.SetOnline(true);
    auto str = d.ToString();
    EXPECT_NE(str.find("192.168.1.10"), std::string::npos);
    EXPECT_NE(str.find("Online"), std::string::npos);
}

TEST(DeviceTest, ToJSON) {
    Device d;
    d.SetIP("192.168.1.10");
    d.SetMAC("00:11:22:33:44:55");
    d.SetHostname("test");
    d.SetOnline(true);

    auto j = d.ToJSON();
    EXPECT_EQ(j["ip"], "192.168.1.10");
    EXPECT_EQ(j["mac"], "00:11:22:33:44:55");
    EXPECT_EQ(j["hostname"], "test");
    EXPECT_TRUE(j["online"].get<bool>());
}
