# NetScope

**Network Topology Discovery Tool** — A fast, modern, cross-platform LAN discovery, scanning, and monitoring application written in C++20.

## Features

- **Network Discovery** — Automatically discover devices on the local subnet (IP, MAC, hostname, vendor)
- **Ping Sweep** — Multi-threaded ping sweep with progress tracking
- **ARP Scanner** — Collect MAC addresses, interfaces, and vendor information
- **Hostname Resolution** — DNS-based hostname resolution
- **OS Detection** — Estimate operating system via TTL and port analysis with confidence levels
- **TCP Port Scanner** — Fast multi-threaded port scanning (default + custom ranges)
- **Service Detection** — Banner grabbing and service identification
- **Live Monitor** — Real-time network monitoring with change detection
- **Export** — JSON, CSV, TXT, and Graphviz DOT format export
- **ASCII Topology** — Generate network topology diagrams
- **Interactive TUI** — Modern terminal dashboard with keyboard navigation
- **CLI Mode** — Full command-line interface for automation

## Architecture

NetScope follows clean architecture principles with clear separation of concerns:

```
NetScope/
├── core/          # Application lifecycle, config, logging, thread pool, platform
├── network/       # Socket abstraction, ICMP, ARP, DNS resolution
├── scan/          # Scanner, ping sweep, port scanner, service detection
├── discovery/     # Device model, topology generation
├── ui/            # Terminal UI, menus, progress bars, tables
├── export/        # JSON, CSV, TXT export
└── monitor/       # Live network monitoring
```

### Design Principles

- **SOLID** — Single responsibility, open/closed, Liskov substitution, interface segregation, dependency inversion
- **RAII** — Resource acquisition is initialization for all system resources
- **Modern C++20** — Smart pointers, `std::thread`, `std::filesystem`, `std::optional`, `nlohmann/json`
- **Thread Safety** — All shared state protected with mutexes and atomics
- **No Global Mutable State** — Singleton only for config and logger (write-guarded)

## Building

### Prerequisites

- C++20 compiler (GCC 11+, Clang 14+, MSVC 2022+)
- CMake 3.20+
- nlohmann/json (fetched automatically by CMake)
- Google Test (optional, for tests)

### Build Steps

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run
./build/bin/netscope

# Run tests
cmake --build build --target netscope_tests
./build/bin/netscope_tests
```

### Windows (MSVC)

```bash
cmake -B build -G "Visual Studio 17 2022"
cmake --build build
```

### Linux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Usage

### Interactive Mode

```bash
netscope
```

Launches the terminal dashboard with keyboard navigation.

### Command Line

```bash
netscope scan                    # Scan local network
netscope monitor                 # Live network monitoring
netscope device list             # List discovered devices
netscope device info <ip>        # Show device details
netscope ports <ip>              # Port scan a device
netscope export json|csv|txt     # Export results
netscope topology                # Show network topology
netscope config                  # View configuration
netscope help                    # Show help
netscope version                 # Show version
```

## Configuration

Edit `config/config.json`:

```json
{
    "network": {
        "subnet": "192.168.1.0/24",
        "timeout_ms": 1000,
        "max_threads": 64
    },
    "ports": {
        "default": [21, 22, 23, 25, 53, 80, 110, 135, 139, 143, 443, 445, 3306, 3389, 8080]
    },
    "monitor": {
        "interval_seconds": 30
    },
    "ui": {
        "theme": "dark"
    }
}
```

## Roadmap

- [x] Project architecture and skeleton
- [x] Configuration management
- [x] Logging system
- [x] Thread pool
- [x] Platform abstraction (Windows/Linux)
- [x] Terminal UI framework
- [x] ICMP ping sweep
- [x] ARP scanning
- [x] DNS resolution
- [x] TCP port scanning
- [x] Service detection / banner grabbing
- [x] OS fingerprinting
- [x] JSON/CSV/TXT export
- [x] ASCII topology
- [ ] Graphviz DOT export
- [ ] Live monitor
- [ ] OUI database (vendor lookup)
- [ ] Device details panel
- [ ] Unit tests (Google Test)
- [ ] CI/CD pipeline
- [ ] Package distribution

## License

MIT License — see [LICENSE](LICENSE).

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Open a pull request
