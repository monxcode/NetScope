#include "netscope/network/icmp.h"
#include "netscope/core/logger.h"

#include <chrono>
#include <cstring>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
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
    initialized_ = (icmp_handle_ != INVALID_HANDLE_VALUE);
    if (!initialized_) {
        core::Logger::Instance().Warn("ICMP: IcmpCreateFile failed (need admin?)");
    }
#else
    raw_socket_ = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (raw_socket_ < 0) {
        core::Logger::Instance().Warn("ICMP: raw socket failed (need root): "
                                      + std::string(std::strerror(errno)));
        initialized_ = false;
        return false;
    }
    struct timeval tv;
    tv.tv_sec = timeout_ms_ / 1000;
    tv.tv_usec = (timeout_ms_ % 1000) * 1000;
    setsockopt(raw_socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    initialized_ = true;
#endif
    return initialized_;
}

void ICMPScanner::Cleanup() {
#ifdef _WIN32
    if (icmp_handle_ && icmp_handle_ != INVALID_HANDLE_VALUE) {
        IcmpCloseHandle(icmp_handle_);
        icmp_handle_ = nullptr;
    }
#else
    if (raw_socket_ >= 0) {
        close(raw_socket_);
        raw_socket_ = -1;
    }
#endif
    initialized_ = false;
}

std::optional<ICMPReply> ICMPScanner::Ping(const std::string& ip) {
    if (!initialized_) {
        core::Logger::Instance().Debug("ICMP not initialized, skipping " + ip);
        return std::nullopt;
    }

    auto start = std::chrono::steady_clock::now();

#ifdef _WIN32
    unsigned long addr = inet_addr(ip.c_str());
    if (addr == INADDR_NONE) return std::nullopt;

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
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start);
                ICMPReply r;
                r.ip = ip;
                r.ttl = reply->Options.Ttl;
                r.rtt = elapsed;
                r.success = true;
                r.bytes_received = reply->DataSize;
                return r;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
#else
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    for (int i = 0; i < retries_; ++i) {
        struct icmphdr icmp_hdr;
        std::memset(&icmp_hdr, 0, sizeof(icmp_hdr));
        icmp_hdr.type = ICMP_ECHO;
        icmp_hdr.code = 0;
        icmp_hdr.un.echo.id = getpid();
        icmp_hdr.un.echo.sequence = i;
        icmp_hdr.checksum = 0;

        char packet[64];
        std::memcpy(packet, &icmp_hdr, sizeof(icmp_hdr));
        std::memset(packet + sizeof(icmp_hdr), 0, sizeof(packet) - sizeof(icmp_hdr));

        sendto(raw_socket_, packet, sizeof(packet), 0,
               reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

        char recv_buffer[256];
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);

        int bytes = recvfrom(raw_socket_, recv_buffer, sizeof(recv_buffer),
                             0, reinterpret_cast<struct sockaddr*>(&from),
                             &from_len);
        if (bytes > 0) {
            struct iphdr* ip_hdr = reinterpret_cast<struct iphdr*>(recv_buffer);
            struct icmphdr* icmp_resp = reinterpret_cast<struct icmphdr*>(
                recv_buffer + (ip_hdr->ihl * 4));

            if (icmp_resp->type == ICMP_ECHOREPLY &&
                icmp_resp->un.echo.id == getpid()) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start);
                ICMPReply r;
                r.ip = ip;
                r.ttl = ip_hdr->ttl;
                r.rtt = elapsed;
                r.success = true;
                r.bytes_received = bytes;
                return r;
            }
        }
    }
#endif

    return std::nullopt;
}

bool ICMPScanner::IsReachable(const std::string& ip) {
    auto result = Ping(ip);
    return result.has_value() && result->success;
}

} // namespace network
} // namespace netscope
