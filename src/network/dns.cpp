#include "netscope/network/dns.h"
#include "netscope/core/logger.h"

#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <windns.h>
#pragma comment(lib, "dnsapi.lib")
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <resolv.h>
#endif

namespace netscope {
namespace network {

std::optional<std::string> DNSResolver::ResolveHostname(const std::string& ip) {
    struct in_addr addr;
    addr.s_addr = inet_addr(ip.c_str());
    if (addr.s_addr == INADDR_NONE) {
        core::Logger::Instance().Debug("DNS: invalid IP " + ip);
        return std::nullopt;
    }

    struct hostent* host = gethostbyaddr(reinterpret_cast<const char*>(&addr),
                                          sizeof(addr), AF_INET);
    if (host && host->h_name) {
        std::string hostname(host->h_name);
        core::Logger::Instance().Debug("DNS: " + ip + " -> " + hostname);
        return hostname;
    }

    return std::nullopt;
}

std::optional<std::string> DNSResolver::ResolveIP(const std::string& hostname) {
    struct hostent* host = gethostbyname(hostname.c_str());
    if (host && host->h_addr_list[0]) {
        struct in_addr addr;
        std::memcpy(&addr, host->h_addr_list[0], sizeof(addr));
        return std::string(inet_ntoa(addr));
    }
    return std::nullopt;
}

std::vector<std::string> DNSResolver::GetDNSServers() {
    std::vector<std::string> servers;

#ifndef _WIN32
    res_init();
    for (int i = 0; i < _res.nscount; ++i) {
        servers.push_back(inet_ntoa(_res.nsaddr_list[i].sin_addr));
    }
#endif

    if (servers.empty()) {
        servers.emplace_back("8.8.8.8");
        servers.emplace_back("1.1.1.1");
    }
    return servers;
}

} // namespace network
} // namespace netscope
