#ifndef NETSCOPE_CORE_APPLICATION_H
#define NETSCOPE_CORE_APPLICATION_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "netscope/core/config.h"
#include "netscope/core/logger.h"

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
    int DeviceInfoMode(const CommandLineArgs& args);
    int PortScanMode(const CommandLineArgs& args);
    int ExportMode(const CommandLineArgs& args);
    int TopologyMode();
    int ConfigMode(const CommandLineArgs& args);

    void PrintBanner();
    void PrintHelp();
    void PrintVersion();

    std::unique_ptr<Logger> logger_;
    AppConfig config_;
    bool initialized_{false};
};

} // namespace core
} // namespace netscope

#endif
