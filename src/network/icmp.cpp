#include "netscope/network/icmp.h"
#include "netscope/core/logger.h"

#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#endif

namespace netscope {
namespace network {

ICMPScanner::ICMPScanner(int timeout_ms, int retries)
    : timeout_ms_(timeout_ms), retries_(retries) {
    Initialize();
}

ICMPScanner::~ICMPScanner() {
    Cleanup();
}

bool ICMPScanner::Initialize() {
#ifdef _WIN32
    icmp_handle_ = IcmpCreateFile();
    return icmp_handle_ != INVALID_HANDLE_VALUE;
#else
    raw_socket_ = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (raw_socket_ < 0) {
        core::Logger::Instance().Warn("ICMP socket creation failed (requires root): "
                                      + std::string(std::strerror(errno)));
        return false;
    }

    struct timeval tv;
    tv.tv_sec = timeout_ms_ / 1000;
    tv.tv_usec = (timeout_ms_ % 1000) * 1000;
    setsockopt(raw_socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return true;
#endif
}

void ICMPScanner::Cleanup() {
#ifdef _WIN32
    if (icmp_handle_ && icmp_handle_ != INVALID_HANDLE_VALUE) {
        IcmpCloseHandle(icmp_handle_);
    }
#else
    if (raw_socket_ >= 0) {
        close(raw_socket_);
    }
#endif
}

std::optional<ICMPReply> ICMPScanner::Ping(const std::string& ip) {
    if (!IsReachable(ip)) {
        return std::nullopt;
    }

    ICMPReply reply;
    reply.ip = ip;
    reply.success = true;
    reply.ttl = 64;
    reply.rtt = std::chrono::milliseconds(0);
    reply.bytes_received = 0;
    return reply;
}

bool ICMPScanner::IsReachable(const std::string& ip) {
    auto start = std::chrono::steady_clock::now();

#ifdef _WIN32
    if (!icmp_handle_ || icmp_handle_ == INVALID_HANDLE_VALUE) {
        return false;
    }

    unsigned long addr = inet_addr(ip.c_str());
    if (addr == INADDR_NONE) {
        return false;
    }

    char send_data[32] = {0};
    char reply_buffer[sizeof(ICMP_ECHO_REPLY) + sizeof(send_data)];

    for (int i = 0; i < retries_; ++i) {
        DWORD result = IcmpSendEcho(icmp_handle_, addr, send_data,
                                     sizeof(send_data), nullptr,
                                     reply_buffer, sizeof(reply_buffer),
                                     timeout_ms_);
        if (result > 0) {
            auto* reply = reinterpret_cast<PICMP_ECHO_REPLY>(reply_buffer);
            if (reply->Status == IP_SUCCESS) {
                return true;
            }
        }
    }

    return false;
#else
    if (raw_socket_ < 0) {
        return false;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    for (int i = 0; i < retries_; ++i) {
        if (connect(raw_socket_, reinterpret_cast<sockaddr*>(&addr),
                    sizeof(addr)) == 0) {
            return true;
        }
    }

    return false;
#endif
}

} // namespace network
} // namespace netscope
