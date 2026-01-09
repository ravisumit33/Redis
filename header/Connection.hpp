#pragma once

#include "AppContext.hpp"
#include <cstdint>

class Connection {
public:
  enum class Type : std::uint8_t { CLIENT, SERVER };

  Connection(Type con_type, unsigned socket_fd, AppContext &context);

  virtual ~Connection();
  Connection(const Connection &) = delete;
  Connection &operator=(const Connection &) = delete;
  Connection(Connection &&) = delete;
  Connection &operator=(Connection &&) = delete;

  virtual void handleConnection() = 0;

  virtual Type getConnectionType() const { return m_type; };

  AppContext &getContext() const { return m_context; }

  unsigned getSocketFd() const { return m_socket_fd; }

  Type getType() const { return m_type; }

private:
  const unsigned m_socket_fd;
  AppContext &m_context;
  Type m_type;
};
