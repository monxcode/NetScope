#include "netscope/core/application.h"
#include "netscope/core/version.h"
#include "netscope/core/platform.h"
#include "netscope/scan/scanner.h"
#include "netscope/scan/ping_sweep.h"
#include "netscope/scan/port_scanner.h"
#include "netscope/scan/service_detector.h"
#include "netscope/export/exporter.h"
#include "netscope/discovery/topology.h"
#include "netscope/monitor/monitor.h"
#include "netscope/utils/network_utils.h"
#include "netscope/utils/statistics.h"
#include "netscope/fingerprint/os_detector.h"

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <thread>
#include <sstream>

namespace netscope {
namespace core {

Application::Application() = default;

Application::~Application() {
    if (initialized_) Shutdown();
}

int Application::Run(int argc, char* argv[]) {
    Initialize();
    auto args = ParseArgs(argc, argv);
    return ExecuteMode(args);
}

void Application::Initialize() {
    if (initialized_) return;
    Config::Instance().Load();
    const auto cfg = Config::Instance().Get();
    Logger::Instance().SetFileEnabled(cfg.logging.file_enabled);
    Logger::Instance().Info("NetScope v" NETSCOPE_VERSION " starting");
    Logger::Instance().Info("Platform: " + Platform::OSName());
    auto interfaces = Platform::EnumerateInterfaces();
    for (const auto& iface : interfaces) {
        if (!iface.is_loopback && iface.is_up && !iface.ip_address.empty()) {
            Logger::Instance().Info("Interface: " + iface.name + " (" + iface.ip_address + ")");
        }
    }
    initialized_ = true;
}

void Application::Shutdown() {
    Logger::Instance().Info("NetScope shutting down");
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

    auto get_option = [argc, argv](const std::string& opt) -> std::string {
        for (int i = 2; i < argc - 1; ++i) {
            if (argv[i] == opt) return argv[i + 1];
        }
        return "";
    };

    auto has_flag = [argc, argv](const std::string& flag) -> bool {
        for (int i = 2; i < argc; ++i) {
            if (argv[i] == flag) return true;
        }
        return false;
    };

    args.options["subnet"] = get_option("--subnet");
    args.options["interface"] = get_option("--interface");
    args.options["range"] = get_option("--range");
    args.options["format"] = get_option("--format");
    args.options["output"] = get_option("--output");
    args.options["timeout"] = get_option("--timeout");
    args.options["threads"] = get_option("--threads");

    if (cmd == "scan") {
        args.mode = AppMode::Scan;
    } else if (cmd == "monitor" || cmd == "watch") {
        args.mode = AppMode::Monitor;
    } else if (cmd == "devices" || cmd == "device") {
        if (argc > 2) {
            std::string sub = argv[2];
            if (sub == "list" || sub == "ls") {
                args.mode = AppMode::DeviceList;
            } else if (sub == "info") {
                if (argc > 3) {
                    args.mode = AppMode::DeviceInfo;
                    args.target_ip = argv[3];
                }
            } else {
                args.mode = AppMode::DeviceInfo;
                args.target_ip = sub;
            }
        } else {
            args.mode = AppMode::DeviceList;
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
    } else if (cmd == "interfaces" || cmd == "iface") {
        args.mode = AppMode::DeviceList;
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
        case AppMode::DeviceInfo:  return DeviceInfoMode(args.target_ip);
        case AppMode::PortScan:    return PortScanMode(args.target_ip, args);
        case AppMode::Export:      return ExportMode(args.export_format);
        case AppMode::Topology:    return TopologyMode();
        case AppMode::ConfigCmd:   return ConfigMode();
        case AppMode::Help:        PrintHelp();  return 0;
        case AppMode::Version:     PrintVersion(); return 0;
        default:                   PrintHelp(); return 1;
    }
}

int Application::InteractiveMode() {
    PrintBanner();
    std::cout << "  Type 'help' for commands, 'exit' to quit.\n\n";

    std::vector<discovery::Device> last_scan;
    double last_duration = 0.0;
    std::string input;
    while (true) {
        std::cout << "netscope> " << std::flush;
        if (!std::getline(std::cin, input)) break;

        if (input == "exit" || input == "quit") break;
        if (input == "help") { PrintHelp(); continue; }

        auto trim = [](const std::string& s) {
            size_t start = s.find_first_not_of(" \t");
            size_t end = s.find_last_not_of(" \t");
            return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
        };
        input = trim(input);
        if (input.empty()) continue;

        if (input == "scan") {
            last_scan = DoScan();
            last_duration = 0.0;
            continue;
        }
        if (input == "monitor" || input == "watch") {
            CommandLineArgs args;
            args.mode = AppMode::Monitor;
            MonitorMode(args);
            continue;
        }
        if (input == "devices" || input == "list" || input == "ls") {
            if (last_scan.empty()) {
                std::cout << "  No devices. Run 'scan' first.\n";
            } else {
                PrintDevices(last_scan);
                auto stats = utils::Statistics::Compute(last_scan, last_duration);
                utils::Statistics::Print(stats);
            }
            continue;
        }
        if (input == "interfaces" || input == "ifaces") {
            PrintInterfaces();
            continue;
        }
        if (input.substr(0, 5) == "ports") {
            std::string rest = input.length() > 6 ? input.substr(6) : "";
            std::string ip = rest;
            size_t space = rest.find(' ');
            if (space != std::string::npos) ip = rest.substr(0, space);

            CommandLineArgs args;
            args.mode = AppMode::PortScan;
            args.target_ip = ip;
            if (space != std::string::npos) {
                std::string remaining = rest.substr(space + 1);
                if (remaining.substr(0, 7) == "--range") {
                    args.options["range"] = remaining.length() > 8 ? remaining.substr(8) : "";
                }
            }
            if (!ip.empty()) PortScanMode(ip, args);
            else std::cout << "  Usage: ports <ip> [--range 1-1024]\n";
            continue;
        }
        if (input.substr(0, 4) == "info") {
            std::string ip = input.length() > 5 ? input.substr(5) : "";
            if (!ip.empty()) DeviceInfoMode(ip);
            else std::cout << "  Usage: info <ip>\n";
            continue;
        }
        if (input == "config") { ConfigMode(); continue; }
        if (input == "stats" && !last_scan.empty()) {
            auto stats = utils::Statistics::Compute(last_scan, last_duration);
            utils::Statistics::Print(stats);
            continue;
        }
        if (input == "topology") {
            if (!last_scan.empty()) PrintTopology(last_scan);
            else std::cout << "  No data. Run 'scan' first.\n";
            continue;
        }
        if (input == "version") { PrintVersion(); continue; }
        if (input == "clear" || input == "cls") {
            std::cout << "\033[2J\033[H";
            continue;
        }
        if (input == "export") {
            CommandLineArgs args;
            args.mode = AppMode::Export;
            args.export_format = "json";
            if (!last_scan.empty()) ExportMode("json", &last_scan);
            else std::cout << "  No data. Run 'scan' first.\n";
            continue;
        }
        std::cout << "  Commands: scan, list, ports <ip>, info <ip>, monitor,\n";
        std::cout << "            config, topology, stats, interfaces, export,\n";
        std::cout << "            clear, version, help, exit\n";
    }
    return 0;
}

int Application::ScanMode(const CommandLineArgs& args) {
    auto cfg = Config::Instance().Get();
    std::string subnet = cfg.network.subnet;

    if (!args.options.at("subnet").empty()) {
        subnet = args.options.at("subnet");
    }
    if (!args.options.at("interface").empty()) {
        auto ifaces = Platform::EnumerateInterfaces();
        for (const auto& iface : ifaces) {
            if (iface.name == args.options.at("interface") && !iface.ip_address.empty()) {
                subnet = utils::IPToCIDR(iface.ip_address, 24);
                break;
            }
        }
    }
    if (!args.options.at("timeout").empty()) {
        try { Config::Instance().Network().timeout_ms = std::stoi(args.options.at("timeout")); }
        catch (...) {}
    }
    if (!args.options.at("threads").empty()) {
        try { Config::Instance().Network().max_threads = std::stoi(args.options.at("threads")); }
        catch (...) {}
    }

    auto start_time = std::chrono::steady_clock::now();
    auto devices = DoScan(subnet);
    auto duration = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start_time).count();

    auto stats = utils::Statistics::Compute(devices, duration);
    utils::Statistics::Print(stats);

    Logger::Instance().Info("Scan complete: " + std::to_string(devices.size())
                            + " devices in " + utils::Statistics::FormatDuration(duration));

    if (!args.options.at("format").empty() || !args.options.at("output").empty()) {
        auto format = args.options.at("format").empty() ? "json" : args.options.at("format");
        auto path = args.options.at("output").empty()
            ? (fs::current_path() / "exports" / ("scan." + format))
            : fs::path(args.options.at("output"));
        export_::Exporter exp;
        auto fmt = export_::Exporter::StringToFormat(format);
        exp.Export(devices, path, fmt);
    }

    return 0;
}

int Application::PortScanMode(const std::string& ip, const CommandLineArgs& args) {
    if (ip.empty()) {
        std::cout << "  Usage: netscope ports <ip> [--range 1-1024]\n";
        return 1;
    }
    if (!utils::ValidateIP(ip)) {
        std::cout << "  Error: invalid IP address '" << ip << "'\n";
        return 1;
    }

    auto cfg = Config::Instance().Get();
    std::cout << "\n  Scanning ports on " << ip << " ...\n\n";

    scan::PortScanner scanner;
    int timeout = cfg.network.timeout_ms;
    int threads = cfg.network.max_threads;

    if (!args.options.at("timeout").empty()) {
        try { timeout = std::stoi(args.options.at("timeout")); } catch (...) {}
    }
    if (!args.options.at("threads").empty()) {
        try { threads = std::stoi(args.options.at("threads")); } catch (...) {}
    }

    scanner.SetTimeout(timeout);
    scanner.SetMaxThreads(threads);

    std::vector<int> ports;
    if (!args.options.at("range").empty()) {
        auto range = utils::ParsePortRange(args.options.at("range"));
        if (range.first > 0 && range.second > 0) {
            ports.reserve(range.second - range.first + 1);
            for (int p = range.first; p <= range.second; ++p) ports.push_back(p);
            std::cout << "  Range: " << range.first << "-" << range.second
                      << " (" << ports.size() << " ports)\n\n";
        }
    }
    if (ports.empty()) {
        ports = cfg.ports.default_ports;
    }

    std::atomic<bool> done{false};
    std::vector<scan::PortResult> results;

    scanner.SetProgressCallback([&done](const scan::PortScanProgress& p) {
        std::cout << "\r  Progress: " << p.completed << "/" << p.total
                  << " (" << p.open << " open)" << std::flush;
    });

    scanner.ScanAsync(ip, ports,
                      [&results, &done](std::vector<scan::PortResult> r) {
                          results = std::move(r);
                          done.store(true, std::memory_order_release);
                      });

    while (!done.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n\n";
    if (results.empty()) {
        std::cout << "  No open ports found on " << ip << "\n";
    } else {
        std::cout << "  Open ports on " << ip << ":\n\n";
        std::cout << "  " << std::left
                  << std::setw(8) << "Port"
                  << std::setw(10) << "Protocol"
                  << std::setw(18) << "Service"
                  << "Banner\n";
        std::cout << "  " << std::string(70, '-') << "\n";
        for (const auto& r : results) {
            std::string banner_short = r.banner.empty() ? "-"
                : Truncate(r.banner, 40);
            std::cout << "  " << std::left
                      << std::setw(8) << r.port
                      << std::setw(10) << r.protocol
                      << std::setw(18) << r.service
                      << banner_short << "\n";
        }
        std::cout << "\n";

        std::cout << "  Grabbing banners...\n";
        scan::ServiceDetector detector;
        auto services = detector.DetectServices(ip, ports);
        if (!services.empty()) {
            std::cout << "\n  Service details:\n\n";
            std::cout << "  " << std::left
                      << std::setw(8) << "Port"
                      << std::setw(18) << "Service"
                      << "Version\n";
            std::cout << "  " << std::string(50, '-') << "\n";
            for (const auto& s : services) {
                std::cout << "  " << std::left
                          << std::setw(8) << s.port
                          << std::setw(18) << s.name
                          << Truncate(s.version, 60) << "\n";
            }
            std::cout << "\n";
        }

        for (auto& r : results) {
            for (const auto& s : services) {
                if (s.port == r.port) {
                    r.banner = s.banner;
                }
            }
        }
    }

    Logger::Instance().Info("Port scan " + ip + ": "
                            + std::to_string(results.size()) + " open ports");
    return 0;
}

int Application::DeviceInfoMode(const std::string& ip) {
    if (ip.empty()) {
        std::cout << "  Usage: netscope device info <ip>\n";
        std::cout << "         netscope device <ip>\n";
        return 1;
    }
    if (!utils::ValidateIP(ip)) {
        std::cout << "  Error: invalid IP address '" << ip << "'\n";
        return 1;
    }

    std::cout << "\n  Gathering info for " << ip << " ...\n\n";

    scan::Scanner scanner;
    scanner.SetTimeout(Config::Instance().Get().network.timeout_ms);
    auto devices = scanner.ScanSubnet(ip);

    if (devices.empty()) {
        std::cout << "  No response from " << ip << " (offline or blocking ICMP)\n\n";
        std::cout << "  Attempting ARP lookup...\n";
        network::ARPScanner arp;
        auto arp_entry = arp.Resolve(ip);
        if (arp_entry.has_value()) {
            std::cout << "\n  ARP Entry:\n";
            std::cout << "  MAC:  " << arp_entry->mac_address << "\n";
            if (!arp_entry->vendor.empty()) {
                std::cout << "  Vendor: " << arp_entry->vendor << "\n";
            }
        }
        return 0;
    }

    auto& d = devices[0];

    std::cout << "  " << std::string(50, '=') << "\n";
    std::cout << "  Device Information: " << d.IP() << "\n";
    std::cout << "  " << std::string(50, '=') << "\n\n";

    std::cout << "  IP Address:   " << d.IP() << "\n";
    std::cout << "  MAC Address:  " << (d.MAC().empty() ? "N/A" : d.MAC()) << "\n";
    std::cout << "  Hostname:     " << (d.Hostname().empty() ? "N/A" : d.Hostname()) << "\n";
    std::cout << "  Vendor:       " << (d.Vendor().empty() ? "N/A" : d.Vendor()) << "\n";
    std::cout << "  Status:       " << (d.Online() ? "Online" : "Offline") << "\n";
    std::cout << "  OS:           " << d.OS().name << " ("
              << std::fixed << std::setprecision(0) << (d.OS().confidence * 100)
              << "% confidence)" << "\n";
    std::cout << "  TTL:          " << d.TTL() << "\n";
    std::cout << "  Response:     " << d.ResponseTimeMs() << " ms\n";

    if (!d.OpenPorts().empty()) {
        std::cout << "\n  Open Ports:\n";
        for (const auto& p : d.OpenPorts()) {
            std::cout << "    " << std::left << std::setw(8) << p.port
                      << std::setw(8) << p.protocol
                      << p.service << "\n";
        }
    }

    if (!d.Services().empty()) {
        std::cout << "\n  Services:\n";
        for (const auto& s : d.Services()) {
            std::cout << "    " << std::left << std::setw(8) << s.port
                      << std::setw(16) << s.name
                      << Truncate(s.version, 50) << "\n";
        }
    }

    std::cout << "\n";
    return 0;
}

int Application::DeviceListMode() {
    auto ifaces = Platform::EnumerateInterfaces();
    if (ifaces.empty()) {
        std::cout << "  No network interfaces found.\n";
        return 0;
    }

    std::cout << "\n  Network Interfaces:\n\n";
    std::cout << "  " << std::left
              << std::setw(16) << "Name"
              << std::setw(20) << "IP"
              << std::setw(18) << "MAC"
              << std::setw(8) << "Status"
              << "Description\n";
    std::cout << "  " << std::string(90, '-') << "\n";

    for (const auto& iface : ifaces) {
        if (iface.is_loopback) continue;
        std::cout << "  " << std::left
                  << std::setw(16) << iface.name
                  << std::setw(20) << (iface.ip_address.empty() ? "-" : iface.ip_address)
                  << std::setw(18) << (iface.mac_address.empty() ? "-" : iface.mac_address)
                  << std::setw(8) << (iface.is_up ? "UP" : "DOWN")
                  << Truncate(iface.description, 40) << "\n";
    }
    std::cout << "\n";
    return 0;
}

int Application::MonitorMode(const CommandLineArgs& args) {
    const auto cfg = Config::Instance().Get();
    std::string subnet = cfg.network.subnet;

    if (!args.options.at("subnet").empty()) {
        subnet = args.options.at("subnet");
    }

    std::cout << "\n  Starting network monitor on " << subnet << "\n";
    std::cout << "  Press Ctrl+C to stop.\n\n";

    monitor::Monitor monitor;
    monitor.SetSubnet(subnet);

    auto last_notification = std::chrono::steady_clock::now();

    monitor.SetNotificationCallback(
        [&last_notification](const monitor::MonitorNotification& n) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - last_notification).count();

            auto time_t = std::chrono::system_clock::to_time_t(n.timestamp);
            std::tm bt;
#ifdef _WIN32
            localtime_s(&bt, &time_t);
#else
            localtime_r(&time_t, &bt);
#endif
            char time_str[32];
            std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &bt);

            const char* event_str = "";
            switch (n.event) {
                case monitor::MonitorEvent::DeviceConnected:
                    event_str = "[NEW]    "; break;
                case monitor::MonitorEvent::DeviceDisconnected:
                    event_str = "[LEFT]   "; break;
                case monitor::MonitorEvent::IPChanged:
                    event_str = "[CHANGE] "; break;
                case monitor::MonitorEvent::DeviceOnline:
                    event_str = "[ONLINE] "; break;
                case monitor::MonitorEvent::DeviceOffline:
                    event_str = "[OFFLINE]"; break;
            }

            if (n.event == monitor::MonitorEvent::DeviceConnected
                || n.event == monitor::MonitorEvent::DeviceDisconnected
                || n.event == monitor::MonitorEvent::IPChanged) {
                std::cout << "  " << event_str << " " << time_str << " - "
                          << n.message << "\n";
            }

            if (elapsed >= 30) {
                std::cout << "  [STATUS] " << time_str << " - " << n.message << "\n";
                last_notification = now;
            }
        });

    Logger::Instance().Info("Monitor started on " + subnet);
    monitor.Start(cfg.monitor.interval_seconds);

    std::cout << "  Monitor running. Press Enter to stop.\n";
    std::cin.get();

    monitor.Stop();
    Logger::Instance().Info("Monitor stopped by user");
    return 0;
}

int Application::ExportMode(const std::string& format) {
    const auto cfg = Config::Instance().Get();
    std::string subnet = cfg.network.subnet;

    std::cout << "\n  Scanning " << subnet << " for export...\n";
    scan::Scanner scanner;
    scanner.SetTimeout(cfg.network.timeout_ms);
    auto devices = scanner.ScanSubnet(subnet);

    if (devices.empty()) {
        std::cout << "  No devices found. Nothing to export.\n";
        return 0;
    }

    return ExportMode(format, &devices);
}

int Application::ExportMode(const std::string& format,
                             const std::vector<discovery::Device>* devices) {
    export_::Exporter exp;
    auto fmt = export_::Exporter::StringToFormat(format);
    auto ext = export_::FormatToString(fmt);

    auto path = fs::current_path() / "exports"
                / ("scan_results_" + ext + "." + ext);

    fs::create_directories(path.parent_path());

    std::cout << "  Exporting " << devices->size() << " devices to "
              << ext << "...\n";

    bool ok = exp.Export(*devices, path, fmt);
    if (ok) {
        std::cout << "  Exported successfully: " << path.string() << "\n";
    } else {
        std::cout << "  Export failed.\n";
    }

    return ok ? 0 : 1;
}

int Application::TopologyMode() {
    const auto cfg = Config::Instance().Get();

    std::cout << "\n  Scanning " << cfg.network.subnet << " for topology...\n";
    scan::Scanner scanner;
    scanner.SetTimeout(cfg.network.timeout_ms);
    auto devices = scanner.ScanSubnet(cfg.network.subnet);

    if (devices.empty()) {
        std::cout << "  No devices found.\n";
        return 0;
    }

    PrintTopology(devices);

    auto dot_path = fs::current_path() / "exports" / "topology.dot";
    fs::create_directories(dot_path.parent_path());

    discovery::Topology topo;
    for (const auto& d : devices) {
        topo.AddDevice(d);
    }
    if (topo.ExportDOT(dot_path.string())) {
        std::cout << "  DOT export: " << dot_path.string() << "\n";
    }

    return 0;
}

int Application::ConfigMode() {
    const auto cfg = Config::Instance().Get();
    std::cout << "\n  Current Configuration:\n";
    std::cout << "  " << std::string(40, '-') << "\n";
    std::cout << "  Subnet:       " << cfg.network.subnet << "\n";
    std::cout << "  Timeout:      " << cfg.network.timeout_ms << " ms\n";
    std::cout << "  Retries:      " << cfg.network.retries << "\n";
    std::cout << "  Threads:      " << cfg.network.max_threads << "\n";
    std::cout << "  Ping Count:   " << cfg.network.ping_count << "\n";
    std::cout << "  Default Ports: ";
    for (size_t i = 0; i < cfg.ports.default_ports.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << cfg.ports.default_ports[i];
    }
    std::cout << "\n";
    std::cout << "  Monitor Int:  " << cfg.monitor.interval_seconds << "s";
    if (cfg.monitor.enabled) std::cout << " (enabled)";
    std::cout << "\n";
    std::cout << "  Auto Export:  " << (cfg.export_.auto_export ? "yes" : "no") << "\n";
    std::cout << "  Log Level:    " << cfg.logging.level << "\n";
    std::cout << "  Theme:        " << cfg.ui.theme << "\n";
    std::cout << std::endl;
    return 0;
}

std::vector<discovery::Device> Application::DoScan() {
    const auto cfg = Config::Instance().Get();
    return DoScan(cfg.network.subnet);
}

std::vector<discovery::Device> Application::DoScan(const std::string& subnet) {
    std::cout << "\n  Scanning " << subnet << " ...\n\n";

    scan::Scanner scanner;
    const auto cfg = Config::Instance().Get();
    scanner.SetTimeout(cfg.network.timeout_ms);
    scanner.SetMaxThreads(cfg.network.max_threads);

    std::atomic<bool> done{false};
    std::vector<discovery::Device> result;

    scanner.SetProgressCallback([&done](const scan::ScanProgress& p) {
        std::cout << "\r  Progress: [" << std::string(p.found, '#')
                  << std::string(p.total_hosts - p.found, '.') << "] "
                  << p.completed << "/" << p.total_hosts
                  << " (" << std::fixed << std::setprecision(0)
                  << p.elapsed_seconds << "s)"
                  << std::flush;
    });

    scanner.ScanSubnetAsync(subnet, [&result, &done](std::vector<discovery::Device> devices) {
        result = std::move(devices);
        done.store(true, std::memory_order_release);
    });

    while (!done.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n\n";
    PrintDevices(result);
    Logger::Instance().Info("Scan found " + std::to_string(result.size()) + " devices");
    return result;
}

void Application::PrintDevices(const std::vector<discovery::Device>& devices) {
    if (devices.empty()) {
        std::cout << "  No devices found.\n";
        return;
    }

    std::cout << "  Found " << devices.size() << " device(s):\n\n";

    std::cout << "  " << std::left
              << std::setw(18) << "IP"
              << std::setw(24) << "Hostname"
              << std::setw(10) << "Status"
              << std::setw(20) << "MAC"
              << std::setw(16) << "Vendor"
              << std::setw(16) << "OS"
              << std::setw(6) << "TTL"
              << std::setw(10) << "RTT"
              << "\n";
    std::cout << "  " << std::string(120, '-') << "\n";

    for (const auto& d : devices) {
        std::cout << "  " << std::left
                  << std::setw(18) << d.IP()
                  << std::setw(24) << (d.Hostname().empty() ? "-" : Truncate(d.Hostname(), 22))
                  << std::setw(10) << (d.Online() ? "Online" : "Offline")
                  << std::setw(20) << d.MAC()
                  << std::setw(16) << (d.Vendor().empty() ? "-" : Truncate(d.Vendor(), 14))
                  << std::setw(16) << Truncate(d.OS().name, 14)
                  << std::setw(6) << d.TTL()
                  << std::setw(10) << (d.ResponseTimeMs() > 0
                      ? std::to_string(d.ResponseTimeMs()) + "ms" : "-")
                  << "\n";
    }
    std::cout << "\n";
}

void Application::PrintTopology(const std::vector<discovery::Device>& devices) {
    discovery::Topology topo;
    for (const auto& d : devices) {
        topo.AddDevice(d);
    }

    std::string gw;
    auto ifaces = Platform::EnumerateInterfaces();
    for (const auto& iface : ifaces) {
        if (!iface.is_loopback && !iface.ip_address.empty()) {
            auto last_dot = iface.ip_address.rfind('.');
            if (last_dot != std::string::npos) {
                gw = iface.ip_address.substr(0, last_dot + 1) + "1";
                break;
            }
        }
    }
    if (!gw.empty()) topo.SetGateway(gw);
    topo.SetInternet(true);

    std::cout << "\n  Network Topology:\n\n";
    std::cout << topo.GenerateASCII() << "\n";
}

void Application::PrintInterfaces() {
    auto ifaces = Platform::EnumerateInterfaces();
    if (ifaces.empty()) {
        std::cout << "  No interfaces found.\n";
        return;
    }

    std::cout << "\n  Network Interfaces:\n\n";
    std::cout << "  " << std::left
              << std::setw(16) << "Name"
              << std::setw(20) << "IP"
              << std::setw(18) << "MAC"
              << std::setw(8) << "Status"
              << std::setw(18) << "Netmask"
              << "\n";
    std::cout << "  " << std::string(80, '-') << "\n";
    for (const auto& iface : ifaces) {
        if (iface.is_loopback) continue;
        std::cout << "  " << std::left
                  << std::setw(16) << iface.name
                  << std::setw(20) << (iface.ip_address.empty() ? "-" : iface.ip_address)
                  << std::setw(18) << (iface.mac_address.empty() ? "-" : iface.mac_address)
                  << std::setw(8) << (iface.is_up ? "UP" : "DOWN")
                  << (iface.netmask.empty() ? "-" : iface.netmask)
                  << "\n";
    }
    std::cout << "\n";
}

std::string Application::Truncate(const std::string& s, size_t max_len) {
    if (s.length() <= max_len) return s;
    return s.substr(0, max_len - 3) + "...";
}

void Application::PrintBanner() {
    std::cout << "\n";
    std::cout << "  \xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97\n";
    std::cout << "  \xe2\x95\x91       NetScope v" << NETSCOPE_VERSION;
    for (size_t i = 0; i < (12 - std::string(NETSCOPE_VERSION).length()); ++i)
        std::cout << " ";
    std::cout << "       \xe2\x95\x91\n";
    std::cout << "  \xe2\x95\x91  Network Topology Discovery Tool   \xe2\x95\x91\n";
    std::cout << "  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\n";
    std::cout << std::endl;
}

void Application::PrintHelp() {
    PrintBanner();
    std::cout << "  USAGE:\n";
    std::cout << "    netscope                        Interactive mode\n";
    std::cout << "    netscope scan                   Scan local network\n";
    std::cout << "    netscope scan --subnet <cidr>   Scan specific subnet\n";
    std::cout << "    netscope scan --iface <name>    Scan interface subnet\n";
    std::cout << "    netscope scan --format <fmt>    Export format (json|csv|txt|dot)\n";
    std::cout << "    netscope scan --output <path>   Export output path\n";
    std::cout << "    netscope scan --timeout <ms>    Set scan timeout\n";
    std::cout << "    netscope scan --threads <num>   Set thread count\n";
    std::cout << "    netscope monitor                Live network monitor\n";
    std::cout << "    netscope monitor --subnet <cidr> Monitor specific subnet\n";
    std::cout << "    netscope devices                List discovered devices\n";
    std::cout << "    netscope device <ip>            Show device details\n";
    std::cout << "    netscope device info <ip>       Show device details\n";
    std::cout << "    netscope ports <ip>             Scan ports on device\n";
    std::cout << "    netscope ports <ip> --range N-M Scan port range\n";
    std::cout << "    netscope ports <ip> --timeout <ms> Port scan timeout\n";
    std::cout << "    netscope export <format>        Export scan results\n";
    std::cout << "    netscope topology               Show network topology\n";
    std::cout << "    netscope interfaces             List network interfaces\n";
    std::cout << "    netscope config                 Show configuration\n";
    std::cout << "    netscope help                   Show this help\n";
    std::cout << "    netscope version                Show version\n";
    std::cout << "\n";
    std::cout << "  FORMATS: json, csv, txt, dot\n";
    std::cout << std::endl;
}

void Application::PrintVersion() {
    std::cout << "NetScope v" << NETSCOPE_VERSION << "\n";
    std::cout << NETSCOPE_DESCRIPTION << "\n";
    std::cout << "Platform: " << Platform::OSName() << "\n";
    std::cout << "Hostname: " << Platform::GetHostname() << "\n";
}

} // namespace core
} // namespace netscope
