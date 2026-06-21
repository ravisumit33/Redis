#pragma once

#include "AppContext.hpp"
#include "Command.hpp"
#include "RespType.hpp"
#include "connections/ExecContext.hpp"
#include "connections/Socket.hpp"
#include "connections/Subscriptions.hpp"
#include "connections/TransactionState.hpp"
#include <string>
#include <vector>

class ClientConnection {
public:
  ClientConnection(unsigned socket_fd, AppContext &context)
      : m_socket(socket_fd), m_context(context), m_subscriptions(socket_fd) {}

  ClientConnection(const ClientConnection &) = delete;
  ClientConnection &operator=(const ClientConnection &) = delete;
  ClientConnection(ClientConnection &&) = delete;
  ClientConnection &operator=(ClientConnection &&) = delete;

  ~ClientConnection();

  unsigned getSocketFd() const { return m_socket.fd(); }
  AppContext &getContext() const { return m_context; }

  void handleConnection();

  ClientExecContext makeExecContext() {
    return {getContext(), getSocketFd(), m_transaction, m_subscriptions};
  }

private:
  void addAsSlave();
  void processCommand(Command command, std::vector<RespValue> args,
                      const std::string &raw_data);
  void handleSubscribedModeError(const Command &command) const;
  void checkAndRegisterSlave(const Command &command,
                             const std::vector<RespValue> &args);

  Socket m_socket;
  AppContext &m_context;
  bool m_is_slave{};
  TransactionState m_transaction;
  Subscriptions m_subscriptions;
};
