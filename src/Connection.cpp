#include "Connection.hpp"
#include "unistd.h"
#include <iostream>

Connection::Connection(const unsigned socket_fd, const AppConfig &config)
    : m_socket_fd(socket_fd), m_config(config) {}

Connection::~Connection() {
  std::cout << "Connection closed [fd=" << m_socket_fd << "]" << std::endl;
  close(m_socket_fd);
}