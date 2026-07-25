#ifndef NETSCOPE_CORE_APPLICATION_H
#define NETSCOPE_CORE_APPLICATION_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

#include "netscope/core/config.h"
#include "netscope/core/logger.h"
#include "netscope/discovery/device.h"

namespace netscope {
namespace core {

enum class AppMode {
    Interactive,
    Scan,
    Monitor,
    DeviceList,
    DeviceInfo,
    PortScan,
    Export,
    Topology,
    ConfigCmd,
    Help,
    Version
};

struct CommandLineArgs {
    AppMode mode = AppMode::Interactive;
    std::vector<std::string> positional;
    std::unordered_map<std::string, std::string> options;
    std::string target_ip;
    std::string export_format;
};

class Application {
public:
    Application();
    ~Application();

    int Run(int argc, char* argv[]);

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

private:
    void Initialize();
    void Shutdown();
    CommandLineArgs ParseArgs(int argc, char* argv[]);
    int ExecuteMode(const CommandLineArgs& args);

    int InteractiveMode();
    int ScanMode(const CommandLineArgs& args);
    int MonitorMode(const CommandLineArgs& args);
    int DeviceListMode();
    int DeviceInfoMode(const std::string& ip);
    int PortScanMode(const std::string& ip, const CommandLineArgs& args);
    int ExportMode(const std::string& format);
    int ExportMode(const std::string& format,
                   const std::vector<discovery::Device>* devices);
    int TopologyMode();
    int ConfigMode();

    std::vector<discovery::Device> DoScan();
    std::vector<discovery::Device> DoScan(const std::string& subnet);
    void PrintDevices(const std::vector<discovery::Device>& devices);
    void PrintTopology(const std::vector<discovery::Device>& devices);
    void PrintInterfaces();
    static std::string Truncate(const std::string& s, size_t max_len);

    void PrintBanner();
    void PrintHelp();
    void PrintVersion();

    AppConfig config_;
    bool initialized_{false};
};

} // namespace core
} // namespace netscope

#endif
