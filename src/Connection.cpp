#include "Connection.hpp"
#include "AppConfig.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "RespTypeParser.hpp"
#include "unistd.h"
#include "utils.hpp"
#include <cstddef>
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

Connection::~Connection() {
  std::cout << "Connection closed [fd=" << m_socket_fd << "]" << std::endl;
  close(m_socket_fd);
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

          if (getConfig().isMaster()) {
            if (command->getType() == Command::REPLCONF) {
              auto first_arg =
                  static_cast<RespBulkString &>(*args.at(0)).getValue();
              if (first_arg == "listening-port") {
                addAsSlave();
                std::cout << "Client registered as slave [fd=" << getSocketFd()
                          << "]" << std::endl;
              }
            }
          }

          auto responses = command->execute(args, getConfig(), getSocketFd());
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
        } catch (const std::exception &ex) {
          std::cerr << "Command execution error [fd=" << getSocketFd()
                    << "]: " << ex.what() << std::endl;
          RespError errResponse("ERR " + std::string(ex.what()));
          writeToSocket(getSocketFd(), errResponse.serialize());
        }
      }
    }
  } catch (const std::exception &exp) {
    std::cerr << "Client connection error [fd=" << getSocketFd()
              << "]: " << exp.what() << std::endl;
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
  std::cout << "Received: " << servResponse << " from master: " << getSocketFd()
            << std::endl;
  std::istringstream in(servResponse);
  char type;
  in.get(type);
  if (!in) {
    throw std::runtime_error("No response RESP type");
  }

  if (type != '+') {
    throw std::runtime_error(
        "Replication handshake: Expected simple string '+' response, got '" +
        std::string(1, type) + "'");
  }

  auto parser = RespParserRegistry::instance().get(type);
  if (!parser) {
    throw std::runtime_error(
        "Replication handshake: No parser found for response type '" +
        std::string(1, type) + "'");
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
  validateResponse([](const std::string &response, std::istringstream &in) {
    if (response != "OK") {
      std::cerr << "REPLCONF listening-port: Expected 'OK' but got '"
                << response << "'" << std::endl;
      throw std::runtime_error("REPLCONF listening-port failed: got '" +
                               response + "' instead of 'OK'");
    }
  });

  cmd.clear();
  cmd.push_back(std::make_unique<RespBulkString>("REPLCONF"));
  cmd.push_back(std::make_unique<RespBulkString>("capa"));
  cmd.push_back(std::make_unique<RespBulkString>("psync2"));
  sendCommand(std::move(cmd));
  validateResponse([](const std::string &response, std::istringstream &in) {
    if (response != "OK") {
      std::cerr << "REPLCONF capa: Expected 'OK' but got '" << response << "'"
                << std::endl;
      throw std::runtime_error("REPLCONF capa failed: got '" + response +
                               "' instead of 'OK'");
    }
  });
}

void ServerConnection::configurePsync() {
  std::vector<std::unique_ptr<RespType>> cmd;
  cmd.push_back(std::make_unique<RespBulkString>("PSYNC"));
  cmd.push_back(std::make_unique<RespBulkString>("?"));
  cmd.push_back(std::make_unique<RespBulkString>("-1"));
  sendCommand(std::move(cmd));
  validateResponse([this](const std::string &response, std::istringstream &in) {
    std::istringstream stream(response);
    stream.exceptions(std::ios::badbit);
    std::string cmd, repl_id;
    unsigned repl_offset;
    stream >> cmd >> repl_id >> repl_offset;
    if (cmd != "FULLRESYNC") {
      std::cerr << "PSYNC: Expected 'FULLRESYNC' but got '" << response << "'"
                << std::endl;
      throw std::runtime_error("PSYNC failed: expected 'FULLRESYNC' but got '" +
                               response + "'");
    }
    std::cout << "FULLRESYNC recieved" << std::endl;

    auto extractRDB = [this](std::istream &ins) -> bool {
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
      std::cout << "Received RDB file from master [fd=" << getSocketFd() << "]"
                << std::endl;
      return true;
    };

    std::reference_wrapper<std::istringstream> input_stream_ref = in;
    std::optional<std::istringstream> next_server_response;

    if (in.peek() != std::char_traits<char>::eof()) {
      if (!extractRDB(in)) {
        throw std::runtime_error("Unexpected RDB file");
      }
    } else {
      std::string servResponse = readFromSocket(getSocketFd());
      std::cout << "Received: " << servResponse
                << " from master [fd=" << getSocketFd() << "]" << std::endl;
      next_server_response.emplace(servResponse);
      if (!extractRDB(*next_server_response)) {
        throw std::runtime_error("Unexpected RDB file");
      }
      input_stream_ref = *next_server_response;
    }

    auto &input_stream = input_stream_ref.get();
    std::streampos pos = input_stream.tellg();
    std::size_t total_bytes_after_handshake =
        (pos != -1)
            ? (input_stream.str().size() - static_cast<std::size_t>(pos))
            : 0;
    std::size_t bytes_recorded = 0;
    while (input_stream.peek() != std::char_traits<char>::eof()) {
      auto [command, args] = parseCommand(input_stream);
      auto responses = command->execute(args, getConfig(), getSocketFd());
      std::string resp;
      for (const auto &response : responses) {
        resp += response->serialize();
      }
      if (command->getType() == Command::REPLCONF) {
        writeToSocket(getSocketFd(), resp);
      }
      std::streampos pos = input_stream.tellg();
      std::size_t bytes_remaining =
          (pos != -1)
              ? (input_stream.str().size() - static_cast<std::size_t>(pos))
              : 0;
      std::size_t bytes_read = total_bytes_after_handshake - bytes_remaining;
      std::size_t new_bytes_read = bytes_read - bytes_recorded;
      auto &slave_state = ReplicationManager::getInstance().slave();
      slave_state.addBytesProcessed(new_bytes_read);
      bytes_recorded += new_bytes_read;
    }

    std::cout << "Backlog commands executed" << std::endl;
  });
}

void ServerConnection::handShake() {
  std::vector<std::unique_ptr<RespType>> cmd;
  cmd.push_back(std::make_unique<RespBulkString>("PING"));
  sendCommand(std::move(cmd));
  validateResponse([](const std::string &response, std::istringstream &in) {
    if (response != "PONG") {
      std::cerr << "PING handshake: Expected 'PONG' but got '" << response
                << "'" << std::endl;
      throw std::runtime_error("PING handshake failed: got '" + response +
                               "' instead of 'PONG'");
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
        std::cout << "Server disconnected at: " << getSocketFd() << std::endl;
        break;
      }

      std::cout << "Received: " << data << " from master: " << getSocketFd()
                << std::endl;

      std::stringstream ss(data);
      std::size_t total_bytes_received = data.size();
      std::size_t bytes_recorded = 0;
      while (ss.peek() != std::char_traits<char>::eof()) {
        auto [command, args] = parseCommand(ss);
        auto responses = command->execute(args, getConfig(), getSocketFd());
        std::string serialized_response;
        for (const auto &response : responses) {
          serialized_response += response->serialize();
        }
        if (command->getType() == Command::REPLCONF) {
          writeToSocket(getSocketFd(), serialized_response);
        }
        std::streampos pos = ss.tellg();
        std::size_t bytes_remaining =
            (pos != -1) ? (ss.str().size() - static_cast<std::size_t>(pos)) : 0;
        std::size_t bytes_read = total_bytes_received - bytes_remaining;
        std::size_t new_bytes_read = bytes_read - bytes_recorded;
        auto &slave_state = ReplicationManager::getInstance().slave();
        slave_state.addBytesProcessed(new_bytes_read);
        bytes_recorded += new_bytes_read;
      }
    }
  } catch (const std::exception &exp) {
    std::cerr << "Server connection error [fd=" << getSocketFd()
              << "]: " << exp.what() << std::endl;
  }
}
