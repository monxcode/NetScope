#include <gtest/gtest.h>
#include "netscope/scan/scanner.h"

using namespace netscope::scan;

TEST(ScannerTest, CreateAndDestroy) {
    EXPECT_NO_THROW({ Scanner scanner; });
}

TEST(ScannerTest, InitialState) {
    Scanner scanner;
    EXPECT_FALSE(scanner.IsRunning());
}

TEST(ScannerTest, SetProperties) {
    Scanner scanner;
    EXPECT_NO_THROW(scanner.SetMaxThreads(16));
    EXPECT_NO_THROW(scanner.SetTimeout(2000));
}

TEST(ScannerTest, CancelBeforeStart) {
    Scanner scanner;
    EXPECT_NO_THROW(scanner.Cancel());
}

TEST(ScannerTest, ProgressCallback) {
    Scanner scanner;
    int calls = 0;
    scanner.SetProgressCallback([&calls](const ScanProgress&) { calls++; });
    EXPECT_NO_THROW(scanner.SetProgressCallback(nullptr));
}

TEST(ScannerTest, ScanEmptySubnet) {
    Scanner scanner;
    auto devices = scanner.ScanSubnet("");
    EXPECT_TRUE(devices.empty());
}
