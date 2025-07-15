#pragma once

#include "AppConfig.hpp"
#include "Command.hpp"
#include <istream>
#include <memory>
#include <string>
#include <vector>

class Connection {
public:
  Connection(const unsigned socket_fd, const AppConfig &config);
  ~Connection();
  virtual void handleConnection() = 0;

protected:
  std::string readFromSocket() const;

  void writeToSocket(const std::string &data) const;

  std::pair<Command *, std::vector<std::unique_ptr<RespType>>>
  parseCommand(std::istream &in) const;

  const AppConfig &getConfig() const { return m_config; }

private:
  const unsigned m_socket_fd;
  AppConfig m_config;
};

class ClientConnection : public Connection {
public:
  ClientConnection(const unsigned socket_fd, const AppConfig &config)
      : Connection(socket_fd, config) {}

  virtual void handleConnection() override;
};

class ServerConnection : public Connection {
public:
  ServerConnection(const unsigned socket_fd, const AppConfig &config)
      : Connection(socket_fd, config) {}

  virtual void handleConnection() override;

private:
  void sendCommand(std::vector<std::unique_ptr<RespType>> args);

  void validateResponse(auto validate);

  void configureRepl();

  void configurePsync();

  void handShake();
};
