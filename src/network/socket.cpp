#include "netscope/network/socket.h"

#include <cstring>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#endif

namespace netscope {
namespace network {

WinSockGuard::WinSockGuard() {
#ifdef _WIN32
    WSADATA wsa_data;
    initialized_ = (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0);
#endif
}

WinSockGuard::~WinSockGuard() {
#ifdef _WIN32
    if (initialized_) WSACleanup();
#endif
}

bool WinSockGuard::IsValid() const {
    return initialized_;
}

socket_t CreateSocket(int domain, int type, int protocol) {
    socket_t fd = ::socket(domain, type, protocol);
    if (fd == INVALID_SOCK) {
        throw SocketError("Failed to create socket: " + GetLastErrorString());
    }
    return fd;
}

void CloseSocket(socket_t fd) {
    if (fd == INVALID_SOCK) return;
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

bool SetSocketTimeout(socket_t fd, int timeout_ms) {
#ifdef _WIN32
    DWORD to = timeout_ms;
    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                      reinterpret_cast<const char*>(&to), sizeof(to)) == 0;
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
#endif
}

bool ConnectWithTimeout(socket_t fd, const sockaddr_in& addr, int timeout_ms) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif

    int rc = connect(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));

#ifdef _WIN32
    if (rc != 0 && WSAGetLastError() != WSAEWOULDBLOCK) {
        mode = 0;
        ioctlsocket(fd, FIONBIO, &mode);
        return false;
    }
#else
    if (rc != 0 && errno != EINPROGRESS) {
        fcntl(fd, F_SETFL, flags);
        return false;
    }
#endif

    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(fd, &write_fds);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

#ifdef _WIN32
    int result = select(0, nullptr, &write_fds, nullptr, &tv);
    bool connected = (result > 0);
    mode = 0;
    ioctlsocket(fd, FIONBIO, &mode);
    return connected;
#else
    int result = select(fd + 1, nullptr, &write_fds, nullptr, &tv);
    bool connected = (result > 0);
    if (connected) {
        int sock_err = 0;
        socklen_t len = sizeof(sock_err);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &sock_err, &len) == 0) {
            connected = (sock_err == 0);
        }
    }
    fcntl(fd, F_SETFL, flags);
    return connected;
#endif
}

std::string GetLastErrorString() {
#ifdef _WIN32
    int err = WSAGetLastError();
    std::ostringstream oss;
    oss << "WSA error " << err;
    return oss.str();
#else
    return std::string(std::strerror(errno));
#endif
}

int GetLastErrorCode() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

} // namespace network
} // namespace netscope
