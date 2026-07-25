#include "netscope/monitor/monitor.h"
#include "netscope/core/logger.h"
#include "netscope/core/config.h"

#include <sstream>
#include <algorithm>

namespace netscope {
namespace monitor {

Monitor::Monitor() {
    auto& cfg = core::Config::Instance().Get();
    subnet_ = cfg.network.subnet;
    interval_seconds_ = cfg.monitor.interval_seconds;
    scanner_ = std::make_unique<scan::Scanner>();
}

Monitor::~Monitor() {
    Stop();
}

void Monitor::Start(int interval_seconds) {
    if (running_.load(std::memory_order_acquire)) {
        core::Logger::Instance().Warn("Monitor is already running");
        return;
    }

    interval_seconds_ = interval_seconds;
    stop_requested_.store(false, std::memory_order_release);
    running_.store(true, std::memory_order_release);

    monitor_thread_ = std::thread(&Monitor::MonitorLoop, this);

    core::Logger::Instance().Info("Network monitor started (interval: " +
                                   std::to_string(interval_seconds_) + "s)");
}

void Monitor::Stop() {
    if (!running_.load(std::memory_order_acquire)) return;

    stop_requested_.store(true, std::memory_order_release);
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    running_.store(false, std::memory_order_release);

    core::Logger::Instance().Info("Network monitor stopped");
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
    while (!stop_requested_.load(std::memory_order_acquire)) {
        core::Logger::Instance().Debug("Monitor scan cycle starting");

        auto devices = scanner_->ScanSubnet(subnet_);

        if (!previous_devices_.empty()) {
            CompareDevices(devices);
        }

        previous_devices_ = devices;

        for (int i = 0; i < interval_seconds_ && !stop_requested_.load(std::memory_order_acquire); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void Monitor::CompareDevices(const std::vector<discovery::Device>& current) {
    for (const auto& device : current) {
        auto it = std::find_if(previous_devices_.begin(), previous_devices_.end(),
                                [&device](const discovery::Device& d) {
                                    return d.IP() == device.IP();
                                });

        if (it == previous_devices_.end()) {
            Notify(MonitorEvent::DeviceConnected, device,
                   "New device connected: " + device.IP());
        } else if (!it->Online() && device.Online()) {
            Notify(MonitorEvent::DeviceOnline, device,
                   "Device is now online: " + device.IP());
        } else if (it->Online() && !device.Online()) {
            Notify(MonitorEvent::DeviceOffline, device,
                   "Device went offline: " + device.IP());
        } else if (it->MAC() != device.MAC() && device.MAC() != it->MAC() &&
                   !it->MAC().empty() && !device.MAC().empty()) {
            Notify(MonitorEvent::IPChanged, device,
                   "MAC changed for IP " + device.IP() + ": " +
                   it->MAC() + " -> " + device.MAC());
        }
    }

    for (const auto& device : previous_devices_) {
        auto it = std::find_if(current.begin(), current.end(),
                                [&device](const discovery::Device& d) {
                                    return d.IP() == device.IP();
                                });
        if (it == current.end()) {
            Notify(MonitorEvent::DeviceDisconnected, device,
                   "Device disconnected: " + device.IP());
        }
    }
}

void Monitor::Notify(MonitorEvent event, const discovery::Device& device,
                     const std::string& message) {
    MonitorNotification notification;
    notification.event = event;
    notification.device = device;
    notification.message = message;
    notification.timestamp = std::chrono::system_clock::now();

    if (callback_) {
        callback_(notification);
    }

    switch (event) {
        case MonitorEvent::DeviceConnected:
            core::Logger::Instance().Info("[MONITOR] " + message);
            break;
        case MonitorEvent::DeviceDisconnected:
            core::Logger::Instance().Warn("[MONITOR] " + message);
            break;
        case MonitorEvent::IPChanged:
            core::Logger::Instance().Warn("[MONITOR] " + message);
            break;
        case MonitorEvent::DeviceOnline:
            core::Logger::Instance().Info("[MONITOR] " + message);
            break;
        case MonitorEvent::DeviceOffline:
            core::Logger::Instance().Warn("[MONITOR] " + message);
            break;
    }
}

} // namespace monitor
} // namespace netscope
