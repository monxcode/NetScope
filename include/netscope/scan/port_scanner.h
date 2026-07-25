#ifndef NETSCOPE_SCAN_PORT_SCANNER_H
#define NETSCOPE_SCAN_PORT_SCANNER_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>

#include "netscope/core/thread_pool.h"

namespace netscope {
namespace scan {

struct PortResult {
    int port = 0;
    std::string protocol;
    bool open = false;
    std::string service;
    std::string banner;
};

struct PortScanProgress {
    int total = 0;
    int completed = 0;
    int open = 0;
};

class PortScanner {
public:
    using ProgressCallback = std::function<void(const PortScanProgress&)>;

    PortScanner();
    ~PortScanner();

    void SetProgressCallback(ProgressCallback callback);
    void SetMaxThreads(int threads);
    void SetTimeout(int timeout_ms);

    std::vector<PortResult> Scan(const std::string& ip,
                                 const std::vector<int>& ports);
    std::vector<PortResult> ScanRange(const std::string& ip,
                                      int start, int end);
    void ScanAsync(const std::string& ip,
                   const std::vector<int>& ports,
                   std::function<void(std::vector<PortResult>)> on_complete);

    void Cancel();
    bool IsRunning() const;

    PortScanner(const PortScanner&) = delete;
    PortScanner& operator=(const PortScanner&) = delete;

private:
    PortResult ScanPort(const std::string& ip, int port);

    std::unique_ptr<core::ThreadPool> pool_;
    ProgressCallback callback_;
    std::atomic<bool> cancelled_{false};
    std::atomic<bool> running_{false};
    int timeout_ms_{1000};
};

} // namespace scan
} // namespace netscope

#endif
