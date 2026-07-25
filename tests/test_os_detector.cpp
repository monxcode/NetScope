#include <gtest/gtest.h>
#include "netscope/fingerprint/os_detector.h"

using namespace netscope::fingerprint;

TEST(OSDetectorTest, DetectFromTTLWindows) {
    OSDetector detector;
    auto guess = detector.DetectFromTTL(128);
    EXPECT_EQ(guess.name, "Windows 10/11");
    EXPECT_GT(guess.confidence, 0.5);
}

TEST(OSDetectorTest, DetectFromTTLLinux) {
    OSDetector detector;
    auto guess = detector.DetectFromTTL(64);
    EXPECT_TRUE(guess.name.find("Linux") != std::string::npos
                || guess.name.find("Unix") != std::string::npos);
    EXPECT_GT(guess.confidence, 0.5);
}

TEST(OSDetectorTest, DetectFromTTLNetworkDevice) {
    OSDetector detector;
    auto guess = detector.DetectFromTTL(255);
    EXPECT_GT(guess.confidence, 0);
}

TEST(OSDetectorTest, DetectFromTTLZero) {
    OSDetector detector;
    auto guess = detector.DetectFromTTL(0);
    EXPECT_EQ(guess.name, "Unknown");
    EXPECT_DOUBLE_EQ(guess.confidence, 0.0);
}

TEST(OSDetectorTest, DetectFromPortsWindows) {
    OSDetector detector;
    auto guess = detector.DetectFromPorts({135, 139, 445});
    EXPECT_EQ(guess.name, "Windows");
    EXPECT_GT(guess.confidence, 0.5);
}

TEST(OSDetectorTest, DetectFromPortsLinux) {
    OSDetector detector;
    auto guess = detector.DetectFromPorts({22});
    EXPECT_EQ(guess.name, "Linux/Unix");
    EXPECT_GT(guess.confidence, 0.5);
}

TEST(OSDetectorTest, DetectFromPortsEmpty) {
    OSDetector detector;
    auto guess = detector.DetectFromPorts({});
    EXPECT_EQ(guess.name, "Unknown");
    EXPECT_DOUBLE_EQ(guess.confidence, 0.0);
}

TEST(OSDetectorTest, GetOSFamily) {
    EXPECT_EQ(OSDetector::GetOSFamily("Windows 10/11"), "Windows");
    EXPECT_EQ(OSDetector::GetOSFamily("Linux"), "Linux");
    EXPECT_EQ(OSDetector::GetOSFamily("macOS"), "macOS");
    EXPECT_EQ(OSDetector::GetOSFamily("Cisco IOS"), "Network");
    EXPECT_EQ(OSDetector::GetOSFamily("Unknown"), "Unknown");
}

TEST(OSDetectorTest, OSDescription) {
    discovery::OSGuess guess{"Windows", 0.85, 128};
    auto desc = OSDetector::OSDescription(guess);
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("Windows"), std::string::npos);
}
