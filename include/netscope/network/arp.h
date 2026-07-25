#ifndef NETSCOPE_NETWORK_ARP_H
#define NETSCOPE_NETWORK_ARP_H

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace netscope {
namespace network {

struct ARPEntry {
    std::string ip_address;
    std::string mac_address;
    std::string interface_name;
    std::string vendor;
};

class ARPScanner {
public:
    ARPScanner();
    ~ARPScanner();

    std::vector<ARPEntry> ScanLocalNetwork();
    std::optional<ARPEntry> Resolve(const std::string& ip);

    ARPScanner(const ARPScanner&) = delete;
    ARPScanner& operator=(const ARPScanner&) = delete;

private:
    std::string MacToString(const uint8_t* mac, size_t len);
    std::string LookupVendor(const std::string& mac);

    std::unordered_map<std::string, std::string> oui_database_;
};

} // namespace network
} // namespace netscope

#endif
