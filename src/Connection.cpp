#include "Connection.hpp"
#include "AppContext.hpp"
#include "unistd.h"
#include <iostream>

Connection::Connection(const Type con_type, const unsigned socket_fd,
                       AppContext &context)
    : m_type(con_type), m_socket_fd(socket_fd), m_context(context) {}

Connection::~Connection() {
  std::cout << "Connection closed [fd=" << m_socket_fd << "]" << '\n';
  close(static_cast<int>(m_socket_fd));
}
