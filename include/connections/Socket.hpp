#pragma once

#include <iostream>
#include <unistd.h>

class Socket {
public:
  explicit Socket(unsigned socket_fd) : m_fd(socket_fd) {}

  ~Socket() {
    std::cout << "Connection closed [fd=" << m_fd << "]" << '\n';
    close(static_cast<int>(m_fd));
  }

  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;
  Socket(Socket &&) = delete;
  Socket &operator=(Socket &&) = delete;

  unsigned fd() const { return m_fd; }

private:
  unsigned m_fd;
};
