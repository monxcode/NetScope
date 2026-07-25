#include <gtest/gtest.h>
#include "netscope/discovery/device.h"

using namespace netscope::discovery;

TEST(DeviceTest, DefaultState) {
    Device d;
    EXPECT_FALSE(d.Online());
    EXPECT_TRUE(d.IP().empty());
    EXPECT_EQ(d.ResponseTimeMs(), 0);
}

TEST(DeviceTest, SetProperties) {
    Device d;
    d.SetIP("192.168.1.10");
    d.SetMAC("00:11:22:33:44:55");
    d.SetHostname("test-pc");
    d.SetOnline(true);
    d.SetResponseTimeMs(5);
    d.SetTTL(128);

    EXPECT_EQ(d.IP(), "192.168.1.10");
    EXPECT_EQ(d.MAC(), "00:11:22:33:44:55");
    EXPECT_TRUE(d.Online());
    EXPECT_EQ(d.ResponseTimeMs(), 5);
    EXPECT_EQ(d.TTL(), 128);
}

TEST(DeviceTest, OSGuess) {
    Device d;
    OSGuess os{"Windows", 0.85, 128};
    d.SetOS(os);
    EXPECT_EQ(d.OS().name, "Windows");
    EXPECT_DOUBLE_EQ(d.OS().confidence, 0.85);
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

TEST(DeviceTest, ToJSON) {
    Device d;
    d.SetIP("10.0.0.1");
    d.SetMAC("aa:bb:cc:dd:ee:ff");
    d.SetOnline(true);
    auto j = d.ToJSON();
    EXPECT_EQ(j["ip"], "10.0.0.1");
    EXPECT_TRUE(j["online"].get<bool>());
}
