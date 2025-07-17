#include "Connection.hpp"
#include "AppConfig.hpp"
#include "Command.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "RespTypeParser.hpp"
#include "unistd.h"
#include "utils.hpp"
#include <exception>
#include <functional>
#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <utility>
#include <vector>

Connection::Connection(const unsigned socket_fd, const AppConfig &config)
    : m_socket_fd(socket_fd), m_config(config) {}

Connection::~Connection() { close(m_socket_fd); }

std::pair<Command *, std::vector<std::unique_ptr<RespType>>>
Connection::parseCommand(std::istream &in) const {
  char type;
  in.get(type);
  if (!in) {
    throw std::runtime_error("No command RESP type");
  }

  if (type != '*') {
    throw std::runtime_error("Unexpected command RESP type");
  }

  auto parser = RespParserRegistry::instance().get(type);
  if (!parser) {
    throw std::logic_error("Parser not found");
  }
  auto parsed_command = parser->parse(in);
  RespArray &cmd_arr = static_cast<RespArray &>(*parsed_command);

  auto command_args = cmd_arr.release();
  auto command_name_ptr = command_args.at(0).get();
  if (command_name_ptr->getType() != RespType::BULK_STRING) {
    throw std::runtime_error("Unexpected command name RESP type");
  }

  auto cmd_name =
      static_cast<const RespBulkString &>(*command_name_ptr).getValue();
  auto command = CommandRegistry::instance().get(cmd_name);
  if (!command) {
    throw std::runtime_error("Command not supported");
  }
  command_args.erase(command_args.begin());

  return {command, std::move(command_args)};
}

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
        std::cout << "Client disconnected" << std::endl;
        break;
      }

      std::cout << "Received: " << data << " from " << getSocketFd()
                << std::endl;

      std::stringstream ss(data);
      while (ss.peek() != std::char_traits<char>::eof()) {
        try {
          auto [command, args] = parseCommand(ss);

          if (getConfig().isMaster()) {
            if (command->getType() == Command::REPLCONF) {
              auto first_arg =
                  static_cast<RespBulkString &>(*args.at(0)).getValue();
              if (first_arg == "listening-port") {
                addAsSlave();
              }
            }
          }

          auto responses = command->execute(std::move(args), getConfig());
          for (const auto &response : responses) {
            writeToSocket(getSocketFd(), response->serialize());
          }

          if (getConfig().isMaster() && command->isWriteCommand()) {
            auto &master_state = ReplicationManager::getInstance().master();
            if (master_state.hasSlaves()) {
              master_state.propagateToSlave(data);
            }
          }
        } catch (const std::exception &ex) {
          std::cerr << "Exception occurred: " << ex.what() << std::endl;
          RespError errResponse("Unknown command");
          writeToSocket(getSocketFd(), errResponse.serialize());
        }
      }
    }
  } catch (const std::exception &exp) {
    std::cerr << "Error handling client: " << exp.what() << std::endl;
  }
}

void ServerConnection::sendCommand(
    std::vector<std::unique_ptr<RespType>> args) {
  auto array = std::make_unique<RespArray>();
  for (auto &resp_type_ptr : args) {
    array->add(std::move(resp_type_ptr));
  }
  writeToSocket(getSocketFd(), array->serialize());
}

void ServerConnection::validateResponse(auto validate) {
  auto servResponse = readFromSocket(getSocketFd());
  std::cout << "Received: " << servResponse
            << " with number of bytes: " << servResponse.size()
            << " from master at: " << getSocketFd() << std::endl;
  std::istringstream in(servResponse);
  char type;
  in.get(type);
  if (!in) {
    throw std::runtime_error("No response RESP type");
  }

  if (type != '+') {
    throw std::runtime_error("Unexpected response RESP type");
  }

  auto parser = RespParserRegistry::instance().get(type);
  if (!parser) {
    throw std::runtime_error("Malformed response");
  }
  auto parsed_response = parser->parse(in);
  RespString &response = static_cast<RespString &>(*parsed_response);
  validate(response.getValue(), in);
}

void ServerConnection::configureRepl() {
  std::vector<std::unique_ptr<RespType>> cmd;
  cmd.push_back(std::make_unique<RespBulkString>("REPLCONF"));
  cmd.push_back(std::make_unique<RespBulkString>("listening-port"));
  cmd.push_back(
      std::make_unique<RespBulkString>(std::to_string(getConfig().getPort())));
  sendCommand(std::move(cmd));
  validateResponse([](const std::string &response, std::istream &in) {
    if (response != "OK") {
      std::cerr << "Expected OK but got " << response << std::endl;
      throw std::runtime_error("Unexpected response from server");
    }
  });

  cmd.clear();
  cmd.push_back(std::make_unique<RespBulkString>("REPLCONF"));
  cmd.push_back(std::make_unique<RespBulkString>("capa"));
  cmd.push_back(std::make_unique<RespBulkString>("psync2"));
  sendCommand(std::move(cmd));
  validateResponse([](const std::string &response, std::istream &in) {
    if (response != "OK") {
      std::cerr << "Expected OK but got " << response << std::endl;
      throw std::runtime_error("Unexpected response from server");
    }
  });
}

void ServerConnection::configurePsync() {
  std::vector<std::unique_ptr<RespType>> cmd;
  cmd.push_back(std::make_unique<RespBulkString>("PSYNC"));
  cmd.push_back(std::make_unique<RespBulkString>("?"));
  cmd.push_back(std::make_unique<RespBulkString>("-1"));
  sendCommand(std::move(cmd));
  validateResponse([this](const std::string &response, std::istream &in) {
    std::istringstream ss(response);
    ss.exceptions(std::ios::badbit);
    std::string cmd, repl_id;
    unsigned repl_offset;
    ss >> cmd >> repl_id >> repl_offset;
    if (cmd != "FULLRESYNC") {
      std::cerr << "Expected FULLRESYNC but got " << response << std::endl;
      throw std::runtime_error("Unexpected response from server");
    }
    std::cout << "FULLRESYNC recieved" << std::endl;

    auto extractRDB = [](std::istream &ins) -> bool {
      char type;
      ins.get(type);
      if (!ins || type != '$') {
        return false;
      }
      auto parser = RespParserRegistry::instance().get(type);
      if (!parser) {
        return false;
      }
      auto bulk_string_parser = static_cast<RespBulkStringParser &>(*parser);
      bulk_string_parser.disableLastCrlfParsing();
      auto parsed_rdb_ptr = bulk_string_parser.parse(ins); // TODO: Store RDB
      auto rdb_bulk_string =
          static_cast<RespBulkString &>(*parsed_rdb_ptr.get());
      return true;
    };

    std::reference_wrapper<std::istream> input_stream_ref = in;

    if (in.peek() != std::char_traits<char>::eof()) {
      if (!extractRDB(in)) {
        throw std::runtime_error("Unexpected RDB file");
      }
    } else {
      std::string servResponse = readFromSocket(getSocketFd());
      std::istringstream ss(servResponse);
      input_stream_ref = ss;
      if (!extractRDB(input_stream_ref)) {
        throw std::runtime_error("Unexpected RDB file");
      }
    }

    std::cout << "RDB extraction done" << std::endl;

    auto &input_stream = input_stream_ref.get();
    while (input_stream.peek() != std::char_traits<char>::eof()) {
      auto [command, args] = parseCommand(input_stream);
      auto responses = command->execute(std::move(args), getConfig());
      if (command->getType() == Command::REPLCONF) {
        for (const auto &response : responses) {
          writeToSocket(getSocketFd(), response->serialize());
        }
      }
    }

    std::cout << "Backlog commands executed" << std::endl;
  });
}

void ServerConnection::handShake() {
  std::vector<std::unique_ptr<RespType>> cmd;
  cmd.push_back(std::make_unique<RespBulkString>("PING"));
  sendCommand(std::move(cmd));
  validateResponse([](const std::string &response, std::istream &in) {
    if (response != "PONG") {
      std::cerr << "Expected PONG but got " << response << std::endl;
      throw std::runtime_error("Unexpected response from server");
    }
  });
  std::cout << "PING complete" << std::endl;

  configureRepl();
  std::cout << "Repl configured" << std::endl;

  configurePsync();
  std::cout << "Psync configured" << std::endl;
}

void ServerConnection::handleConnection() {
  handShake();
  m_ready_callback();
  std::cout << "Handshake done" << std::endl;

  try {
    while (true) {
      std::string data = readFromSocket(getSocketFd());
      if (data.empty()) {
        std::cout << "Server disconnected" << std::endl;
        break;
      }

      std::cout << "Received: " << data << " from " << getSocketFd()
                << std::endl;

      std::stringstream ss(data);
      while (ss.peek() != std::char_traits<char>::eof()) {
        auto [command, args] = parseCommand(ss);
        auto responses = command->execute(std::move(args), getConfig());
        if (command->getType() == Command::REPLCONF) {
          for (const auto &response : responses) {
            writeToSocket(getSocketFd(), response->serialize());
          }
        }
      }
    }
  } catch (const std::exception &exp) {
    std::cerr << "Error handling server: " << exp.what() << std::endl;
  }
}
