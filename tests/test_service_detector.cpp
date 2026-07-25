#include <gtest/gtest.h>
#include "netscope/scan/service_detector.h"

using namespace netscope::scan;

TEST(ServiceDetectorTest, Create) {
    EXPECT_NO_THROW({ ServiceDetector sd; });
}

TEST(ServiceDetectorTest, IdentifyService) {
    ServiceDetector sd;
    EXPECT_EQ("HTTP", sd.GrabBanner("127.0.0.1", 80).has_value() ? "found" : "no connection");
}

TEST(ServiceDetectorTest, KnownPorts) {
    ServiceDetector sd;
    auto svc = sd.GrabBanner("127.0.0.1", 22);
    if (svc.has_value()) {
        EXPECT_EQ(svc->port, 22);
        EXPECT_EQ(svc->protocol, "tcp");
    }
}

TEST(ServiceDetectorTest, DetectServices) {
    ServiceDetector sd;
    sd.SetMaxThreads(4);
    sd.SetTimeout(1000);
    EXPECT_NO_THROW(sd.DetectServices("127.0.0.1", {80, 443, 22}));
}
