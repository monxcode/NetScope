#include "netscope/core/platform.h"

#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

namespace netscope {
namespace core {

PlatformType Platform::cached_platform_ = PlatformType::Unknown;

PlatformType Platform::Detect() {
    if (cached_platform_ != PlatformType::Unknown) return cached_platform_;
#ifdef _WIN32
    cached_platform_ = PlatformType::Windows;
#elif defined(__linux__)
    cached_platform_ = PlatformType::Linux;
#else
    cached_platform_ = PlatformType::Unknown;
#endif
    return cached_platform_;
}

std::string Platform::OSName() {
    switch (Detect()) {
        case PlatformType::Windows: return "Windows";
        case PlatformType::Linux:   return "Linux";
        default:                    return "Unknown";
    }
}

bool Platform::IsAdmin() {
#ifdef _WIN32
    BOOL is_admin = FALSE;
    PSID admin_group = nullptr;
    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&nt_authority, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0, &admin_group)) {
        CheckTokenMembership(nullptr, admin_group, &is_admin);
        FreeSid(admin_group);
    }
    return is_admin == TRUE;
#else
    return geteuid() == 0;
#endif
}

std::vector<NetworkInterface> Platform::EnumerateInterfaces() {
    std::vector<NetworkInterface> interfaces;
#ifdef _WIN32
    ULONG size = 0;
    GetAdaptersAddresses(AF_INET, 0, nullptr, nullptr, &size);
    auto buf = std::make_unique<char[]>(size);
    auto adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.get());
    if (GetAdaptersAddresses(AF_INET, 0, nullptr, adapter, &size) != ERROR_SUCCESS)
        return interfaces;
    while (adapter) {
        NetworkInterface iface;
        iface.name = adapter->AdapterName;
        iface.description = adapter->Description;
        iface.is_loopback = (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK);
        iface.is_up = (adapter->OperStatus == IfOperStatusUp);
        if (adapter->PhysicalAddressLength > 0) {
            std::ostringstream mac;
            for (DWORD i = 0; i < adapter->PhysicalAddressLength; ++i) {
                if (i > 0) mac << ":";
                mac << std::hex << std::setfill('0') << std::setw(2)
                    << static_cast<int>(adapter->PhysicalAddress[i]);
            }
            iface.mac_address = mac.str();
        }
        auto unicast = adapter->FirstUnicastAddress;
        if (unicast) {
            auto addr = reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr);
            iface.ip_address = inet_ntoa(addr->sin_addr);
        }
        interfaces.push_back(iface);
        adapter = adapter->Next;
    }
#else
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) return interfaces;
    for (auto* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        NetworkInterface iface;
        iface.name = ifa->ifa_name;
        iface.is_loopback = (ifa->ifa_flags & IFF_LOOPBACK) != 0;
        iface.is_up = (ifa->ifa_flags & IFF_UP) != 0;
        auto* addr = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
        iface.ip_address = inet_ntoa(addr->sin_addr);
        if (ifa->ifa_netmask) {
            auto* mask = reinterpret_cast<sockaddr_in*>(ifa->ifa_netmask);
            iface.netmask = inet_ntoa(mask->sin_addr);
        }
        interfaces.push_back(iface);
    }
    freeifaddrs(ifaddr);
#endif
    return interfaces;
}

std::string Platform::GetHostname() {
    char buffer[256];
    if (gethostname(buffer, sizeof(buffer)) == 0) return buffer;
    return "unknown";
}

bool Platform::PingHost(const std::string& ip, int timeout_ms, int retries) {
    return false;
}

std::optional<std::string> Platform::ResolveHostname(const std::string& ip) {
    return std::nullopt;
}

std::chrono::milliseconds Platform::GetTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
}

} // namespace core
} // namespace netscope
