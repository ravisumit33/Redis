#pragma once

#include "Command.hpp"
#include <istream>
#include <memory>
#include <string>
class ClientConnection {
public:
  ClientConnection(const unsigned socket_fd);
  ~ClientConnection();
  void handleClient();

private:
  const unsigned m_socket_fd;
  std::string readFromSocket();
  void writeToSocket(const std::string &data);

  std::pair<std::unique_ptr<Command>, std::vector<std::unique_ptr<RespType>>>
  parseCommand(std::istream &in);
};
