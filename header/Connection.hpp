#pragma once

#include "AppConfig.hpp"

class Connection {
public:
  Connection(const unsigned socket_fd, const AppConfig &config);

  virtual ~Connection();

  virtual void handleConnection() = 0;

  const AppConfig &getConfig() const { return m_config; }

  const unsigned getSocketFd() const { return m_socket_fd; }

private:
  const unsigned m_socket_fd;
  const AppConfig &m_config;
};
