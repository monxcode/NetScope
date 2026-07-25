#include <gtest/gtest.h>
#include "netscope/scan/ping_sweep.h"

using namespace netscope::scan;

TEST(PingSweepTest, Create) {
    EXPECT_NO_THROW({ PingSweep sweep; });
}

TEST(PingSweepTest, SetProperties) {
    PingSweep sweep;
    EXPECT_NO_THROW(sweep.SetMaxThreads(32));
    EXPECT_NO_THROW(sweep.SetTimeout(500));
}

TEST(PingSweepTest, Cancel) {
    PingSweep sweep;
    EXPECT_NO_THROW(sweep.Cancel());
}

TEST(PingSweepTest, ProgressCallback) {
    PingSweep sweep;
    int calls = 0;
    sweep.SetProgressCallback([&calls](const PingSweepProgress&) { calls++; });
    EXPECT_NO_THROW(sweep.SetProgressCallback(nullptr));
}
