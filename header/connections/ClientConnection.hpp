#pragma once

#include "Connection.hpp"
#include "RespType.hpp"
#include <memory>
#include <vector>

class Command;

class ClientConnection : public Connection {
public:
  ClientConnection(const unsigned socket_fd, const AppConfig &config)
      : Connection(socket_fd, config), m_is_slave(false) {}

  virtual void handleConnection() override;

  ~ClientConnection();

  void beginTransaction() {
    m_in_transaction = true;
    m_queued_commands.clear();
  }

  void queueCommand(Command *cmd, std::vector<std::unique_ptr<RespType>> args) {
    m_queued_commands.emplace_back(cmd, std::move(args));
  }

  std::unique_ptr<RespType> executeTransaction();

  void discardTransaction() {
    m_queued_commands.clear();
    m_in_transaction = false;
  }

  bool isInTransaction() { return m_in_transaction == true; }

private:
  void addAsSlave();

  bool m_is_slave;

  bool m_in_transaction = false;

  std::vector<std::pair<Command *, std::vector<std::unique_ptr<RespType>>>>
      m_queued_commands;
};
