#include <gtest/gtest.h>
#include "netscope/utils/network_utils.h"

using namespace netscope::utils;

TEST(NetworkUtilsTest, ValidateIP) {
    EXPECT_TRUE(ValidateIP("192.168.1.1"));
    EXPECT_TRUE(ValidateIP("0.0.0.0"));
    EXPECT_TRUE(ValidateIP("255.255.255.255"));
    EXPECT_FALSE(ValidateIP(""));
    EXPECT_FALSE(ValidateIP("not-an-ip"));
    EXPECT_FALSE(ValidateIP("256.1.2.3"));
    EXPECT_FALSE(ValidateIP("192.168.1"));
    EXPECT_FALSE(ValidateIP("192.168.1.1.1"));
}

TEST(NetworkUtilsTest, ValidateSubnet) {
    EXPECT_TRUE(ValidateSubnet("192.168.1.0/24"));
    EXPECT_TRUE(ValidateSubnet("10.0.0.0/8"));
    EXPECT_TRUE(ValidateSubnet("192.168.1.1"));
    EXPECT_FALSE(ValidateSubnet(""));
    EXPECT_FALSE(ValidateSubnet("not-a-subnet"));
    EXPECT_FALSE(ValidateSubnet("256.1.2.3/24"));
    EXPECT_FALSE(ValidateSubnet("192.168.1.0/33"));
}

TEST(NetworkUtilsTest, ValidatePort) {
    EXPECT_TRUE(ValidatePort(80));
    EXPECT_TRUE(ValidatePort(1));
    EXPECT_TRUE(ValidatePort(65535));
    EXPECT_FALSE(ValidatePort(0));
    EXPECT_FALSE(ValidatePort(65536));
    EXPECT_FALSE(ValidatePort(-1));
}

TEST(NetworkUtilsTest, ValidatePortRange) {
    EXPECT_TRUE(ValidatePortRange(1, 1024));
    EXPECT_TRUE(ValidatePortRange(80, 80));
    EXPECT_FALSE(ValidatePortRange(0, 80));
    EXPECT_FALSE(ValidatePortRange(80, 79));
    EXPECT_FALSE(ValidatePortRange(1, 65536));
}

TEST(NetworkUtilsTest, ParsePortList) {
    auto ports = ParsePortList("80,443,8080");
    ASSERT_EQ(ports.size(), 3);
    EXPECT_EQ(ports[0], 80);
    EXPECT_EQ(ports[1], 443);
    EXPECT_EQ(ports[2], 8080);
}

TEST(NetworkUtilsTest, ParsePortListWithRange) {
    auto ports = ParsePortList("80,443,8000-8005");
    EXPECT_GE(ports.size(), 7);
    EXPECT_EQ(ports[0], 80);
    EXPECT_EQ(ports[1], 443);
    EXPECT_EQ(ports[2], 8000);
    EXPECT_EQ(ports[3], 8001);
}

TEST(NetworkUtilsTest, ParsePortRange) {
    auto range = ParsePortRange("1-1024");
    EXPECT_EQ(range.first, 1);
    EXPECT_EQ(range.second, 1024);
}

TEST(NetworkUtilsTest, ParsePortRangeInvalid) {
    auto range = ParsePortRange("invalid");
    EXPECT_EQ(range.first, 0);
    EXPECT_EQ(range.second, 0);
}

TEST(NetworkUtilsTest, IPToUint) {
    EXPECT_EQ(IPToUint("0.0.0.0"), 0u);
    EXPECT_NE(IPToUint("192.168.1.1"), 0u);
    EXPECT_EQ(IPToUint("invalid"), 0u);
}

TEST(NetworkUtilsTest, UintToIP) {
    EXPECT_EQ(UintToIP(0), "0.0.0.0");
    EXPECT_FALSE(UintToIP(3232235521).empty());
}

TEST(NetworkUtilsTest, ValidateMAC) {
    EXPECT_TRUE(ValidateMAC("00:11:22:33:44:55"));
    EXPECT_TRUE(ValidateMAC("FF:EE:DD:CC:BB:AA"));
    EXPECT_FALSE(ValidateMAC(""));
    EXPECT_FALSE(ValidateMAC("00:11:22:33:44"));
    EXPECT_FALSE(ValidateMAC("00:11:22:33:44:55:66"));
    EXPECT_FALSE(ValidateMAC("not-a-mac"));
}

TEST(NetworkUtilsTest, ParseCIDR) {
    auto [ip, prefix] = ParseCIDR("192.168.1.0/24");
    EXPECT_EQ(ip, "192.168.1.0");
    EXPECT_EQ(prefix, 24);
}

TEST(NetworkUtilsTest, ParseCIDRNoPrefix) {
    auto [ip, prefix] = ParseCIDR("192.168.1.1");
    EXPECT_EQ(ip, "192.168.1.1");
    EXPECT_EQ(prefix, 32);
}

TEST(NetworkUtilsTest, ExpandCIDR) {
    auto hosts = ExpandCIDR("192.168.1.0/24", 5);
    ASSERT_EQ(hosts.size(), 5);
    EXPECT_EQ(hosts[0], "192.168.1.1");
    EXPECT_EQ(hosts[4], "192.168.1.5");
}

TEST(NetworkUtilsTest, ExpandCIDRSmall) {
    auto hosts = ExpandCIDR("192.168.1.0/30");
    ASSERT_EQ(hosts.size(), 2);
    EXPECT_EQ(hosts[0], "192.168.1.1");
    EXPECT_EQ(hosts[1], "192.168.1.2");
}

TEST(NetworkUtilsTest, ExpandCIDRInvalid) {
    auto hosts = ExpandCIDR("invalid");
    EXPECT_TRUE(hosts.empty());
}
