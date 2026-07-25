#include "netscope/core/platform.h"

#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <fcntl.h>
#endif

namespace netscope {
namespace core {

PlatformType Platform::cached_platform_ = PlatformType::Unknown;

PlatformType Platform::Detect() {
    if (cached_platform_ != PlatformType::Unknown) {
        return cached_platform_;
    }

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
        case PlatformType::Windows:
            return "Windows";
        case PlatformType::Linux:
            return "Linux";
        default:
            return "Unknown";
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
                                 0, 0, 0, 0, 0, 0,
                                 &admin_group)) {
        if (!CheckTokenMembership(nullptr, admin_group, &is_admin)) {
            is_admin = FALSE;
        }
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
    DWORD size = 0;
    GetAdaptersAddresses(AF_INET, 0, nullptr, nullptr, &size);

    auto addresses = std::make_unique<char[]>(size);
    auto adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(addresses.get());

    if (GetAdaptersAddresses(AF_INET, 0, nullptr, adapter, &size) != ERROR_SUCCESS) {
        return interfaces;
    }

    while (adapter) {
        NetworkInterface iface;
        iface.name = adapter->AdapterName;
        iface.description = adapter->Description;
        iface.is_loopback = adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK;
        iface.is_up = adapter->OperStatus == IfOperStatusUp;

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
    if (getifaddrs(&ifaddr) == -1) {
        return interfaces;
    }

    for (auto* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

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
    if (gethostname(buffer, sizeof(buffer)) == 0) {
        return buffer;
    }
    return "unknown";
}

bool Platform::PingHost(const std::string& ip, int timeout_ms, int retries) {
#ifdef _WIN32
    HANDLE icmp = IcmpCreateFile();
    if (icmp == INVALID_HANDLE_VALUE) {
        return false;
    }

    unsigned long addr = inet_addr(ip.c_str());
    char send_data[32] = {0};
    char reply_buffer[sizeof(ICMP_ECHO_REPLY) + sizeof(send_data)];
    bool alive = false;

    for (int i = 0; i < retries; ++i) {
        auto result = IcmpSendEcho(icmp, addr, send_data, sizeof(send_data),
                                    nullptr, reply_buffer,
                                    sizeof(reply_buffer), timeout_ms);
        if (result > 0) {
            auto* reply = reinterpret_cast<PICMP_ECHO_REPLY>(reply_buffer);
            if (reply->Status == IP_SUCCESS) {
                alive = true;
                break;
            }
        }
    }

    IcmpCloseHandle(icmp);
    return alive;
#else
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (sock < 0) {
        return false;
    }

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    bool alive = false;
    for (int i = 0; i < retries; ++i) {
        if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
            alive = true;
            break;
        }
    }

    close(sock);
    return alive;
#endif
}

std::optional<std::string> Platform::ResolveHostname(const std::string& ip) {
    struct in_addr addr;
    addr.s_addr = inet_addr(ip.c_str());

    struct hostent* host = gethostbyaddr(reinterpret_cast<const char*>(&addr),
                                          sizeof(addr), AF_INET);
    if (host && host->h_name) {
        return std::string(host->h_name);
    }

    return std::nullopt;
}

std::chrono::milliseconds Platform::GetTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
}

} // namespace core
} // namespace netscope
