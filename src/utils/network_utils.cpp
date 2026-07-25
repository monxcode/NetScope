#include "netscope/utils/network_utils.h"

#include <sstream>
#include <algorithm>
#include <cstring>
#include <regex>
#include <cmath>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace netscope {
namespace utils {

bool ValidateIP(const std::string& ip) {
    if (ip.empty()) return false;
    struct in_addr addr;
    return inet_pton(AF_INET, ip.c_str(), &addr) == 1;
}

bool ValidateSubnet(const std::string& subnet) {
    auto slash = subnet.find('/');
    if (slash == std::string::npos) return ValidateIP(subnet);

    std::string ip_part = subnet.substr(0, slash);
    if (!ValidateIP(ip_part)) return false;

    try {
        int prefix = std::stoi(subnet.substr(slash + 1));
        return prefix >= 0 && prefix <= 32;
    } catch (...) {
        return false;
    }
}

bool ValidatePort(int port) {
    return port >= 1 && port <= 65535;
}

bool ValidatePortRange(int start, int end) {
    return ValidatePort(start) && ValidatePort(end) && start <= end;
}

std::vector<int> ParsePortList(const std::string& input) {
    std::vector<int> ports;
    std::stringstream ss(input);
    std::string token;

    while (std::getline(ss, token, ',')) {
        token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
        if (token.empty()) continue;

        auto dash = token.find('-');
        if (dash != std::string::npos) {
            try {
                int start = std::stoi(token.substr(0, dash));
                int end = std::stoi(token.substr(dash + 1));
                for (int p = start; p <= end; ++p) {
                    if (ValidatePort(p)) ports.push_back(p);
                }
            } catch (...) {}
        } else {
            try {
                int p = std::stoi(token);
                if (ValidatePort(p)) ports.push_back(p);
            } catch (...) {}
        }
    }

    return ports;
}

std::pair<int, int> ParsePortRange(const std::string& input) {
    auto dash = input.find('-');
    if (dash == std::string::npos) return {0, 0};

    try {
        int start = std::stoi(input.substr(0, dash));
        int end = std::stoi(input.substr(dash + 1));
        if (ValidatePortRange(start, end)) return {start, end};
    } catch (...) {}

    return {0, 0};
}

uint32_t IPToUint(const std::string& ip) {
    struct in_addr addr;
    if (inet_pton(AF_INET, ip.c_str(), &addr) == 1) {
        return ntohl(addr.s_addr);
    }
    return 0;
}

std::string UintToIP(uint32_t addr) {
    struct in_addr in;
    in.s_addr = htonl(addr);
    return inet_ntoa(in);
}

std::string MacToString(const uint8_t* mac, size_t len) {
    if (!mac || len == 0) return "00:00:00:00:00:00";
    std::ostringstream oss;
    for (size_t i = 0; i < len && i < 6; ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << std::setfill('0') << std::setw(2)
            << static_cast<int>(mac[i]);
    }
    return oss.str();
}

uint64_t MacToUint(const std::string& mac) {
    uint64_t result = 0;
    unsigned int bytes[6];
    if (std::sscanf(mac.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
                    &bytes[0], &bytes[1], &bytes[2],
                    &bytes[3], &bytes[4], &bytes[5]) == 6) {
        for (int i = 0; i < 6; ++i) {
            result = (result << 8) | (bytes[i] & 0xFF);
        }
    }
    return result;
}

bool ValidateMAC(const std::string& mac) {
    if (mac.length() != 17) return false;
    unsigned int bytes[6];
    return std::sscanf(mac.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
                       &bytes[0], &bytes[1], &bytes[2],
                       &bytes[3], &bytes[4], &bytes[5]) == 6;
}

std::pair<std::string, int> ParseCIDR(const std::string& subnet) {
    auto slash = subnet.find('/');
    if (slash == std::string::npos) return {subnet, 32};

    std::string ip = subnet.substr(0, slash);
    int prefix = 24;
    try {
        prefix = std::stoi(subnet.substr(slash + 1));
        if (prefix < 0 || prefix > 32) prefix = 24;
    } catch (...) {
        prefix = 24;
    }

    return {ip, prefix};
}

std::vector<std::string> ExpandCIDR(const std::string& subnet, int max_hosts) {
    std::vector<std::string> hosts;

    auto [ip_str, prefix] = ParseCIDR(subnet);
    uint32_t ip = IPToUint(ip_str);
    if (ip == 0) return hosts;

    uint32_t mask = (prefix == 0) ? 0 : ~((1u << (32 - prefix)) - 1);
    uint32_t network = ip & mask;
    uint32_t broadcast = network | ~mask;
    uint32_t host_count = broadcast - network - 1;

    if (host_count > static_cast<uint32_t>(max_hosts)) {
        host_count = static_cast<uint32_t>(max_hosts);
    }

    hosts.reserve(host_count);
    for (uint32_t i = 0; i < host_count; ++i) {
        hosts.push_back(UintToIP(network + i + 1));
    }

    return hosts;
}

std::string IPToCIDR(const std::string& ip, int prefix) {
    return ip + "/" + std::to_string(prefix);
}

} // namespace utils
} // namespace netscope
