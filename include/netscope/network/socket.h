#ifndef NETSCOPE_NETWORK_SOCKET_H
#define NETSCOPE_NETWORK_SOCKET_H

#include <string>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
using socket_t = int;
constexpr socket_t INVALID_SOCK = -1;
#endif

namespace netscope {
namespace network {

class SocketError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class WinSockGuard {
public:
    WinSockGuard();
    ~WinSockGuard();
    bool IsValid() const;

private:
    bool initialized_{false};
};

socket_t CreateSocket(int domain, int type, int protocol);
void CloseSocket(socket_t fd);
bool SetSocketTimeout(socket_t fd, int timeout_ms);
bool ConnectWithTimeout(socket_t fd, const sockaddr_in& addr, int timeout_ms);
std::string GetLastErrorString();
int GetLastErrorCode();

} // namespace network
} // namespace netscope

#endif
