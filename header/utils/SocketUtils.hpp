#ifndef SOCKET_UTILS_HPP
#define SOCKET_UTILS_HPP

#include <netinet/in.h>
#include <sys/socket.h>

namespace SocketUtils {

inline auto toSockAddr(sockaddr_in *addr) -> sockaddr * {
  return static_cast<sockaddr *>(static_cast<void *>(addr));
}

inline auto toSockAddr(const sockaddr_in *addr) -> const sockaddr * {
  return static_cast<const sockaddr *>(static_cast<const void *>(addr));
}

} // namespace SocketUtils

#endif // SOCKET_UTILS_HPP
