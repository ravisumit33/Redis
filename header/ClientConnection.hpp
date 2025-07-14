#pragma once

#include "AppConfig.hpp"
#include "Command.hpp"
#include <istream>
#include <memory>
#include <string>

class ClientConnection {
public:
  ClientConnection(const unsigned socket_fd, const AppConfig &config);
  ~ClientConnection();
  void handleClient();

private:
  const unsigned m_socket_fd;
  AppConfig m_config;
  std::string readFromSocket();
  void writeToSocket(const std::string &data);

  std::pair<Command *, std::vector<std::unique_ptr<RespType>>>
  parseCommand(std::istream &in);
};
