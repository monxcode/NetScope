#include "netscope/fingerprint/os_detector.h"

#include <algorithm>
#include <sstream>

namespace netscope {
namespace fingerprint {

const std::vector<OSDetector::TTLEntry> OSDetector::kTTLTable = {
    {"Linux",         0.75, 60,  68},
    {"FreeBSD",       0.70, 60,  68},
    {"macOS",         0.70, 60,  68},
    {"Android",       0.65, 60,  68},
    {"Solaris",       0.60, 60,  68},
    {"Windows 10/11", 0.80, 120, 132},
    {"Windows 7/8",  0.75, 120, 132},
    {"Windows Server",0.70, 120, 132},
    {"Windows 2000",  0.60, 120, 132},
    {"Cisco IOS",     0.65, 220, 260},
    {"Network Device",0.50, 220, 260},
    {"Printer",       0.40, 220, 260},
};

const std::unordered_map<int, std::string> OSDetector::kPortOSMap = {
    {22,   "Linux/Unix/Network Device"},
    {135,  "Windows"},
    {139,  "Windows"},
    {445,  "Windows"},
    {3389, "Windows"},
    {5800, "VNC (multi-platform)"},
    {5900, "VNC (multi-platform)"},
};

discovery::OSGuess OSDetector::DetectFromTTL(int ttl) {
    discovery::OSGuess best;
    best.ttl = ttl;

    if (ttl <= 0) return {"Unknown", 0.0, ttl};

    for (const auto& entry : kTTLTable) {
        if (ttl >= entry.min_ttl && ttl <= entry.max_ttl) {
            if (entry.confidence > best.confidence) {
                best.name = entry.name;
                best.confidence = entry.confidence;
            }
        }
    }

    if (best.confidence == 0.0) {
        if (ttl <= 64) {
            best = {"Linux/Unix", 0.50, ttl};
        } else if (ttl <= 128) {
            best = {"Windows", 0.50, ttl};
        } else {
            best = {"Network Device", 0.40, ttl};
        }
    }

    return best;
}

discovery::OSGuess OSDetector::DetectFromPorts(const std::vector<int>& open_ports) {
    discovery::OSGuess guess;
    guess.name = "Unknown";
    guess.confidence = 0.0;

    if (open_ports.empty()) return guess;

    bool has_windows_ports = false;
    bool has_unix_ports = false;

    for (int port : open_ports) {
        if (port == 135 || port == 139 || port == 445 || port == 3389) {
            has_windows_ports = true;
        }
        if (port == 22) {
            has_unix_ports = true;
        }
    }

    if (has_windows_ports && !has_unix_ports) {
        guess = {"Windows", 0.60, 0};
    } else if (has_unix_ports && !has_windows_ports) {
        guess = {"Linux/Unix", 0.60, 0};
    } else if (has_windows_ports && has_unix_ports) {
        guess = {"Multi-OS", 0.40, 0};
    }

    return guess;
}

discovery::OSGuess OSDetector::DetectFromTCPBehavior(int ttl,
                                                      const std::vector<int>& open_ports,
                                                      int response_time_ms) {
    auto ttl_guess = DetectFromTTL(ttl);
    auto port_guess = DetectFromPorts(open_ports);

    if (ttl_guess.confidence < 0.5 && port_guess.confidence > 0.5) {
        return port_guess;
    }

    if (port_guess.confidence > 0.3 && ttl_guess.confidence > 0.3) {
        discovery::OSGuess combined;
        combined.ttl = ttl;

        if (ttl_guess.name.find("Windows") != std::string::npos
            && port_guess.name.find("Windows") != std::string::npos) {
            combined.name = ttl_guess.name;
            combined.confidence = std::min(0.85, ttl_guess.confidence + 0.15);
        } else if (ttl_guess.name.find("Linux") != std::string::npos
                   && port_guess.name.find("Linux") != std::string::npos) {
            combined.name = ttl_guess.name;
            combined.confidence = std::min(0.85, ttl_guess.confidence + 0.15);
        } else {
            combined.name = ttl_guess.name;
            combined.confidence = ttl_guess.confidence;
        }

        return combined;
    }

    return ttl_guess;
}

discovery::OSGuess OSDetector::Detect(const discovery::Device& device) {
    std::vector<int> ports;
    for (const auto& p : device.OpenPorts()) {
        ports.push_back(p.port);
    }

    int ttl = device.TTL();
    int rtt = device.ResponseTimeMs();

    return DetectFromTCPBehavior(ttl, ports, rtt);
}

std::string OSDetector::GetOSFamily(const std::string& os_name) {
    if (os_name.find("Windows") != std::string::npos) return "Windows";
    if (os_name.find("Linux") != std::string::npos) return "Linux";
    if (os_name.find("macOS") != std::string::npos
        || os_name.find("OS X") != std::string::npos) return "macOS";
    if (os_name.find("FreeBSD") != std::string::npos
        || os_name.find("BSD") != std::string::npos) return "BSD";
    if (os_name.find("Solaris") != std::string::npos) return "Solaris";
    if (os_name.find("Cisco") != std::string::npos) return "Network";
    if (os_name.find("Android") != std::string::npos) return "Android";
    return "Unknown";
}

std::string OSDetector::OSDescription(const discovery::OSGuess& guess) {
    std::ostringstream oss;
    oss << guess.name;
    if (guess.confidence > 0) {
        oss << " (" << std::fixed << std::setprecision(0)
            << (guess.confidence * 100) << "%)";
    }
    return oss.str();
}

} // namespace fingerprint
} // namespace netscope
