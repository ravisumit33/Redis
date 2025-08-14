#include "connections/ClientConnection.hpp"
#include "Command.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "utils/genericUtils.hpp"
#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

ClientConnection::~ClientConnection() {
  if (m_is_slave) {
    auto &master_state = ReplicationManager::getInstance().master();
    master_state.removeSlave(getSocketFd());
  }
}

void ClientConnection::addAsSlave() {
  auto &master_state = ReplicationManager::getInstance().master();
  master_state.addSlave(getSocketFd());
  m_is_slave = true;
}

void ClientConnection::handleConnection() {
  try {
    while (true) {
      std::string data = readFromSocket(getSocketFd());
      if (data.empty()) {
        std::cout << "Client disconnected gracefully [fd=" << getSocketFd()
                  << "]" << std::endl;
        break;
      }

      std::cout << "Received: " << data << " from client [fd=" << getSocketFd()
                << "]" << std::endl;

      std::stringstream ss(data);
      while (ss.peek() != std::char_traits<char>::eof()) {
        try {
          auto [command, args] = parseCommand(ss);

          if (isInSubscribedMode() && !command->isSubscribedModeCommand()) {
            std::string command_type = command->getTypeStr();
            std::transform(command_type.begin(), command_type.end(),
                           command_type.begin(), tolower);
            auto resp_error = std::make_unique<RespError>(
                "Can't execute '" + command_type + "' in subscribed mode");
            writeToSocket(getSocketFd(), resp_error->serialize());
            continue;
          }

          if (isInTransaction() && !command->isControlCommand()) {
            queueCommand(std::move(command), std::move(args));
            auto resp = std::make_unique<RespString>("QUEUED");
            writeToSocket(getSocketFd(), resp->serialize());
          } else {

            if (getConfig().isMaster()) {
              if (command->getType() == Command::REPLCONF) {
                auto first_arg =
                    static_cast<RespBulkString &>(*args.at(0)).getValue();
                if (first_arg == "listening-port") {
                  addAsSlave();
                  std::cout
                      << "Client registered as slave [fd=" << getSocketFd()
                      << "]" << std::endl;
                }
              }
            }

            auto responses = command->execute(args, *this);
            std::string resp;
            for (const auto &response : responses) {
              resp += response->serialize();
            }
            writeToSocket(getSocketFd(), resp);

            if (getConfig().isMaster() && command->isWriteCommand()) {
              auto &master_state = ReplicationManager::getInstance().master();
              master_state.updateReplOffset(data.size());
              master_state.propagateToSlave(data);
            }
          }
        } catch (const std::exception &ex) {
          std::cerr << "Command execution error [fd=" << getSocketFd()
                    << "]: " << ex.what() << std::endl;
          RespError errResponse(ex.what());
          writeToSocket(getSocketFd(), errResponse.serialize());
        }
      }
    }
  } catch (const std::exception &exp) {
    std::cerr << "Client connection error [fd=" << getSocketFd()
              << "]: " << exp.what() << std::endl;
  }
}

std::unique_ptr<RespType> ClientConnection::executeTransaction() {
  auto resp_array = std::make_unique<RespArray>();
  for (const auto &[cmd, args] : m_queued_commands) {
    auto result = cmd->execute(args, *this);
    for (auto &res : result) {
      resp_array->add(std::move(res));
    }
  }
  m_queued_commands.clear();
  m_in_transaction = false;
  return resp_array;
}

void ClientConnection::RedisSubscriber::onMessage(const std::string &msg) {
  auto resp_array = std::make_unique<RespArray>();
  resp_array->add(std::make_unique<RespBulkString>("message"))
      ->add(std::make_unique<RespBulkString>(m_channel_name))
      ->add(std::make_unique<RespBulkString>(msg));
  ;
  writeToSocket(m_con->getSocketFd(), resp_array->serialize());
}
