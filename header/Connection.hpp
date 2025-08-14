#pragma once

#include "AppConfig.hpp"

class Connection {
public:
  enum class Type { CLIENT, SERVER };

  Connection(const Type t, const unsigned socket_fd, const AppConfig &config);

  virtual ~Connection();

  virtual void handleConnection() = 0;

  virtual Type getConnectionType() const { return m_type; };

  const AppConfig &getConfig() const { return m_config; }

  const unsigned getSocketFd() const { return m_socket_fd; }

  Type getType() const { return m_type; }

private:
  const unsigned m_socket_fd;
  const AppConfig &m_config;
  Type m_type;
};
