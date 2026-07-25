#include "netscope/network/arp.h"
#include "netscope/core/logger.h"

#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#endif

namespace netscope {
namespace network {

ARPScanner::ARPScanner() {
    core::Logger::Instance().Debug("ARPScanner initialized");
}

ARPScanner::~ARPScanner() = default;

std::vector<ARPEntry> ARPScanner::ScanLocalNetwork() {
    std::vector<ARPEntry> entries;

#ifdef _WIN32
    ULONG size = 0;
    GetIpNetTable(nullptr, &size, FALSE);

    auto buffer = std::make_unique<char[]>(size);
    auto table = reinterpret_cast<PMIB_IPNETTABLE>(buffer.get());

    if (GetIpNetTable(table, &size, FALSE) != NO_ERROR) {
        core::Logger::Instance().Error("Failed to get ARP table");
        return entries;
    }

    for (DWORD i = 0; i < table->dwNumEntries; ++i) {
        auto& row = table->table[i];
        if (row.dwType == MIB_IPNET_TYPE_INVALID) continue;

        ARPEntry entry;
        struct in_addr addr;
        addr.S_un.S_addr = row.dwAddr;
        entry.ip_address = inet_ntoa(addr);
        entry.mac_address = MacToString(row.bPhysAddr, row.dwPhysAddrLen);

        entry.vendor = LookupVendor(entry.mac_address);
        entries.push_back(entry);
    }
#else
    FILE* fp = fopen("/proc/net/arp", "r");
    if (!fp) {
        core::Logger::Instance().Error("Failed to open /proc/net/arp");
        return entries;
    }

    char line[256];
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        char ip[64], hw_type[16], flags[16], mac[64], mask[16], device[64];
        if (sscanf(line, "%63s %15s %15s %63s %15s %63s",
                   ip, hw_type, flags, mac, mask, device) < 6) {
            continue;
        }

        ARPEntry entry;
        entry.ip_address = ip;
        entry.mac_address = mac;
        entry.interface_name = device;
        entry.vendor = LookupVendor(mac);
        entries.push_back(entry);
    }

    fclose(fp);
#endif

    core::Logger::Instance().Info("ARP scan found " +
                                  std::to_string(entries.size()) + " entries");
    return entries;
}

std::optional<ARPEntry> ARPScanner::Resolve(const std::string& ip) {
    auto entries = ScanLocalNetwork();
    for (const auto& entry : entries) {
        if (entry.ip_address == ip) {
            return entry;
        }
    }
    return std::nullopt;
}

std::string ARPScanner::MacToString(const uint8_t* mac, size_t len) {
    if (len == 0) return "00:00:00:00:00:00";
    std::ostringstream oss;
    for (size_t i = 0; i < len && i < 6; ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << std::setfill('0') << std::setw(2)
            << static_cast<int>(mac[i]);
    }
    return oss.str();
}

std::string ARPScanner::LookupVendor(const std::string& mac) {
    if (mac.length() < 8) return "";

    std::string oui = mac.substr(0, 8);
    std::transform(oui.begin(), oui.end(), oui.begin(), ::toupper);

    auto it = oui_database_.find(oui);
    if (it != oui_database_.end()) {
        return it->second;
    }

    static const std::unordered_map<std::string, std::string> kCommonOUI = {
        {"00:00:0C", "Cisco"},
        {"00:01:5C", "Xerox"},
        {"00:03:93", "Apple"},
        {"00:05:69", "Hewlett-Packard"},
        {"00:0C:29", "VMware"},
        {"00:14:22", "Dell"},
        {"00:16:3E", "Xen"},
        {"00:1A:11", "Google"},
        {"00:1B:21", "Broadcom"},
        {"00:1E:68", "Netgear"},
        {"00:1F:90", "Nokia"},
        {"00:21:5A", "Intel"},
        {"00:23:AE", "Amazon"},
        {"00:24:18", "Intel"},
        {"00:25:90", "Apple"},
        {"00:26:AB", "Apple"},
        {"00:50:56", "VMware"},
        {"00:50:79", "Microsoft"},
        {"00:60:2F", "3Com"},
        {"00:90:27", "Huawei"},
        {"00:E0:4C", "Realtek"},
        {"08:00:27", "Oracle"},
        {"08:00:46", "Intel"},
        {"08:74:02", "Cisco"},
        {"0C:9D:92", "Microsoft"},
        {"10:60:4B", "Apple"},
        {"14:10:9F", "Intel"},
        {"18:1D:EA", "Samsung"},
        {"1C:B0:94", "Dell"},
        {"20:68:9D", "Apple"},
        {"24:4B:FE", "Apple"},
        {"28:16:2E", "Apple"},
        {"2C:54:2D", "Hon Hai"},
        {"30:10:E4", "Google"},
        {"34:02:86", "ASUS"},
        {"34:08:04", "Google"},
        {"3C:07:54", "Intel"},
        {"3C:22:FB", "Dell"},
        {"3C:D0:F8", "Apple"},
        {"40:61:86", "Apple"},
        {"44:D8:31", "Intel"},
        {"48:2C:A0", "ASUS"},
        {"48:45:20", "Google"},
        {"50:3A:7F", "Amazon"},
        {"54:04:A6", "Cisco"},
        {"54:60:09", "Intel"},
        {"58:55:CA", "Apple"},
        {"5C:95:AE", "Intel"},
        {"60:30:D4", "Intel"},
        {"64:B3:10", "Google"},
        {"68:7A:9B", "Intel"},
        {"6C:40:08", "Intel"},
        {"6C:96:CF", "Intel"},
        {"70:5A:0F", "Netgear"},
        {"74:40:BB", "Intel"},
        {"78:45:C4", "ASUS"},
        {"78:4F:43", "Intel"},
        {"7C:04:D0", "Cisco"},
        {"80:86:F2", "Dell"},
        {"84:16:F9", "Intel"},
        {"88:66:5A", "Intel"},
        {"8C:85:90", "Intel"},
        {"90:48:9A", "Intel"},
        {"94:65:2D", "Intel"},
        {"94:B8:6D", "Intel"},
        {"98:01:A7", "Intel"},
        {"98:90:96", "Intel"},
        {"9C:4E:36", "Intel"},
        {"A0:1D:48", "Dell"},
        {"A0:36:9F", "Intel"},
        {"A0:88:B4", "Intel"},
        {"A4:93:3C", "Intel"},
        {"A8:1E:84", "Intel"},
        {"AC:22:0B", "Intel"},
        {"B0:6C:BF", "Intel"},
        {"B4:2E:99", "Intel"},
        {"B8:27:EB", "Raspberry Pi"},
        {"BC:54:2F", "Intel"},
        {"C0:25:06", "Intel"},
        {"C8:5B:76", "Apple"},
        {"CC:2D:8C", "Intel"},
        {"CC:46:D6", "Intel"},
        {"D0:50:99", "Intel"},
        {"D4:AE:52", "Intel"},
        {"D8:BF:C0", "Intel"},
        {"DC:A6:32", "Intel"},
        {"E0:3F:49", "Intel"},
        {"E4:54:E8", "Intel"},
        {"E8:6C:2F", "Intel"},
        {"EC:1A:59", "Intel"},
        {"F0:18:98", "Intel"},
        {"F0:4D:A2", "Intel"},
        {"F4:6D:04", "Intel"},
        {"F8:FF:C2", "Intel"},
        {"FC:AA:14", "Intel"},
        {"FC:F8:AE", "Intel"},
    };

    auto iter = kCommonOUI.find(oui);
    if (iter != kCommonOUI.end()) {
        return iter->second;
    }

    return "Unknown";
}

} // namespace network
} // namespace netscope
