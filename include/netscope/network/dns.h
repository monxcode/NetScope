#ifndef NETSCOPE_NETWORK_DNS_H
#define NETSCOPE_NETWORK_DNS_H

#include <string>
#include <optional>
#include <vector>

namespace netscope {
namespace network {

class DNSResolver {
public:
    DNSResolver() = default;

    static std::optional<std::string> ResolveHostname(const std::string& ip);
    static std::optional<std::string> ResolveIP(const std::string& hostname);
    static std::vector<std::string> GetDNSServers();

    DNSResolver(const DNSResolver&) = delete;
    DNSResolver& operator=(const DNSResolver&) = delete;
};

} // namespace network
} // namespace netscope

#endif
