#include "netscope/monitor/monitor.h"
#include "netscope/core/logger.h"
#include "netscope/scan/scanner.h"

#include <algorithm>
#include <sstream>

namespace netscope {
namespace monitor {

Monitor::Monitor() {
    core::Logger::Instance().Debug("Monitor created");
}

Monitor::~Monitor() {
    Stop();
}

void Monitor::Start(int interval_seconds) {
    if (running_.load(std::memory_order_acquire)) return;

    interval_seconds_ = interval_seconds;
    stop_requested_.store(false, std::memory_order_release);
    running_.store(true, std::memory_order_release);

    monitor_thread_ = std::thread(&Monitor::MonitorLoop, this);
    core::Logger::Instance().Info("Monitor started (interval="
                                  + std::to_string(interval_seconds_) + "s)");
}

void Monitor::Stop() {
    stop_requested_.store(true, std::memory_order_release);
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    running_.store(false, std::memory_order_release);
    core::Logger::Instance().Info("Monitor stopped");
}

bool Monitor::IsRunning() const {
    return running_.load(std::memory_order_acquire);
}

void Monitor::SetNotificationCallback(NotificationCallback callback) {
    callback_ = std::move(callback);
}

void Monitor::SetSubnet(const std::string& subnet) {
    subnet_ = subnet;
}

void Monitor::MonitorLoop() {
    scan::Scanner scanner;
    scanner.SetTimeout(2000);
    scanner.SetMaxThreads(16);

    core::Logger::Instance().Info("Monitor: using subnet " + subnet_);

    while (!stop_requested_.load(std::memory_order_acquire)) {
        core::Logger::Instance().Debug("Monitor: scanning " + subnet_);

        auto current_devices = scanner.ScanSubnet(subnet_);

        if (!previous_devices_.empty()) {
            DetectChanges(previous_devices_, current_devices);
        }

        previous_devices_ = current_devices;

        if (callback_) {
            auto now = std::chrono::system_clock::now();
            MonitorNotification summary;
            summary.event = MonitorEvent::DeviceOnline;
            summary.message = "Monitor cycle complete: "
                              + std::to_string(current_devices.size()) + " devices";
            summary.timestamp = now;
            callback_(summary);
        }

        int slept = 0;
        while (slept < interval_seconds_ * 1000
               && !stop_requested_.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            slept += 100;
        }
    }
}

void Monitor::DetectChanges(const std::vector<discovery::Device>& previous,
                             const std::vector<discovery::Device>& current) {
    auto now = std::chrono::system_clock::now();

    auto dev_in_list = [](const std::vector<discovery::Device>& list,
                          const std::string& ip) -> const discovery::Device* {
        for (const auto& d : list) {
            if (d.IP() == ip) return &d;
        }
        return nullptr;
    };

    for (const auto& prev : previous) {
        auto* cur = dev_in_list(current, prev.IP());
        if (!cur) {
            MonitorNotification n;
            n.event = MonitorEvent::DeviceDisconnected;
            n.device = prev;
            n.message = "Device went offline: " + prev.IP();
            n.timestamp = now;
            if (callback_) callback_(n);
            core::Logger::Instance().Info("Monitor: device offline " + prev.IP());
        }
    }

    for (const auto& cur : current) {
        auto* prev = dev_in_list(previous, cur.IP());
        if (!prev) {
            MonitorNotification n;
            n.event = MonitorEvent::DeviceConnected;
            n.device = cur;
            n.message = "New device: " + cur.IP()
                        + (cur.Hostname().empty() ? "" : " (" + cur.Hostname() + ")");
            n.timestamp = now;
            if (callback_) callback_(n);
            core::Logger::Instance().Info("Monitor: new device " + cur.IP());
            continue;
        }

        if (prev->MAC() != cur.MAC() && !prev->MAC().empty()
            && !cur.MAC().empty()) {
            MonitorNotification n;
            n.event = MonitorEvent::IPChanged;
            n.device = cur;
            n.message = "MAC changed for " + cur.IP() + ": "
                        + prev->MAC() + " -> " + cur.MAC();
            n.timestamp = now;
            if (callback_) callback_(n);
            core::Logger::Instance().Info("Monitor: MAC change " + cur.IP());
        }
    }
}

} // namespace monitor
} // namespace netscope
