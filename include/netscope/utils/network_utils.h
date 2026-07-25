#ifndef NETSCOPE_UTILS_NETWORK_UTILS_H
#define NETSCOPE_UTILS_NETWORK_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

namespace netscope {
namespace utils {

bool ValidateIP(const std::string& ip);
bool ValidateSubnet(const std::string& subnet);
bool ValidatePort(int port);
bool ValidatePortRange(int start, int end);

std::vector<int> ParsePortList(const std::string& input);
std::pair<int, int> ParsePortRange(const std::string& input);

uint32_t IPToUint(const std::string& ip);
std::string UintToIP(uint32_t addr);

std::string MacToString(const uint8_t* mac, size_t len);
uint64_t MacToUint(const std::string& mac);
bool ValidateMAC(const std::string& mac);

std::pair<std::string, int> ParseCIDR(const std::string& subnet);
std::vector<std::string> ExpandCIDR(const std::string& subnet, int max_hosts = 256);

std::string IPToCIDR(const std::string& ip, int prefix = 24);

} // namespace utils
} // namespace netscope

#endif
