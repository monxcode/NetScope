#ifndef NETSCOPE_SCAN_PING_SWEEP_H
#define NETSCOPE_SCAN_PING_SWEEP_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>

#include "netscope/core/thread_pool.h"
#include "netscope/discovery/device.h"

namespace netscope {
namespace scan {

struct PingSweepProgress {
    int total = 0;
    int completed = 0;
    int alive = 0;
};

class PingSweep {
public:
    using ProgressCallback = std::function<void(const PingSweepProgress&)>;

    PingSweep();
    ~PingSweep();

    void SetProgressCallback(ProgressCallback callback);
    void SetMaxThreads(int threads);
    void SetTimeout(int timeout_ms);

    std::vector<discovery::Device> Sweep(const std::string& subnet);
    void Cancel();

    PingSweep(const PingSweep&) = delete;
    PingSweep& operator=(const PingSweep&) = delete;

private:
    std::unique_ptr<core::ThreadPool> pool_;
    ProgressCallback callback_;
    std::atomic<bool> cancelled_{false};
    int timeout_ms_{1000};
};

} // namespace scan
} // namespace netscope

#endif
