# NetScope

**Professional Network Topology Discovery & LAN Analysis Tool**

NetScope is a cross-platform (Windows & Linux) C++20 network discovery tool that scans local networks to identify devices, their MAC addresses, vendors, open ports, running services, and operating systems. It includes an interactive CLI mode, live network monitoring, and multi-format export (JSON, CSV, TXT, Graphviz DOT).

## Features

- **Automatic Interface Detection** — lists all active network interfaces with IP, MAC, and status
- **ICMP Ping Sweep** — multi-threaded ping scan with configurable timeout and retries
- **ARP Network Scanner** — reads local ARP table (Windows `GetIpNetTable` / Linux `/proc/net/arp`) and resolves MAC vendors via OUI lookup (90+ vendors)
- **Reverse DNS / Hostname Resolution** — `gethostbyaddr` reverse lookup
- **OS Fingerprinting** — TTL-based OS detection with confidence scoring, enhanced by open port analysis
- **TCP Port Scanner** — multi-threaded non-blocking connect scan with progress callback; supports common ports, custom port lists, and port ranges
- **Service Detection** — banner grabbing for open ports with version string parsing
- **Device Information View** — detailed per-device view (IP, MAC, hostname, vendor, OS, TTL, RTT, open ports, services)
- **Network Topology** — ASCII tree topology diagram with device type classification; Graphviz DOT export
- **Live Network Monitor** — continuously monitors the LAN and detects new devices, offline devices, and MAC changes
- **Network Statistics** — total/online/offline devices, open ports, average/min/max latency, scan duration
- **Multi-format Export** — JSON, CSV, TXT, and Graphviz DOT
- **Interactive CLI Dashboard** — professional terminal interface with commands for scanning, listing, monitoring, and configuration
- **Configuration File** — JSON-based configuration (scan timeout, thread count, port list, subnet, logging level)
- **Structured Logging** — timestamped log files with severity levels (DEBUG, INFO, WARN, ERROR, FATAL)

## Architecture

```
NetScope/
├── CMakeLists.txt              # Root build configuration
├── CMakePresets.json            # Build presets (Debug/Release, MSVC/GCC/Clang)
├── config/
│   └── config.json              # Default configuration file
├── include/netscope/
│   ├── core/                    # Application, Config, Logger, Platform, ThreadPool
│   ├── network/                 # Socket, ICMP, ARP, DNS
│   ├── scan/                    # Scanner, PingSweep, PortScanner, ServiceDetector
│   ├── discovery/               # Device model, Topology
│   ├── export/                  # Exporter (JSON/CSV/TXT/DOT)
│   ├── monitor/                 # Live network monitor
│   ├── fingerprint/             # OS fingerprinting
│   └── utils/                   # Network utilities, Statistics
├── src/                         # Implementation files (mirrors include/ structure)
├── tests/                       # GoogleTest unit tests
├── exports/                     # Export output directory
└── logs/                        # Log output directory
```

## Requirements

| Component      | Minimum Version                             |
|----------------|---------------------------------------------|
| C++ Standard   | C++20                                       |
| CMake          | 3.20+                                       |
| Compiler       | GCC 11+, Clang 14+, MSVC 2022+              |
| nlohmann/json  | v3.11.3 (auto-fetched by CMake FetchContent) |
| GoogleTest     | v1.14.0 (auto-fetched for tests)            |
| libpcap        | Optional — only needed for raw ARP on Linux |
| Npcap          | Optional — only needed for raw ARP on Windows |

## Installation

### Windows

###### 1. Install Visual Studio 2022

Download and install [Visual Studio 2022](https://visualstudio.microsoft.com/). During setup select the **Desktop development with C++** workload.

###### 2. Install CMake

Download and install [CMake](https://cmake.org/download/) 3.20 or later. Ensure CMake is added to your system PATH during installation.

###### 3. Clone the Repository

```powershell
git clone https://github.com/yourusername/netscope.git
cd NetScope
```

###### 4. Configure the Project

```powershell
cmake --preset debug-windows
```

###### 5. Build the Project

```powershell
cmake --build build/msvc-debug --config Debug
```

###### 6. Run NetScope

```powershell
build\msvc-debug\bin\netscope.exe
```

###### 7. (Optional) Install Npcap

Download and install [Npcap](https://npcap.com/) if you need raw packet capture functionality. This is not required for standard ARP table scanning.

### Linux

###### 1. Install Build Dependencies

```bash
sudo apt update
sudo apt install build-essential cmake libpcap-dev
```

###### 2. Clone the Repository

```bash
git clone https://github.com/yourusername/netscope.git
cd NetScope
```

###### 3. Configure the Project

```bash
cmake --preset debug-linux
```

###### 4. Build the Project

```bash
cmake --build build/debug
```

###### 5. Run NetScope

```bash
sudo ./build/debug/bin/netscope
```

Root privileges are required for ICMP and ARP operations on Linux.

## Usage

```
netscope                           Interactive mode
netscope scan                      Scan local network
netscope scan --subnet 10.0.0.0/24 Scan specific subnet
netscope scan --interface eth0     Scan interface subnet
netscope scan --format json        Scan and export results
netscope scan --output results.json Save results to file
netscope scan --timeout 2000       Set scan timeout (ms)
netscope scan --threads 128        Set thread count
netscope monitor                   Live network monitor
netscope devices                   List interfaces
netscope device 192.168.1.10       Show device details
netscope ports 192.168.1.10        Scan common ports
netscope ports 192.168.1.10 --range 1-1024  Scan port range
netscope export json               Export results to JSON
netscope topology                  Show network topology
netscope config                    Show configuration
netscope interfaces                List network interfaces
netscope help                      Show help
netscope version                   Show version
```

### Interactive Mode Commands

| Command          | Description                          |
|------------------|--------------------------------------|
| `scan`           | Scan configured subnet               |
| `list` / `ls`    | List discovered devices              |
| `ports <ip>`     | Scan ports on a device               |
| `info <ip>`      | Show detailed device information     |
| `monitor`        | Start live network monitoring        |
| `topology`       | Show network topology diagram        |
| `stats`          | Show scan statistics                 |
| `interfaces`     | List network interfaces              |
| `config`         | Show current configuration           |
| `export`         | Export scan results to JSON          |
| `clear`          | Clear terminal                       |
| `version`        | Show version                         |
| `help`           | Show help                            |
| `exit` / `quit`  | Exit                                 |

## Configuration

Configuration file: `config/config.json`

```json
{
    "network": {
        "subnet": "192.168.1.0/24",
        "timeout_ms": 1000,
        "retries": 2,
        "max_threads": 64,
        "ping_count": 2
    },
    "ports": {
        "default": [21, 22, 23, 25, 53, 80, 110, 135, 139, 143, 443, 445, 3306, 3389, 8080],
        "custom": null,
        "range_start": null,
        "range_end": null
    },
    "monitor": {
        "interval_seconds": 30,
        "enabled": false
    },
    "export": {
        "default_format": "json",
        "auto_export": false
    },
    "logging": {
        "level": "info",
        "file_enabled": true,
        "max_size_mb": 10
    },
    "ui": {
        "theme": "dark",
        "refresh_rate_ms": 500
    }
}
```

## Troubleshooting

### Permission Denied
- **Linux**: ICMP ping and raw socket operations require root. Run with `sudo`.
- **Windows**: Some network operations (ARP table, raw sockets) may require Administrator privileges.

### No Npcap / libpcap
ARK scanning uses the OS ARP table API (`GetIpNetTable` on Windows, `/proc/net/arp` on Linux) and does NOT require libpcap/Npcap. Those libraries are only needed for advanced raw packet features.

### No Devices Found
- Verify the configured subnet is correct (`netscope config`)
- Run `netscope interfaces` to confirm your active interface
- Try increasing the timeout: `netscope scan --timeout 3000`
- Ensure ICMP is not blocked by a firewall
- Run with appropriate privileges

## Permissions

This tool only performs **passive and standard network discovery** operations:
- ICMP Echo (ping)
- ARP table queries
- TCP connection attempts (port scanning)
- Reverse DNS lookups

It does **NOT** perform any exploitation, credential attacks, or intrusive scanning. Only scan networks you own or have explicit authorization to test.

## Testing

```bash
# Build and run tests
cmake --preset default
cmake --build build/debug
cd build/debug && ctest --output-on-failure
```

### Test Categories

- **Unit Tests** — IP validation, CIDR parsing, subnet calculation, port parsing, configuration parsing, JSON/CSV export, device model, network utilities, OS detection, topology, statistics
- No tests require network access to pass

## Build Commands

```bash
# Debug build
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug

# Release build
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release

# Using presets
cmake --preset default
cmake --build --preset default

# Run tests
cd build/debug && ctest --output-on-failure
```

## Roadmap

- [x] Phase 1: Project skeleton, CMake, core modules
- [x] Phase 2: ICMP/ARP/DNS scanning, multi-threaded scanner, CIDR expansion, OS detection
- [x] Phase 3: TCP port scanner, service detection, banner grabbing
- [x] Phase 4: Full CLI options, export, monitor, topology, statistics, documentation, tests
- [ ] Phase 5: Web dashboard, REST API, SNMP support
- [ ] Phase 6: IPv6 support, WiFi scanning, packet capture

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
