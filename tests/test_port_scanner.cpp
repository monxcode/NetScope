#include <gtest/gtest.h>
#include "netscope/scan/port_scanner.h"

using namespace netscope::scan;

TEST(PortScannerTest, Create) {
    EXPECT_NO_THROW({ PortScanner ps; });
}

TEST(PortScannerTest, SetProperties) {
    PortScanner ps;
    EXPECT_NO_THROW(ps.SetMaxThreads(32));
    EXPECT_NO_THROW(ps.SetTimeout(500));
}

TEST(PortScannerTest, InitialState) {
    PortScanner ps;
    EXPECT_FALSE(ps.IsRunning());
}

TEST(PortScannerTest, Cancel) {
    PortScanner ps;
    EXPECT_NO_THROW(ps.Cancel());
}

TEST(PortScannerTest, ProgressCallback) {
    PortScanner ps;
    int calls = 0;
    ps.SetProgressCallback([&calls](const PortScanProgress&) { calls++; });
    EXPECT_NO_THROW(ps.SetProgressCallback(nullptr));
}

TEST(PortScannerTest, ScanEmptyPorts) {
    PortScanner ps;
    auto results = ps.Scan("192.168.1.1", {});
    EXPECT_TRUE(results.empty());
}

TEST(PortScannerTest, ScanRangeInvalid) {
    PortScanner ps;
    auto results = ps.ScanRange("192.168.1.1", 10, 5);
    EXPECT_TRUE(results.empty());
}

TEST(PortScannerTest, ScanSingleHost) {
    PortScanner ps;
    ps.SetTimeout(200);
    auto results = ps.Scan("127.0.0.1", {80, 443});
    EXPECT_NO_THROW(results.size());
}
