#include <gtest/gtest.h>
#include "netscope/discovery/topology.h"
#include "netscope/discovery/device.h"

using namespace netscope::discovery;

TEST(TopologyTest, Create) {
    EXPECT_NO_THROW({ Topology topo; });
}

TEST(TopologyTest, AddDevice) {
    Topology topo;
    Device d;
    d.SetIP("192.168.1.10");
    d.SetHostname("test-pc");
    EXPECT_NO_THROW(topo.AddDevice(d));
    EXPECT_EQ(topo.Nodes().size(), 1);
}

TEST(TopologyTest, AddDuplicateDevice) {
    Topology topo;
    Device d;
    d.SetIP("192.168.1.10");
    topo.AddDevice(d);
    topo.AddDevice(d);
    EXPECT_EQ(topo.Nodes().size(), 1);
}

TEST(TopologyTest, GenerateASCII) {
    Topology topo;
    Device d;
    d.SetIP("192.168.1.10");
    d.SetHostname("test-pc");
    topo.AddDevice(d);
    topo.SetGateway("192.168.1.1");
    topo.SetInternet(true);

    std::string ascii = topo.GenerateASCII();
    EXPECT_FALSE(ascii.empty());
    EXPECT_NE(ascii.find("test-pc"), std::string::npos);
    EXPECT_NE(ascii.find("192.168.1.10"), std::string::npos);
}

TEST(TopologyTest, GenerateDOT) {
    Topology topo;
    Device d;
    d.SetIP("192.168.1.10");
    topo.AddDevice(d);
    topo.SetGateway("192.168.1.1");
    topo.SetInternet(true);

    std::string dot = topo.GenerateDOT();
    EXPECT_FALSE(dot.empty());
    EXPECT_NE(dot.find("digraph"), std::string::npos);
}

TEST(TopologyTest, Clear) {
    Topology topo;
    Device d;
    d.SetIP("192.168.1.10");
    topo.AddDevice(d);
    EXPECT_EQ(topo.Nodes().size(), 1);
    topo.Clear();
    EXPECT_EQ(topo.Nodes().size(), 0);
}

TEST(TopologyTest, DetermineDeviceType) {
    Topology topo;
    Device d;
    d.SetIP("192.168.1.10");
    d.SetOS({"Linux", 0.8, 64});

    scan::PortResult http;
    http.port = 80;
    http.open = true;
    http.service = "HTTP";
    d.AddOpenPort(http);

    topo.AddDevice(d);
    topo.SetGateway("192.168.1.1");

    EXPECT_FALSE(topo.GenerateASCII().empty());
}
