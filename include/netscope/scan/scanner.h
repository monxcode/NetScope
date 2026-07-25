#ifndef NETSCOPE_SCAN_SCANNER_H
#define NETSCOPE_SCAN_SCANNER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>

#include "netscope/core/thread_pool.h"
#include "netscope/discovery/device.h"

namespace netscope {
namespace scan {

struct ScanProgress {
    int total_hosts = 0;
    int completed = 0;
    int found = 0;
    double elapsed_seconds = 0.0;
};

class Scanner {
public:
    using ProgressCallback = std::function<void(const ScanProgress&)>;

    Scanner();
    ~Scanner();

    void SetProgressCallback(ProgressCallback callback);
    void SetMaxThreads(int threads);
    void SetTimeout(int timeout_ms);

    std::vector<discovery::Device> ScanSubnet(const std::string& subnet);
    void ScanSubnetAsync(const std::string& subnet,
                         std::function<void(std::vector<discovery::Device>)> on_complete);

    void Cancel();
    bool IsRunning() const;

    Scanner(const Scanner&) = delete;
    Scanner& operator=(const Scanner&) = delete;

private:
    std::vector<std::string> ExpandSubnet(const std::string& subnet);
    discovery::Device ScanHost(const std::string& ip);

    std::unique_ptr<core::ThreadPool> pool_;
    ProgressCallback progress_callback_;
    std::atomic<bool> cancelled_{false};
    std::atomic<bool> running_{false};
    int timeout_ms_{1000};
};

} // namespace scan
} // namespace netscope

#endif
