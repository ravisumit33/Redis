#include "utils/SocketUtils.hpp"
#include <array>
#include <iostream>
#include <string>
#include <sys/socket.h>

namespace SocketUtils {

std::string readFromSocket(unsigned socket_fd) {
  constexpr size_t buffer_size = 1024;
  std::array<char, buffer_size> buffer{};
  ssize_t bytes_received =
      recv(static_cast<int>(socket_fd), buffer.data(), buffer.size() - 1, 0);
  if (bytes_received <= 0) {
    return "";
  }
  buffer.at(static_cast<size_t>(bytes_received)) = '\0';
  return {buffer.data(), static_cast<size_t>(bytes_received)};
}

void writeToSocket(unsigned socket_fd, const std::string &data) {
  std::cout << "Sent: " << data << " to [fd=" << socket_fd << "]" << '\n';
  send(static_cast<int>(socket_fd), data.c_str(), data.length(), 0);
}

} // namespace SocketUtils
