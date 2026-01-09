#include "connections/ClientConnection.hpp"
#include "AppContext.hpp"
#include "Command.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "utils/genericUtils.hpp"
#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <ranges>
#include <sstream>
#include <string>

ClientConnection::~ClientConnection() {
  if (m_is_slave) {
    auto &master_state = getContext().getReplicationManager().master();
    master_state.removeSlave(getSocketFd());
  }
}

void ClientConnection::addAsSlave() {
  auto &master_state = getContext().getReplicationManager().master();
  master_state.addSlave(getSocketFd());
  m_is_slave = true;
}

void ClientConnection::handleSubscribedModeError(const Command &command) {
  std::string command_type = Command::getTypeStr(command.getType());
  std::ranges::transform(command_type, command_type.begin(),
                         [](unsigned char chr) { return std::tolower(chr); });
  RespError resp_error("Can't execute '" + command_type +
                       "' in subscribed mode");
  writeToSocket(getSocketFd(), resp_error.serialize());
}

void ClientConnection::checkAndRegisterSlave(
    const Command &command, const std::vector<RespValue> &args) {
  if (!getContext().getConfig().isMaster()) {
    return;
  }
  if (command.getType() != Command::Type::REPLCONF) {
    return;
  }
  auto first_arg = getStringValue(args.at(0));
  if (first_arg == "listening-port") {
    addAsSlave();
    std::cout << "Client registered as slave [fd=" << getSocketFd() << "]"
              << '\n';
  }
}

void ClientConnection::processCommand(std::unique_ptr<Command> command,
                                      std::vector<RespValue> args,
                                      const std::string &raw_data) {
  if (isInSubscribedMode() && !command->isSubscribedModeCommand()) {
    handleSubscribedModeError(*command);
    return;
  }

  if (isInTransaction() && !command->isControlCommand()) {
    queueCommand(std::move(command), std::move(args));
    RespString resp("QUEUED");
    writeToSocket(getSocketFd(), resp.serialize());
    return;
  }

  checkAndRegisterSlave(*command, args);

  auto responses = command->execute(args, *this);
  std::string resp;
  for (const auto &response : responses) {
    resp += response.serialize();
  }
  writeToSocket(getSocketFd(), resp);

  if (getContext().getConfig().isMaster() && command->isWriteCommand()) {
    auto &master_state = getContext().getReplicationManager().master();
    master_state.updateReplOffset(raw_data.size());
    master_state.propagateToSlave(raw_data);
  }
}

void ClientConnection::handleConnection() {
  try {
    while (true) {
      std::string data = readFromSocket(getSocketFd());
      if (data.empty()) {
        std::cout << "Client disconnected gracefully [fd=" << getSocketFd()
                  << "]" << '\n';
        break;
      }

      std::cout << "Received: " << data << " from client [fd=" << getSocketFd()
                << "]" << '\n';

      std::stringstream socket_data(data);
      while (socket_data.peek() != std::char_traits<char>::eof()) {
        try {
          auto [command, args] = parseCommand(socket_data, getContext());
          processCommand(std::move(command), std::move(args), data);
        } catch (const std::exception &ex) {
          std::cerr << "Command execution error [fd=" << getSocketFd()
                    << "]: " << ex.what() << '\n';
          RespError errResponse(ex.what());
          writeToSocket(getSocketFd(), errResponse.serialize());
        }
      }
    }
  } catch (const std::exception &exp) {
    std::cerr << "Client connection error [fd=" << getSocketFd()
              << "]: " << exp.what() << '\n';
  }
}

RespValue ClientConnection::executeTransaction() {
  RespArray resp_array;
  for (const auto &[cmd, args] : m_queued_commands) {
    auto result = cmd->execute(args, *this);
    for (auto &res : result) {
      resp_array.add(std::move(res));
    }
  }
  m_queued_commands.clear();
  m_in_transaction = false;
  return resp_array;
}

void ClientConnection::RedisSubscriber::onMessage(const std::string &msg) {
  RespArray resp_array;
  resp_array.add(RespBulkString("message"))
      .add(RespBulkString(m_channel_name))
      .add(RespBulkString(msg));
  writeToSocket(m_con.getSocketFd(), resp_array.serialize());
}
