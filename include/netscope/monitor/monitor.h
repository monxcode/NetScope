#ifndef NETSCOPE_MONITOR_MONITOR_H
#define NETSCOPE_MONITOR_MONITOR_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>

#include "netscope/discovery/device.h"
#include "netscope/scan/scanner.h"

namespace netscope {
namespace monitor {

enum class MonitorEvent {
    DeviceConnected,
    DeviceDisconnected,
    IPChanged,
    DeviceOnline,
    DeviceOffline
};

struct MonitorNotification {
    MonitorEvent event;
    discovery::Device device;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

class Monitor {
public:
    using NotificationCallback = std::function<void(const MonitorNotification&)>;

    Monitor();
    ~Monitor();

    void Start(int interval_seconds = 30);
    void Stop();
    bool IsRunning() const;

    void SetNotificationCallback(NotificationCallback callback);
    void SetSubnet(const std::string& subnet);

    Monitor(const Monitor&) = delete;
    Monitor& operator=(const Monitor&) = delete;
    Monitor(Monitor&&) = delete;
    Monitor& operator=(Monitor&&) = delete;

private:
    void MonitorLoop();
    void CompareDevices(const std::vector<discovery::Device>& current);
    void Notify(MonitorEvent event, const discovery::Device& device,
                const std::string& message);

    std::unique_ptr<scan::Scanner> scanner_;
    NotificationCallback callback_;
    std::string subnet_;
    std::vector<discovery::Device> previous_devices_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::thread monitor_thread_;
    int interval_seconds_{30};
};

} // namespace monitor
} // namespace netscope

#endif
