#include "netscope/core/application.h"
#include "netscope/core/version.h"
#include "netscope/ui/terminal.h"
#include "netscope/ui/menu.h"

#include <iostream>
#include <sstream>
#include <algorithm>

namespace netscope {
namespace core {

Application::Application() = default;

Application::~Application() {
    if (initialized_) {
        Shutdown();
    }
}

int Application::Run(int argc, char* argv[]) {
    try {
        Initialize();
        auto args = ParseArgs(argc, argv);
        return ExecuteMode(args);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        Logger::Instance().Fatal(std::string("Unhandled exception: ") + e.what());
        return 1;
    }
}

void Application::Initialize() {
    if (initialized_) return;

    Config::Instance().Load();

    auto& cfg = Config::Instance();

    auto& logger = Logger::Instance();
    logger.SetLevel(LogLevel::INFO);
    logger.SetFileEnabled(cfg.Get().logging.file_enabled);
    logger.SetMaxSizeMB(cfg.Get().logging.max_size_mb);

    Logger::Instance().Info(NETSCOPE_NAME " v" NETSCOPE_VERSION " starting");
    Logger::Instance().Info("Platform: " + Platform::OSName());
    Logger::Instance().Info("Admin: " + std::string(Platform::IsAdmin() ? "yes" : "no"));

    ui::Terminal::Init();

    initialized_ = true;
}

void Application::Shutdown() {
    Logger::Instance().Info(NETSCOPE_NAME " shutting down");
    ui::Terminal::Reset();
    initialized_ = false;
}

CommandLineArgs Application::ParseArgs(int argc, char* argv[]) {
    CommandLineArgs args;

    if (argc < 2) {
        args.mode = AppMode::Interactive;
        return args;
    }

    std::string cmd = argv[1];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "scan") {
        args.mode = AppMode::Scan;
    } else if (cmd == "monitor") {
        args.mode = AppMode::Monitor;
    } else if (cmd == "device" && argc > 2) {
        std::string sub = argv[2];
        if (sub == "list") {
            args.mode = AppMode::DeviceList;
        } else if (sub == "info" && argc > 3) {
            args.mode = AppMode::DeviceInfo;
            args.target_ip = argv[3];
        }
    } else if (cmd == "ports" && argc > 2) {
        args.mode = AppMode::PortScan;
        args.target_ip = argv[2];
    } else if (cmd == "export" && argc > 2) {
        args.mode = AppMode::Export;
        args.export_format = argv[2];
    } else if (cmd == "topology") {
        args.mode = AppMode::Topology;
    } else if (cmd == "config") {
        args.mode = AppMode::ConfigCmd;
    } else if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        args.mode = AppMode::Help;
    } else if (cmd == "version" || cmd == "--version" || cmd == "-v") {
        args.mode = AppMode::Version;
    } else {
        args.mode = AppMode::Help;
    }

    return args;
}

int Application::ExecuteMode(const CommandLineArgs& args) {
    switch (args.mode) {
        case AppMode::Interactive: return InteractiveMode();
        case AppMode::Scan:        return ScanMode(args);
        case AppMode::Monitor:     return MonitorMode(args);
        case AppMode::DeviceList:  return DeviceListMode();
        case AppMode::DeviceInfo:  return DeviceInfoMode(args);
        case AppMode::PortScan:    return PortScanMode(args);
        case AppMode::Export:      return ExportMode(args);
        case AppMode::Topology:    return TopologyMode();
        case AppMode::ConfigCmd:   return ConfigMode(args);
        case AppMode::Help:
            PrintHelp();
            return 0;
        case AppMode::Version:
            PrintVersion();
            return 0;
        default:
            PrintHelp();
            return 1;
    }
}

int Application::InteractiveMode() {
    PrintBanner();

    ui::Menu menu("NetScope - Network Topology Discovery Tool");

    menu.AddItem(1, "Scan Network", [this]() {
        ui::Terminal::PrintInfo("Starting network scan...");
    }, "Discover all devices on the local network");

    menu.AddItem(2, "Live Monitor", [this]() {
        ui::Terminal::PrintInfo("Starting live monitor...");
    }, "Monitor network for changes in real-time");

    menu.AddItem(3, "Device List", [this]() {
        DeviceListMode();
    }, "Show all discovered devices");

    menu.AddItem(4, "Port Scanner", [this]() {
        ui::Terminal::PrintInfo("Port scanner ready...");
    }, "Scan ports on a specific device");

    menu.AddItem(5, "Export Results", [this]() {
        ui::Terminal::PrintInfo("Export ready...");
    }, "Export scan results to file");

    menu.AddItem(6, "Settings", [this]() {
        ui::Terminal::PrintInfo("Settings...");
    }, "Configure NetScope");

    menu.AddSeparator();

    menu.AddItem(0, "Exit", [this]() {
        ui::Terminal::PrintInfo("Goodbye!");
    }, "Exit NetScope");

    menu.SetFooter("Navigate with number keys, press Enter");

    menu.Run();
    return 0;
}

int Application::ScanMode(const CommandLineArgs& args) {
    ui::Terminal::PrintHeader("Network Scan");
    ui::Terminal::PrintInfo("Scanning subnet: " + config_.network.subnet);
    Logger::Instance().Info("Scan mode initiated for subnet: " + config_.network.subnet);
    return 0;
}

int Application::MonitorMode(const CommandLineArgs& args) {
    ui::Terminal::PrintHeader("Live Monitor");
    ui::Terminal::PrintInfo("Monitor mode - not yet fully implemented");
    Logger::Instance().Info("Monitor mode activated");
    return 0;
}

int Application::DeviceListMode() {
    ui::Terminal::PrintHeader("Device List");
    ui::Terminal::PrintInfo("No devices discovered yet. Run a scan first.");
    return 0;
}

int Application::DeviceInfoMode(const CommandLineArgs& args) {
    if (args.target_ip.empty()) {
        ui::Terminal::PrintError("No IP address specified");
        return 1;
    }
    ui::Terminal::PrintHeader("Device Info: " + args.target_ip);
    return 0;
}

int Application::PortScanMode(const CommandLineArgs& args) {
    if (args.target_ip.empty()) {
        ui::Terminal::PrintError("No IP address specified");
        return 1;
    }
    ui::Terminal::PrintHeader("Port Scan: " + args.target_ip);
    return 0;
}

int Application::ExportMode(const CommandLineArgs& args) {
    ui::Terminal::PrintHeader("Export");
    std::string fmt = args.export_format.empty() ? "json" : args.export_format;
    ui::Terminal::PrintInfo("Exporting in " + fmt + " format...");
    return 0;
}

int Application::TopologyMode() {
    ui::Terminal::PrintHeader("Network Topology");
    ui::Terminal::PrintInfo("Topology generation - not yet implemented");
    return 0;
}

int Application::ConfigMode(const CommandLineArgs& args) {
    ui::Terminal::PrintHeader("Configuration");
    auto& cfg = Config::Instance().Get();
    ui::Terminal::PrintInfo("Current configuration:");
    ui::Terminal::Print("  Subnet: " + cfg.network.subnet);
    ui::Terminal::Print("  Timeout: " + std::to_string(cfg.network.timeout_ms) + "ms");
    ui::Terminal::Print("  Threads: " + std::to_string(cfg.network.max_threads));
    return 0;
}

void Application::PrintBanner() {
    ui::Terminal::Clear();
    ui::Terminal::SetColor(ui::Color::Cyan);
    ui::Terminal::SetStyle(ui::Style::Bold);
    std::cout << R"(
    ╔═══════════════════════════════════════════╗
    ║          NetScope v)" << NETSCOPE_VERSION << R"(                ║
    ║    Network Topology Discovery Tool        ║
    ╚═══════════════════════════════════════════╝
)" << std::endl;
    ui::Terminal::ResetStyle();
}

void Application::PrintHelp() {
    PrintBanner();
    std::cout << R"(
USAGE:
    netscope                       Interactive mode
    netscope scan                  Scan local network
    netscope monitor               Live network monitor
    netscope device list           List discovered devices
    netscope device info <ip>      Show device details
    netscope ports <ip>            Scan ports on device
    netscope export <format>       Export results (json|csv|txt)
    netscope topology              Show network topology
    netscope config                Show configuration
    netscope help                  Show this help
    netscope version               Show version

EXAMPLES:
    netscope scan
    netscope device info 192.168.1.10
    netscope ports 192.168.1.1
    netscope export json
)" << std::endl;
}

void Application::PrintVersion() {
    std::cout << NETSCOPE_NAME " v" NETSCOPE_VERSION << std::endl;
    std::cout << NETSCOPE_DESCRIPTION << std::endl;
    std::cout << "Platform: " << Platform::OSName() << std::endl;
}

} // namespace core
} // namespace netscope
